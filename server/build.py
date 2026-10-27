import subprocess, tempfile, shutil, json, hashlib, os
from pathlib import Path

BASE_SKETCH = Path(__file__).parent / "sketch_src"
BUILD_DIR = Path(__file__).parent / "builds"
LOG_DIR = Path(__file__).parent / "build_logs"
BUILD_DIR.mkdir(exist_ok=True)
LOG_DIR.mkdir(exist_ok=True)

FQBN = "esp32:esp32:esp32c3:PartitionScheme=min_spiffs"

def build(modules: dict, version="custom", task_id="manual") -> dict:
    if not BASE_SKETCH.exists():
        return {"ok": False, "error": "Нет папки sketch_src"}
    
    ino_files = list(BASE_SKETCH.glob("*.ino"))
    if not ino_files:
        return {"ok": False, "error": "Нет .ino файла в sketch_src"}

    log_path = LOG_DIR / f"{task_id}.log"
    with open(log_path, "w", encoding="utf-8") as logf:
        logf.write(f"=== Build started: {version} at {__import__('datetime').datetime.now()} ===\n")
        logf.flush()
        
        work = Path(tempfile.mkdtemp(prefix="mb_"))
        sketch_dir = work / "MINI_BOY"
        sketch_dir.mkdir()
        
        logf.write(f"[1/4] Копирую файлы из {BASE_SKETCH} → {sketch_dir}\n")
        logf.flush()
        for f in BASE_SKETCH.iterdir():
            if f.is_file():
                shutil.copy(f, sketch_dir / f.name)
        
        logf.write(f"[2/4] Переименовываю .ino → MINI_BOY.ino\n")
        logf.flush()
        for ino in sketch_dir.glob("*.ino"):
            if ino.name != "MINI_BOY.ino":
                target = sketch_dir / "MINI_BOY.ino"
                if target.exists():
                    target.unlink()
                ino.rename(target)
                break
        
        logf.write(f"[3/4] Генерирую config_generated.h\n")
        logf.flush()
        lines = ["#pragma once"]
        for name, val in modules.items():
            lines.append(f"#define MODULE_{name} {1 if val else 0}")
        mod_json = json.dumps({k: (1 if v else 0) for k, v in modules.items()}, separators=(",", ":"))
        lines.append(f'#define FIRMWARE_VERSION "{version}"')
        lines.append(f'#define FIRMWARE_MODULES_JSON "{mod_json}"')
        (sketch_dir / "config_generated.h").write_text("\n".join(lines), encoding="utf-8")
        
        cmd = ["arduino-cli", "compile", "--fqbn", FQBN,
               "--output-dir", str(work / "out"), str(sketch_dir)]
        logf.write(f"[4/4] Запускаю: {' '.join(cmd)}\n\n")
        logf.flush()
        
        try:
            # Вывод идёт в файл, не в pipe → нет deadlock
            with open(log_path, "a", encoding="utf-8") as out:
                r = subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT, timeout=300)
        except FileNotFoundError:
            shutil.rmtree(work, ignore_errors=True)
            return {"ok": False, "error": "arduino-cli не найден"}
        except subprocess.TimeoutExpired:
            shutil.rmtree(work, ignore_errors=True)
            return {"ok": False, "error": "Компиляция > 5 минут"}
        
        with open(log_path, "a", encoding="utf-8") as logf:
            logf.write(f"\n=== Exit code: {r.returncode} ===\n")
        
        if r.returncode != 0:
            log_text = log_path.read_text(encoding="utf-8", errors="replace")
            shutil.rmtree(work, ignore_errors=True)
            return {"ok": False, "error": "Ошибка компиляции", "log": log_text[-5000:]}
        
        out_dir = work / "out"
        bin_file = None
        for p in out_dir.glob("*.ino.bin"):
            bin_file = p; break
        if not bin_file:
            for p in out_dir.glob("*.bin"):
                if "bootloader" not in p.name and "partitions" not in p.name:
                    bin_file = p; break
        if not bin_file:
            shutil.rmtree(work, ignore_errors=True)
            return {"ok": False, "error": "Нет .bin после компиляции"}
        
        data = bin_file.read_bytes()
        md5 = hashlib.md5(data).hexdigest()
        out_name = f"fw_{version}_{md5[:8]}.bin"
        (BUILD_DIR / out_name).write_bytes(data)
        shutil.rmtree(work, ignore_errors=True)
        
        return {"ok": True, "file": out_name, "size": len(data), "md5": md5, "log_file": str(log_path)}