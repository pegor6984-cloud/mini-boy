from fastapi import FastAPI, UploadFile, File, Form, Request, HTTPException, Body
from fastapi.responses import HTMLResponse, JSONResponse, FileResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
from starlette.concurrency import run_in_threadpool
from pydantic import BaseModel
import sqlite3, json, hashlib, socket, time, uuid, threading
from pathlib import Path
from datetime import datetime
import requests as req
import build as builder

DB = "store.db"
APP_DIR = Path("apps"); APP_DIR.mkdir(exist_ok=True)
FW_DIR = Path("firmware"); FW_DIR.mkdir(exist_ok=True)
BUILD_DIR = Path("builds"); BUILD_DIR.mkdir(exist_ok=True)
STATE_FILE = Path("build_state.json")

app = FastAPI(title="MINI-BOY Store")
app.mount("/static", StaticFiles(directory="static"), name="static")
tpl = Jinja2Templates(directory="templates")

ADMIN_PASS = "123"

# Загрузка/сохранение состояния сборок
build_tasks = {}
if STATE_FILE.exists():
    try:
        build_tasks = json.loads(STATE_FILE.read_text(encoding="utf-8"))
        print(f"[build] Загружено {len(build_tasks)} задач из build_state.json")
    except Exception as e:
        print(f"[build] Не удалось загрузить state: {e}")

def save_build_state():
    try:
        STATE_FILE.write_text(json.dumps(build_tasks, default=str), encoding="utf-8")
    except Exception as e:
        print(f"[build] Ошибка сохранения state: {e}")

def db():
    c = sqlite3.connect(DB); c.row_factory = sqlite3.Row; return c

def init_db():
    c = db()
    c.execute("""CREATE TABLE IF NOT EXISTS apps (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        slug TEXT UNIQUE, name TEXT, author TEXT, version TEXT,
        engine TEXT, description TEXT, tags TEXT,
        data_json TEXT DEFAULT '', filename TEXT DEFAULT '',
        md5 TEXT, size INTEGER DEFAULT 0,
        downloads INTEGER DEFAULT 0, created TEXT,
        type TEXT DEFAULT 'data'
    )""")
    c.execute("""CREATE TABLE IF NOT EXISTS devices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        mac TEXT UNIQUE, ip TEXT, version TEXT,
        modules_json TEXT, last_seen REAL
    )""")
    c.commit(); c.close()
init_db()

# ============ СТРАНИЦЫ ============
@app.get("/", response_class=HTMLResponse)
def index(request: Request):
    c = db()
    rows = c.execute("SELECT * FROM apps ORDER BY id DESC").fetchall()
    c.close()
    return tpl.TemplateResponse(request, "index.html", {"apps": rows})

@app.get("/admin", response_class=HTMLResponse)
def admin_page(request: Request):
    return tpl.TemplateResponse(request, "admin.html", {})

@app.get("/builder", response_class=HTMLResponse)
def builder_page(request: Request):
    modules = json.loads(Path("modules.json").read_text(encoding="utf-8-sig"))
    return tpl.TemplateResponse(request, "builder.html", {"modules": modules})

# ============ УСТРОЙСТВА ============
@app.post("/api/heartbeat")
async def heartbeat(payload: dict = Body(...)):
    ip = payload.get("ip",""); mac = payload.get("mac","")
    if not ip or not mac: raise HTTPException(400)
    c = db()
    c.execute("""INSERT INTO devices (mac, ip, version, modules_json, last_seen)
        VALUES (?, ?, ?, ?, ?)
        ON CONFLICT(mac) DO UPDATE SET ip=excluded.ip, version=excluded.version,
            modules_json=excluded.modules_json, last_seen=excluded.last_seen""",
        (mac, ip, payload.get("version",""), json.dumps(payload.get("modules",{})), time.time()))
    c.commit(); c.close()
    return {"ok": True}

@app.get("/api/devices")
def devices():
    c = db()
    rows = c.execute("SELECT * FROM devices").fetchall()
    c.close()
    now = time.time()
    return [{"id": r["id"], "ip": r["ip"], "mac": r["mac"], "version": r["version"],
             "modules": json.loads(r["modules_json"] or "{}")}
            for r in rows if (now - float(r["last_seen"])) < 40]

@app.get("/api/device_apps/{dev_id}")
async def device_apps(dev_id: int):
    c = db()
    d = c.execute("SELECT * FROM devices WHERE id=?", (dev_id,)).fetchone()
    c.close()
    if not d: raise HTTPException(404)
    def do_get():
        return req.get(f"http://{d['ip']}/api/apps/all", timeout=20)
    try:
        r = await run_in_threadpool(do_get)
        return r.json()
    except Exception as e:
        raise HTTPException(500, str(e))

@app.post("/api/push_app/{dev_id}")
async def push_app(dev_id: int, slug: str = Body(embed=True)):
    c = db()
    d = c.execute("SELECT * FROM devices WHERE id=?", (dev_id,)).fetchone()
    a = c.execute("SELECT data_json FROM apps WHERE slug=?", (slug,)).fetchone()
    c.close()
    if not d or not a: raise HTTPException(404)
    def do_post():
        return req.post(f"http://{d['ip']}/api/apps/install", data=a["data_json"], timeout=15)
    try:
        r = await run_in_threadpool(do_post)
        return {"ok": r.ok, "text": r.text}
    except Exception as e:
        raise HTTPException(500, str(e))

