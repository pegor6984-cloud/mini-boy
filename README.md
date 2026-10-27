<div align="center">

```
    ███╗   ███╗██╗███╗   ██╗██╗    ██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██║████╗  ██║██║    ██╔══██╗██╔═══██╗╚██╗ ██╔╝
    ██╔████╔██║██║██╔██╗ ██║██║    ██████╔╝██║   ██║ ╚████╔╝ 
    ██║╚██╔╝██║██║██║╚██╗██║██║    ██╔══██╗██║   ██║  ╚██╔╝  
    ██║ ╚═╝ ██║██║██║ ╚████║██║    ██████╔╝╚██████╔╝   ██║   
    ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝    ╚═════╝  ╚═════╝    ╚═╝   
```

### ESP32-C3 handheld gaming console

**25+ games · AI integration · web panel · SD-card store · custom engines**

</div>

---

## ✨ Features

- 🎮 **25+ built-in games** — Dino, Snake, Tetris, Mario, Asteroids, and more
- 🧠 **AI integration** — ask Gemini anything from the device
- 📝 **Notes app** — save AI answers as notes on SD card
- 📁 **SD file manager** — browse, upload, delete files from web panel
- 🎨 **Custom engines** — create new games with JSON, no recompile
- 📡 **Web control panel** — control device from phone
- 🕹️ **Dynamic icon editor** — design icons in JSON
- 🕒 **Real-time clock** — DS3231 with NTP sync

## 🔧 Hardware

| Component | GPIO | Note |
|-----------|------|------|
| OLED SDA | 8 | I2C bus |
| OLED SCL | 9 | I2C bus |
| Buttons | 3,1,5,7,2,6 | LEFT/RIGHT/UP/DOWN/OK/BACK |
| SD CS | 4 | SPI |
| SD SCK | 0 | SPI |
| SD MOSI | 20 | SPI |
| SD MISO | 21 | SPI |
| Buzzer | 10 | PWM |
| RTC DS3231 | 8/9 | shared I2C |

**Display:** SSD1306 128×64 OLED  
**MCU:** ESP32-C3 (4 MB Flash, 400 KB RAM)  
**Framework:** Arduino-ESP32 core 3.3.12

## 🚀 Installation

### 1. Configure

Edit `firmware/MINI_BOY.ino`:

```cpp
const char* GEMINI_API_KEY = "YOUR_GEMINI_API_KEY_HERE";
const char* AP_PASS = "YOUR_AP_PASSWORD";
```

### 2. Build & upload (Arduino IDE)

1. Install **ESP32 by Espressif** from Boards Manager
2. Board: **ESP32C3 Dev Module**
3. Partition Scheme: **Minimal SPIFFS (1.9MB APP with OTA)**
4. Upload

### 3. First launch

1. Connect to WiFi **MINI-BOY-FILES**
2. Open `http://192.168.4.1` on phone

## 🎯 Usage

| Button | Action |
|--------|--------|
| LEFT / RIGHT | Switch game |
| OK | Launch game |
| Hold OK (2s) | Documentation |
| BACK | Language toggle (EN/RU/UA) |
| Hold BACK (2.5s) | Game options / FM settings |

### Dynamic games (SD-card)

Games are JSON files (`*.mbg`) on SD card `/apps/`. No recompile needed.

**Engines:** `snake`, `runner`, `flappy`, `breakout`, `maze`, `jumper`, `invaders`, `pong`, `calculator`, `app`, `text`, `quiz`, `todo`, `gemini`, `sequence`

```json
{
  "format": "mbg-1",
  "name": "Snake Cyber",
  "engine": "snake",
  "icon": [["box",-8,-2,5,4],["box",-2,-2,5,4],["box",4,-2,5,4]],
  "params": { "speed_ms": 120, "map": ".##......##....." }
}
```

## 📄 License

MIT — see [LICENSE](LICENSE).

## 🙏 Credits

- [U8g2](https://github.com/olikraus/u8g2) — OLED graphics
- [ArduinoJson](https://arduinojson.org/) — JSON parsing
- [ESP32 core](https://github.com/espressif/arduino-esp32) — framework

---

<div align="center">

**⭐ Star this repo if you like it!**

Made with ❤️ for ESP32-C3

</div>