# MINI_BOY Workspace

## firmware/ — PlatformIO project (ESP32-C3)
- src/main.cpp — main sketch
- include/ — headers
- platformio.ini — board config

**Commands (in firmware/):**
- pio run — build
- pio run -t upload — flash
- pio device monitor — serial

## server/ — Python FastAPI server
- server.py — main
- templates/, static/ — HTML/CSS

**Commands (in server/):**
- pip install fastapi uvicorn requests
- python server.py

## Git workflow (VS Code)
1. Left sidebar → Source Control (Ctrl+Shift+G)
2. See each changed file
3. Click file → see diff
4. Click "+" next to file → stage
5. Type message → Ctrl+Enter → commit
6. "..." menu → Push

**Never use "Stage All"** unless you want everything at once.