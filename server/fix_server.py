import re

with open("server.py", "r", encoding="utf-8") as f:
    c = f.read()

# 1. Чиним склеенную строку
c = c.replace("port=8000)from", "port=8000)\nfrom")

# 2. Удаляем старый блок __main__ (он застрял посреди файла)
c = re.sub(r'\nif __name__ == .__main__.:\n(    .*\n)+', '\n', c)

# 3. Чистим и добавляем блок __main__ в самый конец
c = c.rstrip() + '\n'
c += '\nif __name__ == "__main__":\n    import uvicorn\n    uvicorn.run(app, host="0.0.0.0", port=8000)\n'

with open("server.py", "w", encoding="utf-8") as f:
    f.write(c)

print("server.py успешно исправлен")
