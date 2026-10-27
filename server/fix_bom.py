from pathlib import Path
p = Path("server.py")
c = p.read_text(encoding="utf-8")
c = c.replace('.read_text(encoding="utf-8")', '.read_text(encoding="utf-8-sig")')
p.write_text(c, encoding="utf-8")
print("✅ server.py исправлен — BOM теперь игнорируется")
