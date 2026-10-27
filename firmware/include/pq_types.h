#ifndef PQ_TYPES_H
#define PQ_TYPES_H
#include <Arduino.h>
 
enum BtnId : uint8_t { BI_UP, BI_DOWN, BI_LEFT, BI_RIGHT, BI_OK, BI_BACK, BI_COUNT };
enum AppId : uint8_t {
  APP_DINO, APP_FLAPPY, APP_TETRIS, APP_SNAKE, APP_PONG, APP_BRICK,
  APP_INVADE, APP_ASTER, APP_MINES, APP_2048, APP_SIMON, APP_FROGGER,
  APP_TTT, APP_GEMINI, APP_SNAKE_MAZE, APP_MEMORY, APP_SKATE, APP_TANK,
  APP_STARS, APP_REACTION, APP_LABYRINTH, APP_JUMPER, APP_PIXEL_QUEST,
  APP_NOTES,
  APP_SETTINGS, APP_COUNT
};
enum MusicTheme : uint8_t { THEME_NONE=0, THEME_TECH, THEME_CHILL, THEME_DARK, THEME_OLDSCHOOL, THEME_COUNT };
struct Note { uint8_t m; uint8_t t; };
struct Step { uint16_t f; uint16_t ms; };
struct Rock { float x, y, vx, vy; int8_t sz; bool on; };
struct Shot { float x, y, vx, vy; int life; bool on; };
struct BtnState { bool raw=false, stable=false, longDone=false; uint32_t changed=0, downAt=0, lastRep=0; };
struct NetInfo { String ssid; int rssi; bool secure; };
struct TetrisPiece { int type, x, y, rot; };
struct Settings {
  uint8_t bright=2; bool invert=false, showPass=false; uint8_t layout=0, rep=1;
  bool showFps=false, autoConn=true, sfx=true; uint8_t musicTheme=THEME_TECH, volPercent=50;
  uint8_t difficulty=1, animSpeed=1; bool showClock=true, clock24h=true, autoTime=true; uint8_t lang=0;
  bool showFileTypes=true, settingsWrap=true;
  // === КАЛИБРОВКА КНОПОК (сохраняем какой GPIO соответствует какой кнопке) ===
  uint8_t btnGpioUp=5, btnGpioDown=7, btnGpioLeft=3, btnGpioRight=1, btnGpioOk=2, btnGpioBack=6;
};
 
#endif