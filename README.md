# 🎮 MINI-BOY — Portable ESP32-C3 Console

Welcome to the **MINI-BOY** repository — a custom handheld console built on the ESP32-C3 microcontroller. This project combines retro gaming, a smart ecosystem, and IoT capabilities.

## ✨ Features

*   **🕹️ 20+ Built-in Games:** from classics (Snake, Tetris, Breakout) to custom projects (Super Mario, Tanks).
*   **📺 128x64 OLED Display:** smooth graphics powered by the U8g2 library.
*   **💾 SD Card Support:** load dynamic apps (.mbg), save notes, change boot logos, and update firmware directly from the SD card.
*   **🌐 Local Web Panel:** access the console's file system and manage it via a smartphone browser (over Wi-Fi).
*   **🤖 AI Integration (Gemini):** ask artificial intelligence questions right from the console screen.
*   **🔋 Non-Volatile Memory:** saves high scores, sound settings, and button calibration.

## 🛠️ Hardware Requirements
*   **ESP32-C3** microcontroller
*   **SSD1306** display (I2C, 128x64)
*   **SD Card module** (SPI)
*   Piezo buzzer (for 8-bit music and sound effects)
*   6 tactile buttons (Up, Down, Left, Right, OK, Back)

## 🚀 Quick Start (Flashing)

1. Install **VS Code** and the **PlatformIO IDE** extension.
2. Open the irmware folder in VS Code.
3. Connect the ESP32-C3 via USB.
4. Press the **➡️ (Upload)** icon on the bottom PlatformIO panel to build and flash.
5. Done! The console will reboot and launch the main menu.

> 💡 **Note:** If you are flashing the board for the first time, don't forget to insert your GEMINI_API_KEY in main.cpp (or config_generated.h).

## 📂 Project Structure
*   irmware/ — PlatformIO firmware source code (C++).
*   docs/ — Additional instructions and documentation (PlatformIO setup).

## 📜 License
This project is distributed under the MIT License. You are free to modify, extend, and use the code for your own devices!

---
*Created with love for retro games and microcontrollers.* ❤️