import re, sys
from pathlib import Path

p = Path("sketch_src/MINI_BOY.ino")
src = p.read_text(encoding="utf-8")

if '#include "config_generated.h"' not in src:
    src = src.replace('#include "pq_types.h"', '#include "pq_types.h"\n#include "config_generated.h"', 1)

def wrap_function(text, func_name, macro):
    if f"#if {macro}" in text:
        return text
    pattern = re.compile(r'(void\s+' + func_name + r'\s*\(\s*\)\s*\{)')
    m = pattern.search(text)
    if not m:
        print(f"⚠ Не найдена функция {func_name}")
        return text
    start = m.start()
    i = m.end()
    depth = 1
    while i < len(text) and depth > 0:
        if text[i] == '{': depth += 1
        elif text[i] == '}': depth -= 1
        i += 1
    end = i
    return text[:start] + f"#if {macro}\n" + text[start:end] + f"\n#endif // {macro}" + text[end:]

src = wrap_function(src, "runDino", "MODULE_DINO")
src = wrap_function(src, "runSnake", "MODULE_SNAKE")

p.write_text(src, encoding="utf-8")
print("✅ MINI_BOY.ino патчен.")