@app.post("/api/push_fw/{dev_id}")
async def push_fw(dev_id: int, filename: str = Body(embed=True)):
    c = db()
    d = c.execute("SELECT * FROM devices WHERE id=?", (dev_id,)).fetchone()
    c.close()
    if not d: raise HTTPException(404)
    p = BUILD_DIR / filename
    if not p.exists(): raise HTTPException(404, "File not found")
    def do_post():
        with open(p, "rb") as f:
            return req.post(f"http://{d['ip']}/api/firmware/install",
                          files={"update": (filename, f)}, timeout=180)
    try:
        r = await run_in_threadpool(do_post)
        return {"ok": r.ok, "text": r.text}
    except Exception as e:
        raise HTTPException(500, str(e))

# ============ СБОРКА ============
class BuildRequest(BaseModel):
    modules: dict
    version: str = "custom"

@app.post("/api/build/start")
async def api_build_start(payload: BuildRequest):
    task_id = str(uuid.uuid4())
    build_tasks[task_id] = {"status": "running", "started": datetime.now().isoformat(), "version": payload.version}
    save_build_state()
    
    print(f"\n[build] ===== СТАРТ СБОРКИ =====")
    print(f"[build] Task ID: {task_id}")
    print(f"[build] Version: {payload.version}")
    print(f"[build] Модули: {sum(1 for v in payload.modules.values() if v)} из {len(payload.modules)} включено")
    print(f"[build] Ждём ~60 секунд...\n")
    
    def do_build():
        try:
            result = builder.build(payload.modules, payload.version, task_id)
            if result["ok"]:
                build_tasks[task_id] = {"status":"done","file":result["file"],
                                        "size":result["size"],"md5":result["md5"]}
                print(f"[build] ✅ ГОТОВО: {result['file']} ({result['size']//1024} KB)\n")
            else:
                build_tasks[task_id] = {"status":"error","error":result.get("error"),
                                        "log":result.get("log","")}
                print(f"[build] ❌ ОШИБКА: {result.get('error')}")
                if result.get('log'):
                    print(f"[build] Лог: см. build_logs/{task_id}.log\n")
        except Exception as e:
            build_tasks[task_id] = {"status":"error","error":str(e)}
            print(f"[build] ❌ ИСКЛЮЧЕНИЕ: {e}\n")
        finally:
            save_build_state()
    
    threading.Thread(target=do_build, daemon=True).start()
    return {"task_id": task_id}

@app.get("/api/build/status/{task_id}")
def api_build_status(task_id: str):
    if task_id not in build_tasks: raise HTTPException(404)
    return build_tasks[task_id]

@app.get("/builds/{name}")
def get_build(name: str):
    p = BUILD_DIR / name
    if not p.exists() or ".." in name: raise HTTPException(404)
    return FileResponse(str(p), media_type="application/octet-stream", filename=name)

@app.get("/api/build/log/{task_id}")
def get_build_log(task_id: str):
    p = Path("build_logs") / f"{task_id}.log"
    if not p.exists(): raise HTTPException(404)
    return {"log": p.read_text(encoding="utf-8", errors="replace")}

# ============ КАТАЛОГ ============
@app.get("/api/apps")
def list_apps():
    c = db()
    rows = c.execute("SELECT slug,name,author,version,engine,description,type,size FROM apps ORDER BY id DESC").fetchall()
    c.close()
    return [dict(r) for r in rows]

@app.get("/download/{slug}.mbg")
def download_mbg(slug: str):
    c = db()
    row = c.execute("SELECT data_json FROM apps WHERE slug=? AND type='data'", (slug,)).fetchone()
    if not row: raise HTTPException(404)
    c.commit(); c.close()
    return JSONResponse(json.loads(row["data_json"]),
        headers={"Content-Disposition": f'attachment; filename="{slug}.mbg"'})

def _publish_raw(raw: str) -> dict:
    try: data = json.loads(raw)
    except Exception as e: raise HTTPException(400, f"Bad JSON: {e}")
    for k in ("name","engine","params"):
        if k not in data: raise HTTPException(400, f"Missing: {k}")
    slug = "".join(ch for ch in data["name"].lower().replace(" ","-") if ch.isalnum() or ch in "-_")[:40]
    md5 = hashlib.md5(raw.encode()).hexdigest()
    c = db()
    c.execute("""INSERT OR REPLACE INTO apps
        (slug,name,author,version,engine,description,tags,data_json,md5,size,created,type)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?)""",
        (slug, data["name"], data.get("author","anon"), data.get("version","1.0.0"),
         data["engine"], data.get("description",""), ",".join(data.get("tags",[])),
         raw, md5, len(raw.encode()), datetime.now().isoformat(timespec="seconds"), "data"))
    c.commit(); c.close()
    return {"ok": True, "slug": slug}

@app.post("/api/admin/publish")
async def publish(password: str = Form(...), mbg: UploadFile = File(...)):
    if password != ADMIN_PASS: raise HTTPException(401)
    return _publish_raw((await mbg.read()).decode("utf-8"))

@app.post("/api/admin/publish_text")
async def publish_text(password: str = Form(...), code: str = Form(...)):
    if password != ADMIN_PASS: raise HTTPException(401)
    return _publish_raw(code)

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)