// ║    ███╗   ███╗██╗███╗   ██╗██╗    ██████╗  ██████╗ ██╗   ██╗   ║
// ║    ████╗ ████║██║████╗  ██║██║    ██╔══██╗██╔═══██╗╚██╗ ██╔╝   ║
// ║    ██╔████╔██║██║██╔██╗ ██║██║    ██████╔╝██║   ██║ ╚████╔╝    ║
// ║    ██║╚██╔╝██║██║██║╚██╗██║██║    ██╔══██╗██║   ██║  ╚██╔╝     ║
// ║    ██║ ╚═╝ ██║██║██║ ╚████║██║    ██████╔╝╚██████╔╝   ██║      ║
// ║    ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝    ╚═════╝  ╚═════╝    ╚═╝      ║
// ⚠️ ВАЖНО: перед сборкой замени GEMINI_API_KEY на свой.
// Получить: https://aistudio.google.com/apikey
#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include "pq_types.h"
#include <Update.h>
#include "mbedtls/base64.h"
#if __has_include("config_generated.h")
  #include "config_generated.h"
#else
  // Если файл не сгенерирован сервером, включаем всё по умолчанию.
  #ifndef MODULE_DINO
  #define MODULE_DINO 1
  #define MODULE_FLAPPY 1
  #define MODULE_TETRIS 1
  #define MODULE_SNAKE 1
  #define MODULE_PONG 1
  #define MODULE_BRICK 1
  #define MODULE_INVADE 1
  #define MODULE_ASTER 1
  #define MODULE_MINES 1
  #define MODULE_2048 1
  #define MODULE_SIMON 1
  #define MODULE_FROGGER 1
  #define MODULE_TTT 1
  #define MODULE_GEMINI 1
  #define MODULE_SNAKE_MAZE 1
  #define MODULE_MEMORY 1
  #define MODULE_SKATE 1
  #define MODULE_TANK 1
  #define MODULE_STARS 1
  #define MODULE_REACTION 1
  #define MODULE_LABYRINTH 1
  #define MODULE_JUMPER 1
  #define MODULE_MARIO 1
  #define MODULE_NOTES 1
  #define MODULE_SD_FILES 1
  #define MODULE_WEB_PANEL 1
  #endif
  #ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "dev"
  #endif
  #ifndef FIRMWARE_MODULES_JSON
  #define FIRMWARE_MODULES_JSON "{}"
  #endif
#endif
#define DYNAMIC_APP_ID_BASE (APP_COUNT + 1)
struct MbApp {
  String slug;
  String name;
  String engine;
  String path;
  bool ok;
  uint8_t iconOpCount;
  struct { uint8_t op; int8_t p[4]; } icon[12];
};
MbApp mbApps[16];
int mbAppCount = 0;
#define MB_APPS_DIR "/apps"

const char* GEMINI_API_KEY = "YOUR_GEMINI_API_KEY_HERE";
const char* API_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent";

#define OLED_SDA 8
#define OLED_SCL 9
#define SD_CS_PIN 4
#define SD_SCK_PIN 0
#define SD_MOSI_PIN 20
#define SD_MISO_PIN 21
// Legacy joystick routines remain in the source but are disabled; these aliases
// keep those dormant routines compilable. These pins are now assigned to SD.
#define JOY_X_PIN SD_CS_PIN
#define JOY_Y_PIN SD_MOSI_PIN
#define JOY_SW_PIN SD_SCK_PIN
#define JOY_DEADZONE 400
#define JOY_HYSTERESIS 100
#define ENABLE_SERIAL 0
#define BUZ_PIN  10
#define CHARGE_STATUS_PIN (-1)
#define CHARGE_ACTIVE_LOW 1

// Add File Manager as the 25th main-menu app without changing existing app IDs.
#define APP_FILE_MANAGER APP_COUNT
int menuAppCount = APP_COUNT + 1; // Динамический счетчик

uint8_t BTN_PIN[BI_COUNT] = {5, 7, 3, 1, 2, 6};
const int REC_N = 23;
const int SET_N = 35;
// === Заводские GPIO для кнопок ===
const uint8_t DEFAULT_BTN_GPIO[BI_COUNT] = {5, 7, 3, 1, 2, 6};
// Порядок: BI_UP=0, BI_DOWN=1, BI_LEFT=2, BI_RIGHT=3, BI_OK=4, BI_BACK=5
const int CAR_SPACING = 58;
const int GROUND_Y = 56;

Settings cfg;

Rock rocks[16];
Shot shots[5];
BtnState bst[BI_COUNT];
bool evDown[BI_COUNT], evTap[BI_COUNT], evRep[BI_COUNT], evLong[BI_COUNT];
NetInfo nets[16];
int netCount = 0;
// ===== ПРОТОТИПЫ ФУНКЦИЙ (чтобы Arduino IDE не ругалась) =====
void applyDisplay();
void saveCfg();
void runWifiScan();
void reconnectSaved();
void forgetNetwork();
void editClockManually();
void syncClockNow();
void runRecords();
void resetScores();
void showSystemInfo();
void showAbout();
void scanMbApps();
String chooseSdFile();
void waitAnyKey();
void runSdFirmwareUpdate();
void runStore();
void editStoreUrl();
void runDynamicApp(int idx);
void runDynCalculator(JsonDocument& doc);
void runDynMiniApp(JsonDocument& doc);
void runDynText(JsonDocument& doc);
void runDynQuiz(JsonDocument& doc);
void runDynTodo(JsonDocument& doc);
void runDynGemini(JsonDocument& doc);
void runNotes();
void showGameDocs(int id);
void runSettings();
void runLocalFileManager();
void runAppOptions(int id);
void runFmSettings();
void present();
void showScreen(const char* title, const String& a, const String& b = "", const String& c = "");
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
Preferences prefs;

// ================= SD FILE MANAGER + PHONE WEB PANEL =================
// Pin mapping candidate: CS=4, SCK=0, MOSI=20, MISO=21. Confirm board pinout
// and boot/USB behavior before wiring. GPIO0 is a strapping pin on many ESP32-C3 boards.
WebServer fileWeb(80);
DNSServer captiveDns;
const byte DNS_PORT = 53;
SPIClass sdSPI(FSPI);
bool sdReady = false;
bool bootLogoFromSD = false;
String fmPath = "/";
String pendingAttachPath = "";
int kbActionCount = 5;
File webUploadFile;
int pendingRemoteButton = -1; // one-shot input from the browser dashboard
bool remoteButtonHeld[BI_COUNT] = {false,false,false,false,false,false};
bool remoteOkReleasePending = false;
bool suppressSystemOverlay = false; // keep splash/bitmap previews clean
bool webUploadOk = false;
uint32_t sdChangeGeneration = 0;
size_t webUploadBytes = 0;
String webUploadDest;
static const char* AP_NAME = "MINI-BOY-FILES";
static const char* PANEL_HOST = "miniboy"; // open http://miniboy.local when mDNS is supported
static const char* AP_PASS = "YOUR_AP_PASSWORD"; // Change before using outside a trusted environment.
bool otaInProgress = false;
#define FIRMWARE_VERSION "5.1.0"

String htmlEscape(const String &s) {
  String o; o.reserve(s.length()+8);
  for (size_t i=0;i<s.length();++i) {
    char c=s[i]; if(c=='&') o+="&amp;"; else if(c=='<') o+="&lt;";
    else if(c=='>') o+="&gt;"; else if(c=='\\') o+="&#92;"; else if(c=='\"') o+="&quot;"; else o+=c;
  } return o;
}
String cleanPath(String p) {
  p.replace("\\", "/"); if(!p.startsWith("/")) p="/"+p;
  while(p.indexOf("//")>=0) p.replace("//","/");
  // Reject traversal rather than trying to normalize it.
  if(p.indexOf("..")>=0) return String();
  if(p.length()>1 && p.endsWith("/")) p.remove(p.length()-1);
  return p;
}
String parentPath(String p) { p=cleanPath(p); if(p.length()<=1) return "/"; int i=p.lastIndexOf('/'); return i<=0?"/":p.substring(0,i); }
String baseName(String p) { int i=p.lastIndexOf('/'); return i<0?p:p.substring(i+1); }
bool copyFilePath(const String& from, const String& to) {
  File in=SD.open(from, FILE_READ); if(!in || in.isDirectory()) { if(in) in.close(); return false; }
  File out=SD.open(to, FILE_WRITE); if(!out) { in.close(); return false; }
  uint8_t buf[512]; bool ok=true;
  while(in.available()) { int n=in.read(buf,sizeof(buf)); if(n<=0) break; if(out.write(buf,n)!=(size_t)n) { ok=false; break; } }
  out.flush(); out.close(); in.close(); if(!ok) SD.remove(to); return ok;
}
bool removeTree(const String& p) {
  File f=SD.open(p); if(!f) return false;
  if(!f.isDirectory()) { f.close(); return SD.remove(p); }
  File child=f.openNextFile();
  while(child) { String n=String(child.name()); bool dir=child.isDirectory(); child.close(); String full=n.startsWith("/")?n:(p=="/"?"/"+n:p+"/"+n); if(dir) removeTree(full); else SD.remove(full); child=f.openNextFile(); }
  f.close(); return SD.rmdir(p);
}
String listJson(const String& raw) {
  String p=cleanPath(raw); if(!p.length()) return "[]";
  File dir=SD.open(p); if(!dir || !dir.isDirectory()) { if(dir)dir.close(); return "[]"; }
  String out="["; bool first=true; File f=dir.openNextFile(); int count=0;
  while(f && count<256) {
    String nm=String(f.name()); String full=nm.startsWith("/")?nm:(p=="/"?"/"+nm:p+"/"+nm);
    if(!first) out+=","; first=false; out+="{\"name\":\"";
    String safe=nm; safe.replace("\\","\\\\"); safe.replace("\"","\\\"");
    out+=safe+"\",\"path\":\""; String sf=full; sf.replace("\\","\\\\"); sf.replace("\"","\\\"");
    out+=sf+"\",\"dir\":"+(f.isDirectory()?"true":"false")+",\"size\":"+String((uint32_t)f.size())+"}";
    f.close(); f=dir.openNextFile(); count++;
  }
  if(f) f.close(); dir.close(); out+="]"; return out;
}
#include "MINI_BOY_paneldata_k8r4m2.h"
bool fmCopyPathRecursive(const String& from, const String& to);
const char* moduleIdForApp(int appId) {
  switch (appId) {
    case APP_DINO: return "DINO";
    case APP_FLAPPY: return "FLAPPY";
    case APP_TETRIS: return "TETRIS";
    case APP_SNAKE: return "SNAKE";
    case APP_PONG: return "PONG";
    case APP_BRICK: return "BRICK";
    case APP_INVADE: return "INVADE";
    case APP_ASTER: return "ASTER";
    case APP_MINES: return "MINES";
    case APP_2048: return "2048";
    case APP_SIMON: return "SIMON";
    case APP_FROGGER: return "FROGGER";
    case APP_TTT: return "TTT";
    case APP_GEMINI: return "GEMINI";
    case APP_SNAKE_MAZE: return "SNAKE_MAZE";
    case APP_MEMORY: return "MEMORY";
    case APP_SKATE: return "SKATE";
    case APP_TANK: return "TANK";
    case APP_STARS: return "STARS";
    case APP_REACTION: return "REACTION";
    case APP_LABYRINTH: return "LABYRINTH";
    case APP_JUMPER: return "JUMPER";
    case APP_PIXEL_QUEST: return "MARIO";
    case APP_NOTES: return "NOTES";
    case APP_SETTINGS: return "SETTINGS";
    default: return nullptr;
  }
}
void setupFileWeb() {
  sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  sdReady=SD.begin(SD_CS_PIN,sdSPI,10000000);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  IPAddress apIP(192,168,4,1), gateway(192,168,4,1), subnet(255,255,255,0);
  WiFi.softAPConfig(apIP,gateway,subnet);
  bool apStarted=WiFi.softAP(AP_NAME,AP_PASS,6,false,4);
  delay(250);
  if(!apStarted){WiFi.softAPdisconnect(true);WiFi.mode(WIFI_AP);WiFi.softAPConfig(apIP,gateway,subnet);apStarted=WiFi.softAP(AP_NAME,AP_PASS);}

  if (MDNS.begin(PANEL_HOST)) MDNS.addService("http", "tcp", 80);
  captiveDns.start(DNS_PORT, "*", WiFi.softAPIP());
  auto panelRoot = [](){ fileWeb.sendHeader("Cache-Control", "no-store"); fileWeb.send_P(200,"text/html; charset=utf-8",FILE_PANEL); };
  
  fileWeb.on("/", HTTP_GET, panelRoot);
  fileWeb.on("/health", HTTP_GET, [](){ fileWeb.send(200,"text/plain",String("MINI-BOY web service OK; AP=")+WiFi.softAPIP().toString()+"; SD="+(sdReady?"OK":"NO")); });
  
  fileWeb.on("/api/bootlogo", HTTP_GET, [](){ 
    String mode=fileWeb.arg("mode"); 
    if(mode=="sd"){ 
      File f=sdReady?SD.open("/bootlogo.raw",FILE_READ):File(); 
      if(!f || f.isDirectory() || f.size()!=1024){if(f)f.close();fileWeb.send(404,"text/plain","Нужен /bootlogo.raw размером 1024 байта");return;} 
      f.close();bootLogoFromSD=true; 
    } else bootLogoFromSD=false; 
    prefs.putBool("bootlogo_sd",bootLogoFromSD); 
    fileWeb.send(200,"text/plain",bootLogoFromSD?"Boot logo: SD card":"Boot logo: default"); 
  });
  
  fileWeb.on("/api/display", HTTP_GET, [](){
    fileWeb.setContentLength(1024);
    fileWeb.send(200, "application/octet-stream", "");
    fileWeb.sendContent((const char*)oled.getBufferPtr(), 1024);
  });
  
  fileWeb.on("/api/control", HTTP_GET, [](){
    String key=fileWeb.arg("key");
    int id=-1;
    if(key=="left") id=BI_LEFT; else if(key=="right") id=BI_RIGHT;
    else if(key=="up") id=BI_UP; else if(key=="down") id=BI_DOWN;
    else if(key=="ok") id=BI_OK; else if(key=="back") id=BI_BACK;
    if(id<0){fileWeb.send(400,"text/plain","Unknown key");return;}
    String state=fileWeb.arg("state");
    if(state=="down") remoteButtonHeld[id]=true;
    else if(state=="up") { remoteButtonHeld[id]=false; if(id==BI_OK) remoteOkReleasePending=true; }
    else pendingRemoteButton=id;
    fileWeb.send(200,"text/plain","OK");
  });
  
  fileWeb.on("/api/modules", HTTP_GET, [](){
    fileWeb.sendHeader("Access-Control-Allow-Origin", "*");
    fileWeb.send(200, "application/json", "{\"version\":\"" FIRMWARE_VERSION "\",\"modules\":" FIRMWARE_MODULES_JSON "}");
  });
  
  fileWeb.on("/api/apps/list", HTTP_GET, [](){
  if (!sdReady) { fileWeb.send(200, "application/json", "[]"); return; }
  String out = "[";
  bool first = true;
  File dir = SD.open("/apps");
  if (dir) {
    File f = dir.openNextFile();
    while (f) {
      String nm = String(f.name());
      String low = nm; low.toLowerCase();
      if (!f.isDirectory() && low.endsWith(".mbg")) {
        if (!first) out += ",";
        first = false;
        out += "{\"name\":\"" + nm + "\",\"size\":" + String(f.size()) + "}";
      }
      f.close();
      f = dir.openNextFile();
    }
    if (f) f.close();
    dir.close();
  }
  out += "]";
  fileWeb.send(200, "application/json", out);
});

fileWeb.on("/api/apps/delete", HTTP_GET, [](){
  String nm = fileWeb.arg("name");
  if (!sdReady || !nm.length()) { fileWeb.send(400, "text/plain", "Bad request"); return; }
  String path = "/apps/" + nm;
  if (SD.exists(path) && SD.remove(path)) {
    fileWeb.send(200, "text/plain", "Deleted " + nm);
  } else {
    fileWeb.send(404, "text/plain", "Not found");
  }
});

  fileWeb.on("/api/settings", HTTP_GET, [](){
    bool changed=false;
    if(fileWeb.hasArg("brightness")){cfg.bright=(uint8_t)constrain(fileWeb.arg("brightness").toInt(),0,3);applyDisplay();changed=true;}
    if(fileWeb.hasArg("invert")){cfg.invert=fileWeb.arg("invert")=="1";applyDisplay();changed=true;}
    if(fileWeb.hasArg("sound")){cfg.sfx=fileWeb.arg("sound")=="1";changed=true;}
    if(fileWeb.hasArg("showPass")){cfg.showPass=fileWeb.arg("showPass")=="1";changed=true;}
    if(fileWeb.hasArg("layout")){cfg.layout=(uint8_t)constrain(fileWeb.arg("layout").toInt(),0,2);changed=true;}
    if(fileWeb.hasArg("rep")){cfg.rep=(uint8_t)constrain(fileWeb.arg("rep").toInt(),0,2);changed=true;}
    if(fileWeb.hasArg("theme")){cfg.musicTheme=(uint8_t)constrain(fileWeb.arg("theme").toInt(),0,THEME_COUNT-1);changed=true;}
    if(fileWeb.hasArg("volume")){cfg.volPercent=(uint8_t)constrain(fileWeb.arg("volume").toInt(),0,100);changed=true;}
    if(fileWeb.hasArg("fps")){cfg.showFps=fileWeb.arg("fps")=="1";changed=true;}
    if(fileWeb.hasArg("autoConn")){cfg.autoConn=fileWeb.arg("autoConn")=="1";changed=true;}
    if(fileWeb.hasArg("difficulty")){cfg.difficulty=(uint8_t)constrain(fileWeb.arg("difficulty").toInt(),0,2);changed=true;}
    if(fileWeb.hasArg("animSpeed")){cfg.animSpeed=(uint8_t)constrain(fileWeb.arg("animSpeed").toInt(),0,2);changed=true;}
    if(fileWeb.hasArg("showClock")){cfg.showClock=fileWeb.arg("showClock")=="1";changed=true;}
    if(fileWeb.hasArg("clock24h")){cfg.clock24h=fileWeb.arg("clock24h")=="1";changed=true;}
    if(fileWeb.hasArg("autoTime")){cfg.autoTime=fileWeb.arg("autoTime")=="1";changed=true;}
    if(fileWeb.hasArg("lang")){cfg.lang=(uint8_t)constrain(fileWeb.arg("lang").toInt(),0,2);changed=true;}
    if(fileWeb.hasArg("showFileTypes")){cfg.showFileTypes=fileWeb.arg("showFileTypes")=="1";changed=true;}
    if(fileWeb.hasArg("settingsWrap")){cfg.settingsWrap=fileWeb.arg("settingsWrap")=="1";changed=true;}
    if(changed) saveCfg();
    String json=String("{\"brightness\":")+String(cfg.bright)+",\"invert\":"+(cfg.invert?"true":"false")+",\"sound\":"+(cfg.sfx?"true":"false")+
      ",\"showPass\":"+(cfg.showPass?"true":"false")+",\"layout\":"+String(cfg.layout)+",\"rep\":"+String(cfg.rep)+
      ",\"theme\":"+String(cfg.musicTheme)+",\"volume\":"+String(cfg.volPercent)+",\"fps\":"+(cfg.showFps?"true":"false")+
      ",\"autoConn\":"+(cfg.autoConn?"true":"false")+",\"difficulty\":"+String(cfg.difficulty)+",\"animSpeed\":"+String(cfg.animSpeed)+
      ",\"showClock\":"+(cfg.showClock?"true":"false")+",\"clock24h\":"+(cfg.clock24h?"true":"false")+
      ",\"autoTime\":"+(cfg.autoTime?"true":"false")+",\"lang\":"+String(cfg.lang)+
      ",\"showFileTypes\":"+(cfg.showFileTypes?"true":"false")+",\"settingsWrap\":"+(cfg.settingsWrap?"true":"false")+
      ",\"sd\":"+(sdReady?"true":"false")+",\"bootlogo\":\""+(bootLogoFromSD?"sd":"default")+"\"}";
    fileWeb.send(200,"application/json",json);
  });
  
  fileWeb.on("/api/settings/action", HTTP_GET, [](){
    String a=fileWeb.arg("action");
    if(a=="scan") runWifiScan();
    else if(a=="reconnect") reconnectSaved();
    else if(a=="forget") forgetNetwork();
    else if(a=="setTime") editClockManually();
    else if(a=="syncTime") syncClockNow();
    else if(a=="records") runRecords();
    else if(a=="resetScores") resetScores();
    else if(a=="systemInfo") showSystemInfo();
    else if(a=="about") showAbout();
    else if(a=="sdStatus") showScreen("SD card status",sdReady?"SD ready":"SD not detected","Wi-Fi: MINI-BOY-FILES","192.168.4.1");
    else if(a=="bootLogo"){bootLogoFromSD=!bootLogoFromSD;prefs.putBool("bootlogo_sd",bootLogoFromSD);}
    else {fileWeb.send(400,"text/plain","Unknown settings action");return;}
    fileWeb.send(200,"text/plain","Выполнено: "+a);
  });
  
  fileWeb.on("/api/setbootlogo", HTTP_GET, [](){
    String src=cleanPath(fileWeb.arg("path"));
    if(!sdReady || !src.length()){fileWeb.send(400,"text/plain","Неверный путь или SD недоступна");return;}
    File f=SD.open(src,FILE_READ);
    if(!f || f.isDirectory() || f.size()!=1024){if(f)f.close();fileWeb.send(415,"text/plain","Нужен RAW-файл 128x64, ровно 1024 байта");return;}
    f.close();
    bool ok=true;
    if(src!="/bootlogo.raw") { if(SD.exists("/bootlogo.raw")) SD.remove("/bootlogo.raw"); ok=copyFilePath(src,"/bootlogo.raw"); }
    if(ok){bootLogoFromSD=true;prefs.putBool("bootlogo_sd",true);fileWeb.send(200,"text/plain","Заставка сохранена. Перезапусти MINI-BOY.");}
    else fileWeb.send(500,"text/plain","Не удалось записать /bootlogo.raw");
  });
  
  fileWeb.on("/api/view", HTTP_GET, [](){
    String p=cleanPath(fileWeb.arg("path")); String low=p; low.toLowerCase();
    if(!sdReady || !p.length()){fileWeb.send(400,"text/plain","Bad path");return;}
    File f=SD.open(p,FILE_READ); if(!f || f.isDirectory()){if(f)f.close();fileWeb.send(404,"text/plain","Not found");return;}
    const char* mime="application/octet-stream";
    if(low.endsWith(".png"))mime="image/png";else if(low.endsWith(".jpg")||low.endsWith(".jpeg"))mime="image/jpeg";else if(low.endsWith(".gif"))mime="image/gif";else if(low.endsWith(".bmp"))mime="image/bmp";else if(low.endsWith(".svg"))mime="image/svg+xml";else if(low.endsWith(".txt")||low.endsWith(".ino")||low.endsWith(".h")||low.endsWith(".cpp")||low.endsWith(".json")||low.endsWith(".cfg"))mime="text/plain; charset=utf-8";
    fileWeb.streamFile(f,mime); f.close();
  });
  
  fileWeb.on("/generate_204", HTTP_GET, panelRoot);
  fileWeb.on("/hotspot-detect.html", HTTP_GET, panelRoot);
  fileWeb.on("/connecttest.txt", HTTP_GET, panelRoot);
  fileWeb.on("/fwlink", HTTP_GET, panelRoot);
  
  fileWeb.onNotFound([panelRoot](){
    if (fileWeb.uri().startsWith("/api/")) fileWeb.send(404,"text/plain; charset=utf-8","API endpoint not found");
    else panelRoot();
  });
  
  fileWeb.on("/api/list",HTTP_GET,[]{ if(!sdReady){fileWeb.send(503,"application/json","[]");return;} fileWeb.send(200,"application/json",listJson(fileWeb.arg("path"))); });
  
  fileWeb.on("/api/op",HTTP_GET,[]{
    if(!sdReady){fileWeb.send(503,"text/plain","SD недоступна");return;}
    String op=fileWeb.arg("op"), p=cleanPath(fileWeb.arg("path")), to=cleanPath(fileWeb.arg("to")); if(!p.length() || p=="/"){fileWeb.send(400,"text/plain","Недопустимый путь");return;}
    bool ok=false;
    if(op=="mkdir" && p.length() && p!="/") ok=SD.mkdir(p);
    else if(op=="rename" && to.length() && to!="/" && !SD.exists(to)) ok=SD.rename(p,to);
    else if(op=="copy" && to.length() && !SD.exists(to)) ok=fmCopyPathRecursive(p,to);
    else if(op=="move" && to.length() && !SD.exists(to)) { ok=SD.rename(p,to); if(!ok) { File src=SD.open(p,FILE_READ); bool isFile=src && !src.isDirectory(); if(src) src.close(); if(p!="/" && fmCopyPathRecursive(p,to)) { ok=removeTree(p); if(!ok) removeTree(to); } } }
    else if(op=="delete") ok=removeTree(p);
    if(ok) sdChangeGeneration++;
    fileWeb.send(ok?200:400,"text/plain; charset=utf-8",ok?"OK":"Операция не выполнена (проверь путь/тип файла)");
  });
  
  fileWeb.on("/api/update", HTTP_POST, [](){
    if (Update.hasError()) fileWeb.send(500, "text/plain", String("OTA failed: ") + Update.errorString());
    else { fileWeb.send(200, "text/plain", "OTA OK! Rebooting..."); delay(1000); ESP.restart(); }
  }, [](){
    HTTPUpload& upload = fileWeb.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (!upload.filename.endsWith(".bin")) { Update.abort(); return; }
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });
  
  fileWeb.on("/api/firmware/install", HTTP_POST, [](){
    if (Update.hasError()) fileWeb.send(500, "text/plain", String("OTA failed: ") + Update.errorString());
    else { fileWeb.send(200, "text/plain", "OTA OK! Rebooting..."); delay(1000); ESP.restart(); }
  }, [](){
    HTTPUpload& upload = fileWeb.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (!upload.filename.endsWith(".bin")) { Update.abort(); return; }
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });
  
  fileWeb.on("/api/ota", HTTP_POST, [](){
    if (Update.hasError()) fileWeb.send(500, "text/plain", "OTA failed: " + String(Update.errorString()));
    else { fileWeb.send(200, "text/plain", "OTA OK! Rebooting..."); delay(1000); ESP.restart(); }
  }, [](){
    HTTPUpload& upload = fileWeb.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (!upload.filename.endsWith(".bin")) { Update.abort(); return; }
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_END) {
      Update.end(true);
    }
  });
  
  fileWeb.on("/api/download",HTTP_GET,[]{
    String p=cleanPath(fileWeb.arg("path")); 
    if(!sdReady||!p.length()){fileWeb.send(400,"text/plain","Bad path");return;} 
    File f=SD.open(p,FILE_READ); 
    if(!f||f.isDirectory()){if(f)f.close();fileWeb.send(404,"text/plain","Not found");return;} 
    fileWeb.sendHeader("Content-Disposition","attachment; filename=\""+baseName(p)+"\""); 
    fileWeb.streamFile(f,"application/octet-stream"); f.close();
  });
  
  fileWeb.on("/api/preview",HTTP_GET,[]{
    String p=cleanPath(fileWeb.arg("path")); String low=p; low.toLowerCase(); 
    if(!sdReady||!p.length()){fileWeb.send(400,"text/plain","Bad path");return;} 
    if(!(low.endsWith(".txt")||low.endsWith(".ino")||low.endsWith(".h")||low.endsWith(".cpp")||low.endsWith(".json")||low.endsWith(".cfg"))){fileWeb.send(415,"text/plain","Текстовый предпросмотр поддерживает TXT/INO/H/CPP/JSON/CFG. Для PNG/JPG нужен отдельный декодер.");return;} 
    File f=SD.open(p,FILE_READ); 
    if(!f||f.isDirectory()){if(f)f.close();fileWeb.send(404,"text/plain","Not found");return;} 
    fileWeb.streamFile(f,"text/plain; charset=utf-8"); f.close();
  });

  fileWeb.on("/api/apps/install", HTTP_POST, [](){
    if (!sdReady) { fileWeb.send(503, "text/plain", "SD missing"); return; }
    if (!SD.exists("/apps")) SD.mkdir("/apps");
    String body = fileWeb.arg("plain");
    if (body.length() == 0) { fileWeb.send(400, "text/plain", "Empty body"); return; }
    
    String name = "app_" + String(millis()) + ".mbg";
    int np = body.indexOf("\"name\":");
    if (np >= 0) {
      int q1 = body.indexOf('"', np + 7);
      int q2 = body.indexOf('"', q1 + 1);
      if (q1 >= 0 && q2 > q1) {
        String pretty = body.substring(q1 + 1, q2);
        pretty.replace(" ", "_");
        name = pretty + ".mbg";
      }
    }
    File f = SD.open(String("/apps/") + name, FILE_WRITE);
    if (f) {
      f.print(body);
      f.close();
      scanMbApps();
      fileWeb.send(200, "text/plain", "Installed as " + name + ". Restart MINI-BOY.");
    } else {
      fileWeb.send(500, "text/plain", "Failed to write SD");
    }
  });
  
  fileWeb.on("/api/upload", HTTP_POST, [](){
    if (webUploadFile) { webUploadFile.flush(); webUploadFile.close(); }
    if (webUploadOk) { sdChangeGeneration++; fileWeb.send(200,"text/plain; charset=utf-8",String("Загружено: ")+String(webUploadBytes)+" байт в "+webUploadDest); }
    else fileWeb.send(500,"text/plain; charset=utf-8","Загрузка не выполнена: SD недоступна, путь неверен или запись не удалась");
  }, [](){
    HTTPUpload& u=fileWeb.upload();
    if (u.status==UPLOAD_FILE_START) {
      if (webUploadFile) webUploadFile.close();
      webUploadOk=false; webUploadBytes=0; webUploadDest=cleanPath(fileWeb.arg("path"));
      if (sdReady && webUploadDest.length() && webUploadDest!="/" && baseName(webUploadDest).length()) {
        if (SD.exists(webUploadDest)) SD.remove(webUploadDest);
        webUploadFile=SD.open(webUploadDest,FILE_WRITE);
        webUploadOk=(bool)webUploadFile;
      }
    } else if (u.status==UPLOAD_FILE_WRITE) {
      if (webUploadFile && webUploadOk) { size_t n=webUploadFile.write(u.buf,u.currentSize); if(n!=u.currentSize) webUploadOk=false; else webUploadBytes+=n; }
    } else if (u.status==UPLOAD_FILE_END) {
      if (webUploadFile) { webUploadFile.flush(); webUploadFile.close(); }
    } else if (u.status==UPLOAD_FILE_ABORTED) {
      if (webUploadFile) webUploadFile.close(); webUploadOk=false;
    }
  });
fileWeb.on("/api/apps/list", HTTP_GET, [](){
  if (!sdReady) { fileWeb.send(200, "application/json", "[]"); return; }
  String out = "[";
  bool first = true;
  File dir = SD.open("/apps");
  if (dir) {
    File f = dir.openNextFile();
    while (f) {
      String nm = String(f.name());
      String low = nm; low.toLowerCase();
      if (!f.isDirectory() && low.endsWith(".mbg")) {
        if (!first) out += ",";
        first = false;
        out += "{\"name\":\"" + nm + "\",\"size\":" + String(f.size()) + "}";
      }
      f.close();
      f = dir.openNextFile();
    }
    if (f) f.close();
    dir.close();
  }
  out += "]";
  fileWeb.send(200, "application/json", out);
});

fileWeb.on("/api/apps/delete", HTTP_GET, [](){
  String nm = fileWeb.arg("name");
  if (!sdReady || !nm.length()) { fileWeb.send(400, "text/plain", "Bad request"); return; }
  String path = "/apps/" + nm;
  if (SD.exists(path) && SD.remove(path)) {
    scanMbApps();
    fileWeb.send(200, "text/plain", "Deleted");
  } else {
    fileWeb.send(404, "text/plain", "Not found");
  }
});
fileWeb.on("/api/apps/all", HTTP_GET, [](){
  static String cache = "";
  static uint32_t lastUpdate = 0;
  
  if (millis() - lastUpdate < 3000 && cache.length() > 0) {
    fileWeb.sendHeader("Access-Control-Allow-Origin", "*");
    fileWeb.send(200, "application/json", cache);
    return;
  }
  
  fileWeb.sendHeader("Access-Control-Allow-Origin", "*");
  
  JsonDocument modulesDoc;
  deserializeJson(modulesDoc, FIRMWARE_MODULES_JSON);
  
  String out = "{";
  out += "\"version\":\"" FIRMWARE_VERSION "\",";
  out += "\"uptime_ms\":" + String(millis()) + ",";
  out += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  out += "\"wifi_rssi\":" + String(WiFi.status()==WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  
  out += "\"builtin\":[";
  bool first = true;
  for (int i = 0; i < APP_COUNT; i++) {
    const char* mid = moduleIdForApp(i);
    if (!mid) continue;
    bool en = modulesDoc[mid] | false;
    if (!first) out += ",";
    first = false;
    String nm = String(appName(i));
    nm.replace("\"", "\\\"");
    out += "{\"id\":" + String(i) + ",\"module\":\"" + mid + "\",\"name\":\"" + nm + "\",\"enabled\":" + (en?"true":"false") + "}";
  }
  out += "],\"sd\":[";
  first = true;
  if (sdReady) {
    File dir = SD.open("/apps");
    if (dir) {
      File f = dir.openNextFile();
      while (f) {
        String nm = String(f.name());
        String low = nm; low.toLowerCase();
        if (!f.isDirectory() && low.endsWith(".mbg")) {
          if (!first) out += ",";
          first = false;
          out += "{\"name\":\"" + nm + "\",\"size\":" + String(f.size()) + "}";
        }
        f.close();
        f = dir.openNextFile();
      }
      if (f) f.close();
      dir.close();
    }
  }
  out += "],";
  out += "\"sd_ready\":" + String(sdReady ? "true" : "false");
  out += "}";
  
  cache = out;
  lastUpdate = millis();
  fileWeb.send(200, "application/json", out);
});
  fileWeb.begin();
}
void serviceFileWeb(){ captiveDns.processNextRequest(); fileWeb.handleClient(); }


static const uint8_t RTC_ADDR = 0x68;
bool rtcPresent = false;
uint32_t lastRtcWrite = 0;
uint8_t bcdToDec(uint8_t v) { return (uint8_t)(((v>>4)*10)+(v&0x0F)); }
uint8_t decToBcd(uint8_t v) { return (uint8_t)(((v/10)<<4)|(v%10)); }
bool rtcReadTime(struct tm *out) {
  Wire.beginTransmission(RTC_ADDR); Wire.write((uint8_t)0x00);
  if (Wire.endTransmission(false)!=0 || Wire.requestFrom((int)RTC_ADDR,7)!=7) return false;
  uint8_t sec=bcdToDec(Wire.read()&0x7F), min=bcdToDec(Wire.read()&0x7F), hrRaw=Wire.read();
  uint8_t hour=(hrRaw&0x40)?(uint8_t)((bcdToDec(hrRaw&0x1F)%12)+((hrRaw&0x20)?12:0)):bcdToDec(hrRaw&0x3F);
  Wire.read(); uint8_t day=bcdToDec(Wire.read()&0x3F), mon=bcdToDec(Wire.read()&0x1F), yr=bcdToDec(Wire.read());
  if(mon<1||mon>12||day<1||day>31||hour>23||min>59||sec>59) return false;
  memset(out,0,sizeof(*out)); out->tm_sec=sec; out->tm_min=min; out->tm_hour=hour;
  out->tm_mday=day; out->tm_mon=mon-1; out->tm_year=(yr>=70?yr+30:yr+100); out->tm_isdst=-1;
  return true;
}
bool rtcWriteTime(time_t stamp) {
  struct tm t; if(!localtime_r(&stamp,&t)) return false;
  Wire.beginTransmission(RTC_ADDR); Wire.write((uint8_t)0x00);
  Wire.write(decToBcd((uint8_t)t.tm_sec)); Wire.write(decToBcd((uint8_t)t.tm_min));
  Wire.write(decToBcd((uint8_t)t.tm_hour)); Wire.write(decToBcd((uint8_t)(t.tm_wday?t.tm_wday:7)));
  Wire.write(decToBcd((uint8_t)t.tm_mday)); Wire.write(decToBcd((uint8_t)(t.tm_mon+1)));
  Wire.write(decToBcd((uint8_t)((t.tm_year+1900)%100)));
  return Wire.endTransmission()==0;
}
bool rtcSetSystemClock() {
  struct tm t; if(!rtcReadTime(&t)) return false;
  time_t stamp=mktime(&t); if(stamp<1700000000) return false;
  struct timeval tv; tv.tv_sec=stamp; tv.tv_usec=0; settimeofday(&tv,nullptr); return true;
}
void rtcService() {
  if(!rtcPresent) return; time_t now=time(nullptr);
  if(now>1700000000 && millis()-lastRtcWrite>60000){ lastRtcWrite=millis(); rtcWriteTime(now); }
}

const uint32_t FRAME_MS = 33;
int menuSel = 0;
int activeAppId = -1;
bool joystickEnabled = false;
int joyCenterX = 2048, joyCenterY = 2048;
int joyFilteredX = 2048, joyFilteredY = 2048;
bool joyLeftActive=false, joyRightActive=false, joyUpActive=false, joyDownActive=false;
bool joySwitchRaw=false, joySwitchStable=false, joySwitchPrevStable=false;
uint32_t joySwitchChanged=0;
bool joyOkTapEvent=false;
int joyMinX=0, joyMaxX=4095, joyMinY=0, joyMaxY=4095;
bool appSettingsRequested = false;
void calibrateJoystickWizard(bool force = false);
bool insideAppOptions = false;
uint8_t appDifficulty[APP_COUNT] = {0};
int flappyGap = 28;                 // configurable pipe gap, 22..36 px
bool dinoPterodactyl = true;        // unlock flying obstacle after 500 points
int dinoNightScore = 500;           // day/night threshold requested by user
int ttW=0, ttL=0, ttD=0;

void runAppOptions(int id);
uint8_t difficultyFor(int id) {
  if (id < 0 || id >= APP_COUNT) return cfg.difficulty;
  return appDifficulty[id] <= 2 ? appDifficulty[id] : cfg.difficulty;
}

// ============ МУЗЫКА ============
const Note TECH_MENU[]   = {{60,2},{64,2},{67,2},{72,2},{67,2},{64,2},{69,2},{72,2},{71,2},{67,2},{62,2},{67,2},{69,2},{65,2},{60,4},{0,2},{0,0}};
const Note TECH_DINO[]   = {{72,1},{76,1},{79,2},{76,2},{72,2},{67,2},{72,2},{76,2},{80,2},{81,2},{80,1},{79,1},{76,2},{72,2},{67,2},{64,4},{0,2},{0,0}};
const Note TECH_FLAPPY[] = {{79,2},{83,2},{86,2},{83,2},{79,2},{74,2},{78,2},{81,2},{83,2},{81,1},{79,1},{78,2},{76,2},{74,4},{0,2},{79,2},{83,2},{86,2},{91,2},{86,4},{0,0}};
const Note TECH_TETRIS[] = {{40,2},{0,1},{40,1},{47,2},{0,1},{43,1},{40,2},{0,1},{38,1},{40,2},{47,2},{50,1},{47,1},{43,2},{40,2},{0,1},{40,1},{38,2},{35,2},{38,1},{40,1},{43,2},{40,4},{0,2},{0,0}};
const Note TECH_SNAKE[]  = {{67,1},{71,1},{74,1},{71,1},{76,2},{74,2},{71,1},{74,1},{79,2},{78,2},{76,1},{74,1},{71,2},{69,2},{67,4},{0,2},{0,0}};
const Note CHILL_MENU[]  = {{48,4},{52,4},{55,4},{60,6},{55,2},{52,4},{57,4},{60,4},{57,2},{55,4},{52,6},{0,2},{0,0}};
const Note CHILL_DINO[]  = {{55,2},{59,2},{62,3},{59,2},{55,3},{50,2},{55,2},{59,2},{64,3},{62,2},{59,3},{55,4},{0,2},{0,0}};
const Note CHILL_FLAPPY[]= {{62,3},{66,3},{69,4},{66,2},{62,3},{57,2},{62,3},{65,3},{69,4},{65,2},{62,4},{0,2},{0,0}};
const Note CHILL_TETRIS[]= {{67,4},{62,3},{64,3},{67,4},{64,3},{62,3},{59,4},{59,3},{62,2},{67,4},{65,3},{62,3},{60,4},{0,2},{0,0}};
const Note CHILL_SNAKE[] = {{50,2},{54,2},{57,3},{54,2},{50,3},{45,2},{50,2},{54,2},{59,3},{57,2},{54,4},{0,2},{0,0}};
const Note DARK_MENU[]   = {{45,3},{48,3},{42,3},{45,3},{50,4},{45,2},{40,4},{45,4},{48,3},{42,2},{45,4},{0,2},{0,0}};
const Note DARK_DINO[]   = {{50,2},{45,2},{47,3},{42,2},{45,3},{40,2},{45,2},{50,2},{52,3},{47,2},{45,4},{0,2},{0,0}};
const Note DARK_FLAPPY[] = {{52,3},{47,3},{50,4},{47,2},{52,3},{42,2},{50,3},{55,3},{52,4},{47,2},{45,4},{0,2},{0,0}};
const Note DARK_TETRIS[] = {{36,2},{36,1},{0,1},{43,2},{39,2},{36,2},{0,1},{34,1},{36,2},{31,2},{34,1},{36,1},{39,2},{36,4},{0,2},{0,0}};
const Note DARK_SNAKE[]  = {{42,2},{45,2},{48,3},{45,2},{42,3},{37,2},{42,2},{47,2},{50,3},{48,2},{45,4},{0,2},{0,0}};
const Note OLD_MENU[]    = {{60,1},{62,1},{64,1},{65,1},{67,1},{65,1},{64,2},{62,2},{60,2},{62,2},{64,4},{0,2},{0,0}};
const Note OLD_DINO[]    = {{64,1},{66,1},{69,2},{66,2},{64,2},{62,2},{64,2},{66,2},{71,2},{72,2},{71,1},{69,1},{66,2},{64,2},{62,2},{60,4},{0,2},{0,0}};
const Note OLD_FLAPPY[]  = {{72,2},{76,2},{79,2},{76,2},{72,2},{67,2},{71,2},{74,2},{76,2},{74,1},{72,1},{71,2},{69,2},{67,4},{0,2},{0,0}};
const Note OLD_TETRIS[]  = {{40,2},{40,2},{47,2},{43,2},{40,2},{38,2},{40,2},{47,2},{50,2},{47,2},{43,2},{40,4},{0,2},{0,0}};
const Note OLD_SNAKE[]   = {{60,1},{64,1},{67,1},{64,1},{69,2},{67,2},{64,1},{67,1},{72,2},{71,2},{69,1},{67,1},{64,2},{62,2},{60,4},{0,2},{0,0}};
// Original chiptune loops for Super Mario mode (not copied from commercial music).
const Note MARIO_TECH[] = {{72,1},{76,1},{79,2},{84,2},{79,2},{76,2},{72,2},{74,1},{77,1},{81,2},{86,2},{81,2},{77,2},{74,2},{79,1},{83,1},{86,2},{91,3},{86,1},{83,2},{79,2},{76,4},{0,2},{0,0}};
const Note MARIO_OLD[]  = {{60,2},{64,2},{67,2},{72,3},{67,1},{64,2},{62,2},{65,2},{69,3},{65,1},{62,2},{60,2},{64,2},{67,2},{72,4},{0,2},{0,0}};
const Note MARIO_CHILL[]= {{55,3},{60,3},{64,4},{67,4},{64,2},{60,2},{57,3},{62,3},{65,4},{69,4},{65,2},{62,2},{55,4},{59,4},{62,4},{67,5},{0,2},{0,0}};
const Note MARIO_DARK[] = {{48,2},{51,2},{55,3},{58,2},{55,2},{51,3},{46,2},{50,2},{53,3},{58,2},{53,2},{50,3},{48,4},{0,2},{0,0}};
// Additional original loops used as alternate tracks in the existing playlist.
const Note EXTRA_TECH[]  = {{76,1},{79,1},{83,2},{88,2},{83,2},{79,2},{74,1},{78,1},{81,2},{86,2},{81,2},{78,2},{72,1},{76,1},{79,2},{84,4},{0,2},{0,0}};
const Note EXTRA_CHILL[] = {{60,4},{64,3},{67,5},{64,3},{62,4},{65,3},{69,5},{65,3},{59,4},{62,3},{67,5},{62,3},{0,2},{0,0}};
const Note EXTRA_DARK[]  = {{43,3},{46,2},{50,4},{46,2},{41,3},{45,2},{48,4},{45,2},{39,3},{43,2},{46,5},{43,3},{0,2},{0,0}};
const Note EXTRA_OLD[]   = {{64,1},{67,1},{69,2},{72,2},{69,2},{67,2},{62,1},{65,1},{67,2},{71,2},{67,2},{65,2},{60,4},{0,2},{0,0}};

// ============ SFX ============
const Step SF_CLICK[] = {{1600,10}};
const Step SF_MOVE[]  = {{700,8}};
const Step SF_ROT[]   = {{1100,8},{1400,8}};
const Step SF_EAT[]   = {{1000,40},{1500,60}};
const Step SF_SHOOT[] = {{1800,12},{900,30}};
const Step SF_HIT[]   = {{300,70},{200,80}};
const Step SF_SCORE[] = {{1319,60},{1568,60},{2093,110}};
const Step SF_DIE[]   = {{660,120},{494,120},{392,120},{262,300}};
const Step SF_WIN[]   = {{784,90},{988,90},{1175,90},{1568,260}};
const Step SF_JUMP[]  = {{700,25},{1000,25},{1300,30}};
const Step SF_BOOM[]  = {{220,120},{110,220}};
const Step SF_OPEN[]  = {{1200,15}};
const Step SF_FLAG[]  = {{1500,30},{1000,30}};
const Step SP0[] = {{523,160}};
const Step SP1[] = {{659,160}};
const Step SP2[] = {{392,160}};
const Step SP3[] = {{784,160}};
const Step* const SPADS[4] = {SP0,SP1,SP2,SP3};
#define SFX(arr) sfxPlay(arr, sizeof(arr)/sizeof(Step))

const char* appName(int id) {
  if (id >= DYNAMIC_APP_ID_BASE) {
    int idx = id - DYNAMIC_APP_ID_BASE;
    if (idx >= 0 && idx < mbAppCount) return mbApps[idx].name.c_str();
    return "?";
  }
  if (id == APP_FILE_MANAGER) return cfg.lang == 1 ? "Файловый менеджер" : (cfg.lang == 2 ? "Файловий менеджер" : "File Manager");
  
  static const char* namesEN[APP_COUNT] = {
    "Dino Run", "Flappy Bird", "Tetris", "Snake", "Pong", "Breakout",
    "Invaders", "Asteroids", "Minesweeper", "2048", "Simon", "Frogger",
    "Tic-Tac-Toe", "Gemini", "Snake Maze", "Memory", "Skater", "Tank",
    "Catch Stars", "Reaction", "Labyrinth", "Jumper", "Super Mario",
    "Notes", "Settings"
  };
  static const char* namesRU[APP_COUNT] = {
    "Бег Дино", "Флаппи", "Тетрис", "Змейка", "Понг", "Арканоид",
    "Захватчики", "Астероиды", "Сапёр", "2048", "Саймон", "Фроггер",
    "Крестики-нолики", "Gemini", "Змейка в лабиринте", "Память", "Скейт",
    "Танки", "Лови звёзды", "Реакция", "Лабиринт", "Прыгун", "Super Mario",
    "Заметки", "Настройки"
  };
  static const char* namesUA[APP_COUNT] = {
    "Біг Діно", "Флаппі", "Тетріс", "Змійка", "Понг", "Арканоїд",
    "Загарбники", "Астероїди", "Сапер", "2048", "Саймон", "Фрогер",
    "Хрестики-нулики", "Gemini", "Змійка в лабіринті", "Пам'ять", "Скейт",
    "Танки", "Лови зірки", "Реакція", "Лабіринт", "Стрибун", "Super Mario",
    "Нотатки", "Налаштування"
  };
  
  if (id < 0 || id >= APP_COUNT) return "?";
  const char* result;
  if (cfg.lang == 1) result = namesRU[id];
  else if (cfg.lang == 2) result = namesUA[id];
  else result = namesEN[id];
  return result ? result : "?";
}

const char* GAME_DOCS_EN[APP_COUNT] = {
  "DINO RUN: Jump over cacti. OK/UP to jump. Speed increases. Score = distance.",
  "FLAPPY BIRD: Tap OK/UP to flap. Fly through pipe gaps. Hitting a pipe or ground ends the game.",
  "TETRIS: Fit falling blocks into rows. LEFT/RIGHT move, UP/OK rotate, DOWN soft drop. 4 lines = 800 pts.",
  "SNAKE: Eat food to grow. Don't hit walls or yourself. +10 per food.",
  "PONG: Move paddle with UP/DOWN. Beat the AI to 5 points.",
  "BREAKOUT: Break all bricks. Filled bricks need 2 hits. Paddle widens after 100 pts.",
  "INVADERS: Shoot all invaders. LEFT/RIGHT move, OK fires. 10 pts per alien.",
  "ASTEROIDS: Destroy all asteroids. LEFT/RIGHT rotate, UP thrust, OK fires. 3 lives.",
  "MINESWEEPER: Find all mines. Arrows move, OK reveals, BACK flags.",
  "2048: Merge equal tiles. Reach 2048. Arrows to move.",
  "SIMON: Repeat the sequence. UP/LEFT/RIGHT/OK = 4 buttons.",
  "FROGGER: Cross the road. Avoid cars. Reach the top. 3 lives.",
  "TIC-TAC-TOE: Beat the AI. Get 3 in a row. AI is smart and unpredictable.",
  "GEMINI: Ask Gemini AI anything. Requires Wi-Fi.",
  "SNAKE MAZE: Snake in a maze. Avoid walls. Grow longer.",
  "MEMORY: Find all matching pairs. 8 pairs to win.",
  "SKATER: Jump over obstacles. OK/UP to jump. Score for distance.",
  "TANK: Destroy enemy tanks. Arrows move, OK fires. Walls randomize each level.",
  "CATCH STARS: Catch falling stars with the bag. LEFT/RIGHT move. Golden stars = 3 pts.",
  "REACTION: Test your reflexes. Wait for PRESS, hit OK fast. 5 tries.",
  "LABYRINTH: Find the exit. New random maze each level. Arrows move.",
  "JUMPER: Climb platforms upward. Move LEFT/RIGHT. Camera follows you.",
  "SUPER MARIO: Four platform levels, enemies, coins, stars and a boss. LEFT/RIGHT move, UP/OK jump.",
  "SETTINGS: Configure device. Wi-Fi, brightness, sound, time, records, system info."
  "NOTES: Create, edit and save notes to SD. OK: open, hold OK: actions, BACK: exit.",
};

const char* GAME_DOCS_RU[APP_COUNT] = {
  "БЕГ ДИНО: Прыгай через кактусы. OK или ВВЕРХ — прыжок. Скорость растёт.",
  "ФЛАППИ: Тапай OK/ВВЕРХ для взмаха. Пролетай между трубами.",
  "ТЕТРИС: Складывай блоки. ВЛЕВО/ВПРАВО — движение, ВВЕРХ/OK — поворот, ВНИЗ — вниз.",
  "ЗМЕЙКА: Ешь еду, не бейся о стены и хвост. +10 за еду.",
  "ПОНГ: Двигай ракетку ВВЕРХ/ВНИЗ. Победи ИИ до 5 очков.",
  "АРКАНОИД: Разбей кирпичи. Закрашенные — 2 удара. После 100 очков ракетка шире.",
  "ПРИШЕЛЬЦЫ: Сбей всех. ВЛЕВО/ВПРАВО движение, OK огонь. 10 очков за каждого.",
  "АСТЕРОИДЫ: Уничтожь все. ВЛЕВО/ВПРАВО поворот, ВВЕРХ тяга, OK огонь. 3 жизни.",
  "САПЁР: Найди все мины. Стрелки движение, OK открыть, BACK флаг.",
  "2048: Объединяй плитки. Собери 2048. Стрелки для движения.",
  "САЙМОН: Повтори последовательность. ВВЕРХ/ВЛЕВО/ВПРАВО/OK — 4 кнопки.",
  "ЛЯГУШКА: Перейди дорогу. Избегай машин. Доберись до верха. 3 жизни.",
  "КРЕСТИКИ-НОЛИКИ: Победи ИИ. Собери 3 в ряд. ИИ умный и непредсказуемый.",
  "GEMINI: Спроси Gemini AI что угодно. Нужен Wi-Fi.",
  "ЗМЕЙКА-ЛАБИРИНТ: Змейка в лабиринте. Не бейся о стены. Расти.",
  "ПАМЯТЬ: Найди все пары. 8 пар для победы.",
  "СКЕЙТЕР: Прыгай через препятствия. OK/ВВЕРХ прыжок. Очки за дистанцию.",
  "ТАНЧИКИ: Уничтожь танки врага. Стрелки движение, OK огонь. Стены рандом.",
  "ЗВЁЗДЫ: Лови звёзды сумкой. ВЛЕВО/ВПРАВО движение. Золотые = 3 очка.",
  "РЕАКЦИЯ: Тест реакции. Жди ПРИГЛАШЕНИЯ, тисни OK. 5 попыток.",
  "ЛАБИРИНТ: Найди выход. Новый случайный лабиринт на каждом уровне.",
  "ПРЫГУН: Прыгай вверх по платформам. ВЛЕВО/ВПРАВО движение. Камера следит.",
  "SUPER MARIO: Четыре уровня, враги, монеты, звезда и босс. ВЛЕВО/ВПРАВО — движение, ВВЕРХ/OK — прыжок.",
  "НАСТРОЙКИ: Настройки устройства. Wi-Fi, яркость, звук, время, рекорды."
  "ЗАМЕТКИ: Создавай, редактируй и сохраняй заметки на SD. OK: открыть, удержание OK: действия, BACK: выход.",
};

const char* GAME_DOCS_UA[APP_COUNT] = {
  "БІГ ДИНО: Стрибай через кактуси. OK або ВГОРУ — стрибок. Швидкість зростає.",
  "ФЛАППІ: Тапай OK/ВГОРУ для помаху. Пролітай між трубами.",
  "ТЕТРІС: Складай блоки. ВЛІВО/ВПРАВО рух, ВГОРУ/OK поворот, ВНИЗ вниз.",
  "ЗМІЙКА: Їж їжу, не бийся об стіни та хвіст. +10 за їжу.",
  "ПОНГ: Рухай ракетку ВГОРУ/ВНИЗ. Переможи ШІ до 5 очок.",
  "АРКАНОЇД: Розбий цеглини. Зафарбовані — 2 удари. Після 100 очок ракетка ширша.",
  "ПРИШЕЛЬЦІ: Збий усіх. ВЛІВО/ВПРАВО рух, OK вогонь. 10 очок за кожного.",
  "АСТЕРОЇДИ: Знищ усі. ВЛІВО/ВПРАВО поворот, ВГОРУ тяга, OK вогонь. 3 життя.",
  "САПЕР: Знайди всі міни. Стрілки рух, OK відкрити, BACK прапор.",
  "2048: Об'єднуй плитки. Збери 2048. Стрілки для руху.",
  "САЙМОН: Повтори послідовність. ВГОРУ/ВЛІВО/ВПРАВО/OK — 4 кнопки.",
  "ЖАБКА: Перейди дорогу. Уникай машин. Дістанься верху. 3 життя.",
  "ХРЕСТИКИ-НУЛИКИ: Переможи ШІ. Збери 3 в ряд. ШІ розумний та непередбачуваний.",
  "GEMINI: Запитай Gemini AI що завгодно. Потрібен Wi-Fi.",
  "ЗМІЙКА-ЛАБІРИНТ: Змійка в лабіринті. Не бийся об стіни. Рости.",
  "ПАМ'ЯТЬ: Знайди всі пари. 8 пар для перемоги.",
  "СКЕЙТЕР: Стрибай через перешкоди. OK/ВГОРУ стрибок. Очки за дистанцію.",
  "ТАНЧИКИ: Знищ танки ворога. Стрілки рух, OK вогонь. Стіни рандом.",
  "ЗІРКИ: Лови зірки сумкою. ВЛІВО/ВПРАВО рух. Золоті = 3 очки.",
  "РЕАКЦІЯ: Тест реакції. Чекай ЗАПРОШЕННЯ, тисни OK. 5 спроб.",
  "ЛАБІРИНТ: Знайди вихід. Новий випадковий лабіринт на кожному рівні.",
  "СТРИБУН: Стрибай вгору по платформах. ВЛІВО/ВПРАВО рух. Камера стежить.",
  "SUPER MARIO: Чотири рівні, вороги, монети, зірка та бос. ВЛІВО/ВПРАВО — рух, ВГОРУ/OK — стрибок.",
  "НАЛАШТУВАННЯ: Налаштування пристрою. Wi-Fi, яскравість, звук, час, рекорди."
  "НОТАТКИ: Створюй, редагуй та зберігай нотатки на SD. OK: відкрити, утримання OK: дії, BACK: вихід.",
};

// ============ AUDIO ============
const uint8_t VOL_DUTY_TABLE[21] = {0,5,10,15,20,26,32,38,44,50,56,62,68,74,80,86,92,98,104,110,120};
int midiHz(uint8_t m) { return (int)(440.0f * powf(2.0f, (m-69)/12.0f)); }
void toneHz(int f) {
  if (f<=0) { ledcWrite(BUZ_PIN, 0); return; }
  ledcWriteTone(BUZ_PIN, f);
  // Keep the setting in 1% increments. PWM duty is continuous (0..120);
  // perceived loudness is approximate because the buzzer is not linear.
  ledcWrite(BUZ_PIN, (uint32_t)((cfg.volPercent * 120UL + 50UL) / 100UL));
}
const Step* sfxSteps=nullptr; uint8_t sfxN=0,sfxI=0; uint32_t sfxEnd=0; bool sfxOn=false;
const Note* mNotes=nullptr; uint8_t mN=0,mI=0; uint16_t mTick=120; uint32_t mEnd=0; int mFreq=0; bool musicPlaying=false;

void playMusicTheme(const Note* t, const Note* tc, const Note* chill, const Note* dark, const Note* old, uint16_t tick) {
  if (cfg.musicTheme==THEME_NONE || cfg.musicTheme>=THEME_COUNT) return;
  const Note* track = nullptr;
  // Two melodic variants are supplied by each game. Alternate them on each
  // launch so the selected playlist does not repeat the same tune every time.
  static uint8_t playlistTurn = 0;
  const bool alternate = ((playlistTurn++) & 1U) != 0;
  if (cfg.musicTheme==THEME_TECH) track = alternate ? EXTRA_TECH : (tc ? tc : t);
  else if (cfg.musicTheme==THEME_CHILL) track = alternate ? EXTRA_CHILL : chill;
  else if (cfg.musicTheme==THEME_DARK) track = alternate ? EXTRA_DARK : dark;
  else if (cfg.musicTheme==THEME_OLDSCHOOL) track = alternate ? EXTRA_OLD : old;
  if (!track) return;
  mNotes = track; mN = 0;
  while (mN<64 && mNotes[mN].t>0) mN++;
  if (mN==0) return;
  mI = mN-1; mTick = tick; mEnd = 0; musicPlaying = true;
}
void sfxPlay(const Step* s, uint8_t n) {
  if (!cfg.sfx || n==0 || sfxOn) return;
  sfxSteps = s; sfxN = n; sfxI = 0; sfxOn = true;
  sfxEnd = millis() + s[0].ms; toneHz(s[0].f);
}
void audioStop() { musicPlaying=false; sfxOn=false; toneHz(0); }
void audioTick() {
  uint32_t now = millis();
  while (sfxOn && now >= sfxEnd) {
    sfxI++;
    if (sfxI < sfxN) { sfxEnd += sfxSteps[sfxI].ms; toneHz(sfxSteps[sfxI].f); }
    else { sfxOn=false; toneHz(musicPlaying ? mFreq : 0); }
  }
  if (sfxOn || !musicPlaying) return;
  if (now >= mEnd) {
    mI = (mI+1)%mN;
    mFreq = mNotes[mI].m ? midiHz(mNotes[mI].m) : 0;
    mEnd = now + (uint32_t)mNotes[mI].t * mTick;
    toneHz(mFreq);
  }
}

// ============ NVS ============
void saveCfg() {
  prefs.putUChar("bright", cfg.bright);
  prefs.putBool("inv", cfg.invert);
  prefs.putBool("showpw", cfg.showPass);
  prefs.putUChar("layout", cfg.layout);
  prefs.putUChar("rep", cfg.rep);
  prefs.putBool("fps", cfg.showFps);
  prefs.putBool("auto", cfg.autoConn);
  prefs.putBool("sfx", cfg.sfx);
  prefs.putUChar("theme", cfg.musicTheme);
  prefs.putUChar("vol", cfg.volPercent);
  prefs.putUChar("diff", cfg.difficulty);
  prefs.putUChar("anim", cfg.animSpeed);
  prefs.putBool("clkshow", cfg.showClock);
  prefs.putBool("clk24", cfg.clock24h);
  prefs.putBool("autotime", cfg.autoTime);
  prefs.putUChar("lang", cfg.lang);
  prefs.putBool("filetypes", cfg.showFileTypes);
  prefs.putBool("setwrap", cfg.settingsWrap);
  prefs.putBool("joy", joystickEnabled);
  prefs.putInt("jcx", joyCenterX); prefs.putInt("jcy", joyCenterY);
  prefs.putInt("jminx", joyMinX); prefs.putInt("jmaxx", joyMaxX);
  prefs.putInt("jminy", joyMinY); prefs.putInt("jmaxy", joyMaxY);
}
void loadCfg() {
  cfg.bright = prefs.getUChar("bright",2)&3;
  cfg.invert = prefs.getBool("inv",false);
  cfg.showPass = prefs.getBool("showpw",false);
  cfg.layout = prefs.getUChar("layout",0)%3;
  cfg.rep = prefs.getUChar("rep",1)%3;
  cfg.showFps = prefs.getBool("fps",false);
  cfg.autoConn = prefs.getBool("auto",true);
  cfg.sfx = prefs.getBool("sfx",true);
  cfg.musicTheme = prefs.getUChar("theme",THEME_TECH)%THEME_COUNT;
  cfg.volPercent = min(100,(int)prefs.getUChar("vol",50));
  cfg.difficulty = prefs.getUChar("diff",1)%3;
  cfg.animSpeed = prefs.getUChar("anim",1)%3;
  cfg.showClock = prefs.getBool("clkshow",true);
  cfg.clock24h = prefs.getBool("clk24",true);
  cfg.autoTime = prefs.getBool("autotime",true);
  cfg.lang = prefs.getUChar("lang",0)%3;
  cfg.showFileTypes = prefs.getBool("filetypes", true);
  cfg.settingsWrap = prefs.getBool("setwrap", true);
  joystickEnabled = false; // joystick removed; GPIOs reserved for SD SPI
  joyCenterX = constrain(prefs.getInt("jcx", 2048), 0, 4095);
  joyCenterY = constrain(prefs.getInt("jcy", 2048), 0, 4095);
  joyMinX = constrain(prefs.getInt("jminx", 0), 0, 4095); joyMaxX = constrain(prefs.getInt("jmaxx", 4095), 0, 4095);
  joyMinY = constrain(prefs.getInt("jminy", 0), 0, 4095); joyMaxY = constrain(prefs.getInt("jmaxy", 4095), 0, 4095);
  flappyGap = constrain(prefs.getUChar("fgap", 28), 22, 36);
  dinoPterodactyl = prefs.getBool("dpt", true);
  dinoNightScore = constrain(prefs.getUShort("dns", 500), 300, 1500);
  for (int i=0; i<APP_COUNT; ++i) {
    char key[8]; snprintf(key, sizeof(key), "gd%02d", i);
    appDifficulty[i] = prefs.getUChar(key, cfg.difficulty) % 3;
  }
    // Загрузка GPIO кнопок из NVS
  cfg.btnGpioUp = prefs.getUChar("btn_up", 5);
  cfg.btnGpioDown = prefs.getUChar("btn_down", 7);
  cfg.btnGpioLeft = prefs.getUChar("btn_left", 3);
  cfg.btnGpioRight = prefs.getUChar("btn_right", 1);
  cfg.btnGpioOk = prefs.getUChar("btn_ok", 2);
  cfg.btnGpioBack = prefs.getUChar("btn_back", 6);
}
int getBest(int id) { char k[8]; snprintf(k,sizeof k,"b%d",id); return prefs.getInt(k,0); }
void setBest(int id, int v) {
  char k[8]; snprintf(k,sizeof k,"b%d",id);
  if (v > prefs.getInt(k,0)) {
    prefs.putInt(k,v);
    time_t ts = time(nullptr);
    char dk[8]; snprintf(dk,sizeof dk,"d%d",id);
    if (ts>1700000000) prefs.putLong64(dk,(int64_t)ts);
  }
}
int getPlays(int id) { char k[8]; snprintf(k,sizeof k,"p%d",id); return prefs.getInt(k,0); }
void bumpPlays(int id) { char k[8]; snprintf(k,sizeof k,"p%d",id); prefs.putInt(k,prefs.getInt(k,0)+1); }

void applyDisplay() {
  static const uint8_t lv[4]={40,120,200,255};
  oled.setContrast(lv[cfg.bright&3]);
  oled.sendF("c", cfg.invert ? 0xA7 : 0xA6);
}
uint16_t repDelay() { static const uint16_t d[3]={300,450,600}; return d[cfg.rep%3]; }
uint16_t repRate()  { static const uint16_t r[3]={55,90,140};   return r[cfg.rep%3]; }

// ============ КНОПКИ ============
int mapAxis(int val, int center, int minVal, int maxVal) {
  if (val < center) {
    if (center - minVal < 10) return 0;
    return -(center - val) * 100 / (center - minVal);
  } else {
    if (maxVal - center < 10) return 0;
    return (val - center) * 100 / (maxVal - center);
  }
}

void sendHeartbeat() {
  if (WiFi.status() != WL_CONNECTED) return;
  String storeUrl = prefs.getString("store_url", "http://192.168.178.22:8000");
  if (!storeUrl.length()) return;

  String host; int port = 80;
  String url = storeUrl;
  if (url.startsWith("http://")) url = url.substring(7);
  int slash = url.indexOf('/');
  String hp = (slash < 0) ? url : url.substring(0, slash);
  int colon = hp.indexOf(':');
  if (colon >= 0) { host = hp.substring(0, colon); port = hp.substring(colon + 1).toInt(); }
  else { host = hp; }

  String body = "{";
  body += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  body += "\"mac\":\"" + WiFi.macAddress() + "\",";
  body += "\"version\":\"" FIRMWARE_VERSION "\",";
  body += "\"modules\":" FIRMWARE_MODULES_JSON;
  body += "}";

  WiFiClient hc;
  hc.setTimeout(500);
  if (hc.connect(host.c_str(), port)) {
    hc.print("POST /api/heartbeat HTTP/1.1\r\n");
    hc.print("Host: " + host + "\r\n");
    hc.print("Content-Type: application/json\r\n");
    hc.print("Content-Length: " + String(body.length()) + "\r\n");
    hc.print("Connection: close\r\n\r\n");
    hc.print(body);
    delay(80);
    hc.stop();
  }
}
void pollButtons() {
  // Keep the web panel alive during menu, settings, and game input loops.
      static uint32_t lastHB = 0;
  if (millis() - lastHB > 15000) {
    lastHB = millis();
    sendHeartbeat();
  }
    if (!otaInProgress) serviceFileWeb();
  uint32_t now = millis();
  joyOkTapEvent = false;
  
  if (false && joystickEnabled) {
    int rx = analogRead(JOY_X_PIN);
    int ry = analogRead(JOY_Y_PIN);
    
    // Легке згладжування без затримок
    joyFilteredX = (joyFilteredX * 3 + rx) / 4;
    joyFilteredY = (joyFilteredY * 3 + ry) / 4;
    
    // Переводимо в відсотки від -100 до +100 (завдяки калібруванню)
    int nx = mapAxis(joyFilteredX, joyCenterX, joyMinX, joyMaxX);
    int ny = mapAxis(joyFilteredY, joyCenterY, joyMinY, joyMaxY);
    
    #define JOY_THRESHOLD 30 // 30% відхилення для спрацювання
    
    // Ліво/Право виправлено (поміняні місцями < та >)
    joyLeftActive  = (nx >  JOY_THRESHOLD);
    joyRightActive = (nx < -JOY_THRESHOLD);
    joyUpActive    = (ny >  JOY_THRESHOLD);
    joyDownActive  = (ny < -JOY_THRESHOLD);

    // Кнопка SW
    bool swRaw = digitalRead(JOY_SW_PIN) == LOW;
    if (swRaw != joySwitchRaw) { joySwitchRaw = swRaw; joySwitchChanged = now; }
    if (now - joySwitchChanged >= 30) joySwitchStable = joySwitchRaw;
    if (joySwitchPrevStable && !joySwitchStable) joyOkTapEvent = true;
    joySwitchPrevStable = joySwitchStable;
  } else {
    joyLeftActive = joyRightActive = joyUpActive = joyDownActive = false;
  }

  for (int i = 0; i < BI_COUNT; i++) {
    evDown[i] = evTap[i] = evRep[i] = evLong[i] = false;
    BtnState& b = bst[i];
    
    bool raw = (digitalRead(BTN_PIN[i]) == LOW) || remoteButtonHeld[i];
    
    // Підмішуємо джойстик до фізичних кнопок
    if (joystickEnabled) {
      if (i == BI_LEFT && joyLeftActive) raw = true;
      if (i == BI_RIGHT && joyRightActive) raw = true;
      if (i == BI_UP && joyUpActive) raw = true;
      if (i == BI_DOWN && joyDownActive) raw = true;
    }

    if (raw != b.raw) { b.raw = raw; b.changed = now; }
    if (b.raw != b.stable && now - b.changed >= 12) {
      b.stable = b.raw;
      if (b.stable) { evDown[i] = true; b.downAt = b.lastRep = now; b.longDone = false; }
      else if (!b.longDone) evTap[i] = true;
    }
    
    if (b.stable) {
      if (i == BI_BACK && !b.longDone && now - b.downAt >= 2500) { b.longDone = true; evLong[i] = true; }
      
      // Прискорений автоповтор (без великих затримок)
      uint16_t rd = 150; // Затримка перед повтором (мс)
      uint16_t rr = 60;  // Швидкість повтору (мс)
      if (i != BI_OK && i != BI_BACK && now - b.downAt >= rd && now - b.lastRep >= rr) {
        b.lastRep = now; evRep[i] = true;
      }
    }
  }
  // Consume one browser-button click after clearing/reading physical button events.
  if (pendingRemoteButton >= 0 && pendingRemoteButton < BI_COUNT) {
    int rb = pendingRemoteButton; pendingRemoteButton = -1;
    evDown[rb] = true; evTap[rb] = true;
    if (rb == BI_OK) joyOkTapEvent = true;
  }
  if (remoteOkReleasePending) { joyOkTapEvent = true; remoteOkReleasePending = false; }
}

bool nav(BtnId i) { return evDown[i] || evRep[i]; }
bool held(BtnId i) { return bst[i].stable; }
uint32_t heldMs(BtnId i) { return bst[i].stable ? millis() - bst[i].downAt : 0; }

bool quitPressed() {
  if (evLong[BI_BACK] && activeAppId >= 0 && activeAppId != APP_GEMINI && activeAppId != APP_SETTINGS && !insideAppOptions) {
    appSettingsRequested = true;
    return true;
  }
  return evTap[BI_BACK] || evLong[BI_BACK];
}

bool anyHeld() { 
  for (int i = 0; i < BI_COUNT; i++) if (bst[i].stable) return true; 
  return false; 
}

void waitRelease() {
  uint32_t start = millis();
  do {
    pollButtons();
    delay(8);
    // Примусовий вихід через 1 секунду, щоб нічого не зависало
    if (millis() - start > 1000) break;
  } while (anyHeld());
}
uint32_t fpsT=0; int fpsCnt=0, fpsVal=0;
bool clockConfigured=false; uint32_t lastClockAttempt=0;

bool chargingDetected() {
#if CHARGE_STATUS_PIN >= 0
  int level = digitalRead(CHARGE_STATUS_PIN);
  return CHARGE_ACTIVE_LOW ? (level==LOW) : (level==HIGH);
#else
  return false;
#endif
}
void drawSystemStatus() {
  time_t now = time(nullptr);
  struct tm tmNow;
  bool timeValid = now>1700000000 && localtime_r(&now,&tmNow);
  if (chargingDetected()) {
    oled.setDrawColor(0); oled.drawBox(0,0,9,9); oled.setDrawColor(1);
    oled.drawLine(5,1,2,5); oled.drawLine(2,5,5,5); oled.drawLine(5,5,3,8);
  }
  // Games draw their own score HUD; keep the system clock off the game area.
  if (activeAppId < 0 && cfg.showClock && timeValid) {
    int hour = tmNow.tm_hour;
    if (!cfg.clock24h) { hour%=12; if (hour==0) hour=12; }
    char clockText[9];
    snprintf(clockText,sizeof(clockText), cfg.clock24h?"%02d:%02d":"%2d:%02d", hour, tmNow.tm_min);
    oled.setDrawColor(0); oled.drawBox(86,0,42,9); oled.setDrawColor(1);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(128-oled.getStrWidth(clockText),7,clockText);
  }
}
void updateNetworkClock() {
  if (cfg.autoTime && WiFi.status()==WL_CONNECTED && !clockConfigured && millis()-lastClockAttempt>5000) {
    lastClockAttempt = millis();
    setenv("TZ","CET-1CEST,M3.5.0/2,M10.5.0/3",1); tzset();
    configTime(0,0,"pool.ntp.org","time.google.com","time.cloudflare.com");
    clockConfigured = true;
  }
}
void present() {
  audioTick();
  if (cfg.showFps && !suppressSystemOverlay) {
    fpsCnt++;
    if (millis()-fpsT>=1000) { fpsVal=fpsCnt; fpsCnt=0; fpsT=millis(); }
    oled.setFont(u8g2_font_4x6_tr);
    String s = "F"+String(fpsVal);
    oled.drawStr(128-oled.getStrWidth(s.c_str()),63,s.c_str());
  }
  updateNetworkClock();
  rtcService();
  if (!suppressSystemOverlay) drawSystemStatus();
  oled.sendBuffer();
}
void frameWait(uint32_t t0) {
  if (!otaInProgress) serviceFileWeb();
  uint32_t el = millis()-t0;
  if (el < FRAME_MS) delay(FRAME_MS-el);
}
void showScreen(const char* title, const String& a,const String& b, const String& c) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x13_t_cyrillic);
  oled.drawUTF8(0,12,title);
  oled.drawHLine(0,16,128);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  if (a.length()) oled.drawUTF8(4,30,a.c_str());
  if (b.length()) oled.drawUTF8(4,42,b.c_str());
  if (c.length()) oled.drawUTF8(4,54,c.c_str());
  present();
}
void overlayBox(const char* a, const char* b) {
  oled.drawRBox(12,18,104,30,4);
  oled.setDrawColor(0);
  oled.setFont(u8g2_font_6x13_t_cyrillic);
  oled.drawStr(64-oled.getStrWidth(a)/2,31,a);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  oled.drawStr(64-oled.getStrWidth(b)/2,43,b);
  oled.setDrawColor(1);
}
String padNum(int v, int digits) {
  String s = String(v);
  while ((int)s.length()<digits) s = "0"+s;
  return s;
}
int rssiLevel(int rssi) { return rssi>-55?4: rssi>-67?3: rssi>-78?2: rssi>-88?1:0; }
void drawBars(int x, int y, int lvl) {
  for (int i=0;i<4;i++) {
    int h = 2+i*2;
    if (i<lvl) oled.drawBox(x+i*3, y+8-h, 2, h);
    else oled.drawPixel(x+i*3, y+7);
  }
}
void drawWifiStatus(int x, int y) {
  bool on = WiFi.status()==WL_CONNECTED;
  drawBars(x, y, on ? rssiLevel(WiFi.RSSI()) : 0);
}
void drawLock(int x, int y) {
  oled.drawRFrame(x+1,y,4,4,2);
  oled.drawRBox(x,y+3,6,5,1);
}
void drawSparkle(int cx, int cy, int r) {
  int k = max(1, r/3);
  oled.drawTriangle(cx,cy-r,cx-k,cy,cx+k,cy);
  oled.drawTriangle(cx,cy+r,cx-k,cy,cx+k,cy);
  oled.drawTriangle(cx-r,cy,cx,cy-k,cx,cy+k);
  oled.drawTriangle(cx+r,cy,cx,cy-k,cx,cy+k);
}
void drawCloud(int x, int y) {
  oled.drawDisc(x,y,3); oled.drawDisc(x+4,y-2,4); oled.drawDisc(x+9,y,3);
}

// ============ ИКОНКИ ============
void drawIcon(int id, int cx, int cy) {
    if (id >= DYNAMIC_APP_ID_BASE) {
    int idx = id - DYNAMIC_APP_ID_BASE;
    if (idx < 0 || idx >= mbAppCount) return;
    MbApp& app = mbApps[idx];
    
    // Кастомная иконка из JSON, если есть
    if (app.iconOpCount > 0) {
      for (int i = 0; i < app.iconOpCount; i++) {
        auto& op = app.icon[i];
        int px = cx + op.p[0];
        int py = cy + op.p[1];
        switch (op.op) {
          case 0: oled.drawBox(px, py, op.p[2], op.p[3]); break;
          case 1: oled.drawFrame(px, py, op.p[2], op.p[3]); break;
          case 2: oled.drawLine(px, py, cx + op.p[2], cy + op.p[3]); break;
          case 3: oled.drawCircle(px, py, op.p[2]); break;
          case 4: oled.drawDisc(px, py, op.p[2]); break;
          case 5: oled.drawPixel(px, py); break;
        }
      }
      return;
    }
    
    // Иначе — иконка по движку
    String eng = app.engine;
    if (eng == "snake") {
      oled.drawRBox(cx-8, cy-5, 5, 5, 1);
      oled.drawRBox(cx-2, cy-5, 5, 5, 1);
      oled.drawRBox(cx+4, cy-5, 5, 5, 1);
      oled.drawDisc(cx+2, cy+5, 2);
    } else if (eng == "runner") {
      oled.drawRBox(cx-6, cy-2, 8, 8, 2);
      oled.drawRBox(cx-4, cy-6, 6, 6, 2);
      oled.drawHLine(cx-10, cy+8, 8);
      oled.drawHLine(cx+2, cy+8, 8);
    } else if (eng == "flappy") {
      oled.drawDisc(cx-2, cy, 4);
      oled.drawTriangle(cx+2, cy-2, cx+6, cy, cx+2, cy+2);
    } else if (eng == "breakout") {
      for (int r=0;r<2;r++) for (int c=0;c<3;c++)
        oled.drawBox(cx-9+c*6, cy-7+r*4, 5, 3);
      oled.drawDisc(cx+2, cy+5, 2);
    } else if (eng == "maze") {
      oled.drawFrame(cx-9, cy-9, 18, 18);
      oled.drawBox(cx-6, cy-6, 2, 8);
      oled.drawDisc(cx+5, cy+5, 2);
    } else if (eng == "jumper") {
      oled.drawDisc(cx, cy-4, 2);
      oled.drawHLine(cx-7, cy+2, 5);
      oled.drawHLine(cx+2, cy+7, 5);
    } else if (eng == "invaders") {
      oled.drawBox(cx-7, cy-5, 14, 8);
      oled.setDrawColor(0);
      oled.drawBox(cx-3, cy-2, 2, 2);
      oled.drawBox(cx+1, cy-2, 2, 2);
      oled.setDrawColor(1);
    } else if (eng == "pong") {
      oled.drawBox(cx-9, cy-6, 2, 12);
      oled.drawBox(cx+7, cy-3, 2, 12);
      oled.drawBox(cx-2, cy-2, 3, 3);
    } else if (eng == "gemini") {
      drawSparkle(cx-1, cy+1, 8);
      drawSparkle(cx+6, cy-6, 2);
    } else if (eng == "calculator") {
      oled.drawFrame(cx-8, cy-9, 16, 18);
      oled.drawBox(cx-6, cy-7, 12, 5);
      for (int r=0;r<3;r++) for (int c=0;c<3;c++)
        oled.drawBox(cx-6+c*4, cy+0+r*3, 3, 2);
    } else if (eng == "app") {
      oled.drawRBox(cx-7, cy-7, 14, 14, 3);
      oled.setDrawColor(0);
      oled.drawHLine(cx-4, cy-3, 8);
      oled.drawHLine(cx-4, cy+1, 8);
      oled.setDrawColor(1);
    } else {
      // Картридж по умолчанию
      oled.drawRBox(cx-7, cy-9, 14, 18, 2);
      oled.setDrawColor(0);
      oled.drawHLine(cx-4, cy-5, 8);
      oled.drawHLine(cx-4, cy-2, 8);
      oled.setDrawColor(1);
    }
    return;
  }
  switch (id) {
    case APP_DINO: { int x=cx-10,y=cy-8;
      oled.drawTriangle(x+2,y+8,x-1,y+4,x+2,y+11);
      oled.drawRBox(x+2,y+5,9,7,2); oled.drawRBox(x+5,y,8,7,2);
      oled.setDrawColor(0); oled.drawPixel(x+10,y+2); oled.setDrawColor(1);
      oled.drawBox(x+4,y+12,2,4); oled.drawBox(x+8,y+12,2,4); break; }
    case APP_FLAPPY: {
      oled.drawDisc(cx-4,cy,5);
      oled.setDrawColor(0); oled.drawPixel(cx-2,cy-2); oled.setDrawColor(1);
      oled.drawTriangle(cx+1,cy-2,cx+4,cy,cx+1,cy+2);
      oled.drawBox(cx+6,cy-12,7,3); oled.drawBox(cx+7,cy-9,5,7);
      oled.drawBox(cx+6,cy+5,7,3); oled.drawBox(cx+7,cy+8,5,5); break; }
    case APP_TETRIS: { int x=cx-8,y=cy-2;
      oled.drawBox(x,y,3,3); oled.drawBox(x+4,y,3,3); oled.drawBox(x+8,y,3,3);
      oled.drawBox(x+4,y-4,3,3);
      oled.drawFrame(x,y+6,3,3); oled.drawFrame(x+4,y+6,3,3); oled.drawFrame(x+8,y+6,3,3); break; }
    case APP_SNAKE: {
      const int8_t sx[8]={-8,-4,0,4,4,4,0,-4};
      const int8_t sy[8]={-6,-6,-6,-6,-2,2,2,2};
      for (int i=0;i<8;i++) oled.drawBox(cx+sx[i],cy+sy[i],3,3);
      oled.setDrawColor(0); oled.drawPixel(cx-5,cy+3); oled.setDrawColor(1); break; }
    case APP_PONG: {
      oled.drawBox(cx-10,cy-6,2,12); oled.drawBox(cx+8,cy-3,2,12);
      oled.drawBox(cx-2,cy-2,3,3); break; }
    case APP_BRICK: {
      for (int r=0;r<2;r++) for (int c=0;c<4;c++) oled.drawBox(cx-12+c*6,cy-7+r*5,5,4);
      oled.drawDisc(cx+4,cy+7,2); break; }
    case APP_INVADE: {
      oled.drawBox(cx-7,cy-5,14,9);
      oled.setDrawColor(0);
      oled.drawBox(cx-4,cy-2,2,2); oled.drawBox(cx+2,cy-2,2,2);
      oled.setDrawColor(1);
      oled.drawBox(cx-6,cy+4,2,3); oled.drawBox(cx+4,cy+4,2,3); break; }
    case APP_ASTER: {
      oled.drawCircle(cx,cy,9); oled.drawTriangle(cx,cy-4,cx-3,cy+3,cx+3,cy+3); break; }
    case APP_MINES: {
      oled.drawFrame(cx-9,cy-9,18,18); oled.drawDisc(cx,cy,4);
      oled.drawLine(cx-7,cy,cx+7,cy); oled.drawLine(cx,cy-7,cx,cy+7); break; }
    case APP_2048: {
      oled.drawFrame(cx-11,cy-7,22,14);
      oled.setFont(u8g2_font_4x6_tr); oled.drawStr(cx-8,cy+3,"2048"); break; }
    case APP_SIMON: {
      oled.drawRBox(cx-9,cy-8,8,7,1); oled.drawRFrame(cx+1,cy-8,8,7,1);
      oled.drawRFrame(cx-9,cy+1,8,7,1); oled.drawRBox(cx+1,cy+1,8,7,1); break; }
    case APP_FROGGER: {
      oled.drawRBox(cx-12,cy-6,9,4,1); oled.drawRBox(cx+2,cy-6,9,4,1);
      oled.drawDisc(cx-2,cy+5,3); break; }
    case APP_TTT: {
      oled.drawVLine(cx-3,cy-9,18); oled.drawVLine(cx+3,cy-9,18);
      oled.drawHLine(cx-9,cy-3,18); oled.drawHLine(cx-9,cy+3,18);
      oled.drawLine(cx-8,cy-8,cx-5,cy-5); oled.drawLine(cx-5,cy-8,cx-8,cy-5);
      oled.drawCircle(cx+6,cy+6,2); break; }
    case APP_GEMINI: {
      drawSparkle(cx-1,cy+1,10); drawSparkle(cx+8,cy-8,3); break; }
    case APP_SNAKE_MAZE: {
      oled.drawBox(cx-10,cy-10,20,3);
      for (int i=0;i<4;i++) oled.drawBox(cx-8+i*4,cy-5+i*2,3,3);
      oled.drawBox(cx+6,cy+8,4,4); break; }
    case APP_MEMORY: {
      oled.drawFrame(cx-12,cy-8,6,6); oled.drawFrame(cx-4,cy-8,6,6); oled.drawFrame(cx+4,cy-8,6,6);
      oled.drawFrame(cx-12,cy+2,6,6); oled.drawBox(cx-4,cy+2,6,6); oled.drawFrame(cx+4,cy+2,6,6); break; }
    case APP_SKATE: {
      oled.drawRBox(cx-8,cy-2,16,4,2);
      oled.drawDisc(cx-6,cy+3,2); oled.drawDisc(cx+6,cy+3,2);
      oled.drawLine(cx-2,cy-6,cx+2,cy-8); break; }
    case APP_TANK: {
      oled.drawRBox(cx-6,cy-4,12,8,2);
      oled.drawDisc(cx-4,cy-6,2); oled.drawDisc(cx+4,cy-6,2);
      oled.drawLine(cx,cy-1,cx+8,cy-4); break; }
    case APP_STARS: {
      for (int i=0;i<5;i++) {
        int x=cx-8+i*4, y=cy-4+random(-4,5);
        oled.drawPixel(x,y); oled.drawPixel(x,y+1);
      }
      oled.drawBox(cx-6,cy+8,12,2); break; }
    case APP_REACTION: {
      oled.drawCircle(cx,cy,8);
      oled.drawDisc(cx-3,cy-2,2); oled.drawDisc(cx+3,cy-2,2);
      oled.drawLine(cx-3,cy+4,cx+3,cy+4); break; }
    case APP_LABYRINTH: {
      oled.drawFrame(cx-10,cy-10,20,20);
      oled.drawBox(cx-6,cy-8,2,8); oled.drawBox(cx+2,cy-2,6,2); oled.drawBox(cx-8,cy+2,8,2);
      oled.drawDisc(cx+7,cy+7,2); break; }
    case APP_JUMPER: {
      oled.drawRBox(cx-2,cy-1,4,3,1);
      oled.drawHLine(cx-8,cy+4,5); oled.drawHLine(cx+2,cy+8,5);
      oled.drawPixel(cx,cy+12); break; }
        case APP_NOTES: {
      oled.drawRBox(cx-7, cy-9, 14, 18, 2);
      oled.setDrawColor(0);
      oled.drawHLine(cx-5, cy-5, 10);
      oled.drawHLine(cx-5, cy-1, 10);
      oled.drawHLine(cx-5, cy+3, 10);
      oled.setDrawColor(1);
      oled.drawPixel(cx-3, cy-7); oled.drawPixel(cx+3, cy-7);
      break;
    }
    case APP_SETTINGS: {
      oled.drawDisc(cx,cy,7);
      oled.drawBox(cx-2,cy-10,4,4); oled.drawBox(cx-2,cy+6,4,4);
      oled.drawBox(cx-10,cy-2,4,4); oled.drawBox(cx+6,cy-2,4,4);
      oled.setDrawColor(0); oled.drawDisc(cx,cy,2); oled.setDrawColor(1); break; }
    case APP_PIXEL_QUEST: {
      oled.drawRBox(cx-3, cy-7, 6, 6, 2);
      oled.setDrawColor(0); oled.drawPixel(cx-1, cy-5); oled.setDrawColor(1);
      oled.drawBox(cx-3, cy-1, 2, 5); oled.drawBox(cx+1, cy-1, 2, 5);
      oled.drawHLine(cx-9, cy+7, 8); oled.drawHLine(cx+2, cy+7, 8);
      oled.drawFrame(cx+7, cy-11, 4, 4);
      break;
    }
    case APP_FILE_MANAGER: {
      oled.drawRBox(cx-12,cy-7,12,5,2); oled.drawRBox(cx-12,cy-3,24,15,2);
      oled.setDrawColor(0); oled.drawFrame(cx-7,cy+1,14,8); oled.setDrawColor(1);
      oled.drawFrame(cx-7,cy+1,14,8); oled.drawVLine(cx-2,cy+3,4); oled.drawVLine(cx+2,cy+3,4);
      break;
    }
  }
}

// ============ КЛАВИАТУРА ============
const char* keysEN[] = {"q","w","e","r","t","y","u","i","o","p","a","s","d","f","g","h","j","k","l","z","x","c","v","b","n","m"};
const char* keysRU[] = {"й","ц","у","к","е","н","г","ш","щ","з","х","ъ","ф","ы","в","а","п","р","о","л","д","ж","э","я","ч","с","м","и","т","ь","б","ю","ё"};
const char* keysUA[] = {"й","ц","у","к","е","н","г","ш","щ","з","х","ї","ф","і","в","а","п","р","о","л","д","ж","є","я","ч","с","м","и","т","ь","б","ю"};
const char* keysNUM[] = {"1","2","3","4","5","6","7","8","9","0","-","+","=","/","\\","(",")","[","]","!","?","@",":",";","'","\"","_","&","%","#","*",".",","};
const char* ACT_LABEL[5] = {"SP","CAP","DEL","123","OK"};

int kbLayout=0; bool kbUpper=false; bool kbNumbers=false; int kbSel=0; String geminiDraft;

int kbKeyCount() {
  if (kbNumbers) return sizeof(keysNUM)/sizeof(keysNUM[0]);
  if (kbLayout==1) return sizeof(keysRU)/sizeof(keysRU[0]);
  if (kbLayout==2) return sizeof(keysUA)/sizeof(keysUA[0]);
  return sizeof(keysEN)/sizeof(keysEN[0]);
}
int kbCharRows() { return (kbKeyCount()+9)/10; }
const char* kbKey(int i) {
  if (kbNumbers) return keysNUM[i];
  if (kbLayout==1) return keysRU[i];
  if (kbLayout==2) return keysUA[i];
  return keysEN[i];
}
String upperKey(String s) {
  if (s.length()==1 && s[0]>='a' && s[0]<='z') s[0]-=32;
  return s;
}
void dropFirstChar(String& s) {
  if (!s.length()) return;
  unsigned int i=1;
  while (i<s.length() && (((uint8_t)s[i])&0xC0)==0x80) i++;
  s.remove(0,i);
}
void dropLastChar(String& s) {
  if (!s.length()) return;
  int i = s.length()-1;
  while (i>0 && (((uint8_t)s[i])&0xC0)==0x80) i--;
  s.remove(i);
}
void kbPos(int idx, int& r, int& c) {
  int n = kbKeyCount();
  if (idx < n) { r = idx / 10; c = idx % 10; }
  else { r = kbCharRows(); c = (idx - n) * 2; }
}

int kbAt(int r, int c) {
  int n = kbKeyCount();
  int rows = kbCharRows();
  if (r >= rows) {
    int a = c / 2;
    if (a < 0) a = 0;
    if (a >= kbActionCount) a = kbActionCount - 1;
    return n + a;
  }
  int len = min(10, n - r * 10);
  if (c >= len) c = len - 1;
  if (c < 0) c = 0;
  return r * 10 + c;
}

void kbMove(int dr, int dc) {
  int n = kbKeyCount();
  int rows = kbCharRows();
  int r, c;
  kbPos(kbSel, r, c);
  if (dc != 0) {
    if (r >= rows) {
      int a = kbSel - n;
      a = (a + dc + kbActionCount) % kbActionCount;
      kbSel = n + a;
    } else {
      int len = min(10, n - r * 10);
      c = (c + dc + len) % len;
      kbSel = r * 10 + c;
    }
  }
  if (dr != 0) {
    int total = rows + 1;
    int nr = (r + dr + total) % total;
    kbSel = kbAt(nr, c);
  }
}

void drawKeyboard(const char* title, const String& text, bool secret, bool allowAttach = false) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x13_t_cyrillic);
  oled.drawUTF8(0, 12, title);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  const char* tag = kbNumbers ? "123" : (kbLayout == 0 ? "EN" : kbLayout == 1 ? "RU" : "UA");
  int tw = oled.getStrWidth(tag);
  oled.drawRBox(126 - tw - 6, 1, tw + 6, 10, 3);
  oled.setDrawColor(0); oled.drawStr(126 - tw - 3, 9, tag); oled.setDrawColor(1);
  oled.drawHLine(0, 14, 128);

  // Индикатор прикреплённого файла (справа от EN/RU/UA)
  if (allowAttach && pendingAttachPath.length() > 0) {
    oled.setFont(u8g2_font_4x6_tr);
    oled.drawStr(70, 9, "[SD+]");
    oled.setFont(u8g2_font_5x8_t_cyrillic);
  }

  String shown = "";
  if (secret && !cfg.showPass) { for (unsigned int i = 0; i < text.length(); i++) shown += '*'; }
  else shown = text;
  while (shown.length() && oled.getUTF8Width((shown + "_").c_str()) > 122) dropFirstChar(shown);
  bool cursorOn = (millis() / 400) % 2 == 0;
  oled.drawUTF8(2, 24, (shown + (cursorOn ? "_" : " ")).c_str());

  int n = kbKeyCount();
  int actCount = allowAttach ? 6 : 5;
  int total = n + actCount;

  for (int idx = 0; idx < total; idx++) {
    int r, c;
    kbPos(idx, r, c);
    int y = 33 + r * 7;
    int bx, bw;
    String label;
    if (idx < n) {
      bx = c * 12 + 1; bw = 11;
      label = kbKey(idx);
      if (kbUpper && !kbNumbers) label = upperKey(label);
    } else {
      int a = idx - n;
      if (allowAttach) {
        bx = 1 + a * 21; bw = 20;
        static const char* lbl[] = {"SP", "CAP", "DEL", "123", "SD", "OK"};
        label = lbl[a];
      } else {
        bx = 1 + a * 26; bw = 25;
        label = ACT_LABEL[a];
      }
    }
    int tx = bx + (bw - oled.getUTF8Width(label.c_str())) / 2;
    if (idx == kbSel) {
      oled.drawRBox(bx, y - 6, bw, 8, 2);
      oled.setDrawColor(0);
      oled.drawUTF8(tx, y, label.c_str());
      oled.setDrawColor(1);
    } else {
      oled.drawUTF8(tx, y, label.c_str());
    }
  }
  present();
}

bool runKeyboard(const char* title, String& text, bool secret, bool allowAttach = false) {
  waitRelease();
  kbLayout = cfg.layout; kbSel = 0; kbNumbers = false;
  kbActionCount = allowAttach ? 6 : 5;
  if (!allowAttach) pendingAttachPath = "";

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evLong[BI_BACK]) { kbActionCount = 5; return false; }
    if (nav(BI_UP)) kbMove(-1, 0);
    if (nav(BI_DOWN)) kbMove(1, 0);
    if (nav(BI_LEFT)) { kbMove(0, -1); SFX(SF_MOVE); }
    if (nav(BI_RIGHT)) { kbMove(0, 1); SFX(SF_MOVE); }

    if (evDown[BI_OK]) {
      int n = kbKeyCount();
      if (kbSel < n) {
        String k = kbKey(kbSel);
        if (kbUpper && !kbNumbers) k = upperKey(k);
        if (text.length() < 300) text += k;
        SFX(SF_CLICK);
      } else {
        int a = kbSel - n;
        if (allowAttach) {
          switch (a) {
            case 0: text += " "; break;
            case 1: kbUpper = !kbUpper; break;
            case 2: dropLastChar(text); break;
            case 3: kbNumbers = !kbNumbers; kbSel = 0; break;
            case 4: {
              // Открываем выбор файла с SD
              kbActionCount = 5; // временно, чтобы файловый менеджер работал с 5-кнопочным кодом если нужно
              String p = chooseSdFile();
              kbActionCount = 6;
              if (p.length() > 0) pendingAttachPath = p;
              break;
            }
            case 5: kbActionCount = 5; return true;
          }
        } else {
          switch (a) {
            case 0: text += " "; break;
            case 1: kbUpper = !kbUpper; break;
            case 2: dropLastChar(text); break;
            case 3: kbNumbers = !kbNumbers; kbSel = 0; break;
            case 4: return true;
          }
        }
      }
    }
    if (evTap[BI_BACK]) {
      if (kbNumbers) kbNumbers = false;
      else kbLayout = (kbLayout + 1) % 3;
      kbSel = 0;
      SFX(SF_OPEN);
    }
    drawKeyboard(title, text, secret, allowAttach);
    frameWait(t0);
  }
}
String base64Encode(const uint8_t* data, size_t len) {
  size_t outLen = 0;
  mbedtls_base64_encode(NULL, 0, &outLen, data, len);
  String out; out.reserve(outLen + 1);
  uint8_t* buf = (uint8_t*)malloc(outLen + 1);
  if (buf && mbedtls_base64_encode(buf, outLen + 1, &outLen, data, len) == 0) {
    buf[outLen] = 0;
    out = (char*)buf;
  }
  free(buf);
  return out;
}
// ============ GEMINI ============
String askAI(const String& prompt,
             const uint8_t* imgData = nullptr,
             size_t imgLen = 0,
             const String& mimeType = "") {
  if (WiFi.status()!=WL_CONNECTED) return "Wi-Fi offline.";
  WiFiClientSecure client; client.setInsecure();
  HTTPClient https; https.setTimeout(60000);
  if (!https.begin(client, API_URL)) return "HTTPS init failed.";
  https.addHeader("Content-Type","application/json");
  https.addHeader("x-goog-api-key", GEMINI_API_KEY);
  JsonDocument request;
  JsonArray contents = request["contents"].to<JsonArray>();
  JsonObject content = contents.add<JsonObject>();
  JsonArray parts = content["parts"].to<JsonArray>();
  parts.add<JsonObject>()["text"] = prompt;

  // Если передана картинка — добавляем второй part с inline_data
  if (imgData && imgLen > 0 && mimeType.length()) {
    JsonObject inlineData = parts.add<JsonObject>();
    inlineData["inline_data"]["mime_type"] = mimeType;
    inlineData["inline_data"]["data"] = base64Encode(imgData, imgLen);
  }

  String body; serializeJson(request, body);
  String response; int status = -1;
  for (int attempt=0; attempt<3; attempt++) {
    status = https.POST(body);
    response = status>0 ? https.getString() : "";
    if (status==200) break;
    bool retry = status==429 || (status>=500 && status<=504) || status<0;
    if (!retry || attempt==2) break;
    https.end(); delay(1000UL<<attempt);
    if (!https.begin(client, API_URL)) return "HTTPS init failed.";
    https.addHeader("Content-Type","application/json");
    https.addHeader("x-goog-api-key", GEMINI_API_KEY);
  }
  https.end();
  if (status!=200) return "Gemini HTTP error "+String(status);
  JsonDocument result;
  DeserializationError err = deserializeJson(result, response);
  if (err) return "Bad JSON from Gemini.";
  const char* answer = result["candidates"][0]["content"]["parts"][0]["text"];
  if (!answer) return "No text in response.";
  return String(answer);
}
const int MAX_LINES = 120;
String answerLines[MAX_LINES];
int wrapText(const String& src) {
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  String t = src; t.replace('\n',' ');
  int n=0; String line=""; int start=0;
  while (n<MAX_LINES) {
    int sp = t.indexOf(' ',start);
    String w = sp<0 ? t.substring(start) : t.substring(start,sp);
    if (w.length()) {
      String cand = line.length()?line+" "+w:w;
      if (oled.getUTF8Width(cand.c_str())<=120) line=cand;
      else { if (line.length()) { answerLines[n++]=line; line=w; } else { answerLines[n++]=w; line=""; } }
    }
    if (sp<0) break; start = sp+1;
  }
  if (line.length() && n<MAX_LINES) answerLines[n++]=line;
  return n;
}
bool showAnswer(const String& text) {
  int n = wrapText(text); int top = 0;
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evLong[BI_BACK]) return false;
    if (evTap[BI_BACK] || evDown[BI_OK]) return true;
    int maxTop = max(0, n-5);
    if (nav(BI_UP) && top>0) top--;
    if (nav(BI_DOWN) && top<maxTop) top++;
    if (nav(BI_LEFT)) top = max(0, top-6);
    if (nav(BI_RIGHT)) top = min(maxTop, top+6);
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawUTF8(0,12,"Gemini");
    drawSparkle(118,7,4);
    oled.drawHLine(0,16,128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i=0;i<6;i++) {
      int idx = top+i;
      if (idx>=n) break;
      oled.drawUTF8(3,26+i*7,answerLines[idx].c_str());
    }
    if (n>6) {
      int track=42; int thumb=max(6,track*6/n);
      int ty = 18 + (track-thumb)*top/max(1,maxTop);
      oled.drawVLine(125,18,track); oled.drawBox(124,ty,3,thumb);
    }
    present(); frameWait(t0);
  }
}
String chooseSdFile() {
  if (!sdReady) { showScreen("SD missing",""); waitAnyKey(); return ""; }
  String path = "/";
  int sel = 0;
  struct FE { String name, full; bool dir; uint32_t sz; };
  static FE items[40];
  int count = 0;

  auto scan = [&]() {
    count = 0;
    if (path != "/") items[count++] = {"..", parentPath(path), true, 0};
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return; }
    File f = dir.openNextFile();
    while (f && count < 40) {
      String nm = String(f.name());
      String low = nm; low.toLowerCase();
      bool isDir = f.isDirectory();
      bool sup = isDir || low.endsWith(".txt") || low.endsWith(".png")
                 || low.endsWith(".jpg") || low.endsWith(".jpeg")
                 || low.endsWith(".ino") || low.endsWith(".cfg")
                 || low.endsWith(".json") || low.endsWith(".h") || low.endsWith(".cpp");
      if (sup) {
        String full = isDir ? nm : (nm.startsWith("/") ? nm : (path=="/"?"/"+nm:path+"/"+nm));
        items[count++] = {baseName(nm), full, isDir, (uint32_t)f.size()};
      }
      f.close(); f = dir.openNextFile();
    }
    if (f) f.close(); dir.close();
    if (sel >= count) sel = count ? count-1 : 0;
  };
  scan();
  waitRelease();
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evTap[BI_BACK] || evLong[BI_BACK]) return "";
    if (count == 0) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x13_t_cyrillic);
      oled.drawStr(0,12,"No files");
      oled.setFont(u8g2_font_5x8_t_cyrillic);
      oled.drawStr(0,30,path.c_str());
      oled.drawStr(0,50,"BACK: cancel");
      present(); frameWait(t0);
      continue;
    }
    if (nav(BI_UP)) sel = (sel + count - 1) % count;
    if (nav(BI_DOWN)) sel = (sel + 1) % count;
    if (evDown[BI_OK]) {
      FE& e = items[sel];
      if (e.dir) {
        path = e.full; if (!path.startsWith("/")) path = "/" + path;
        sel = 0; scan(); waitRelease(); continue;
      }
      return e.full;
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 11, "Pick file");
    oled.drawHLine(0, 14, 128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    String pt = path; if (pt.length() > 20) pt = "..." + pt.substring(pt.length()-17);
    oled.drawStr(0, 23, pt.c_str());
    for (int r = 0; r < 4; r++) {
      int idx = (sel/4)*4 + r;
      if (idx >= count) break;
      int y = 32 + r*8;
      if (idx == sel) { oled.drawRBox(0, y-6, 128, 8, 2); oled.setDrawColor(0); }
      String nm = items[idx].dir ? ("[" + items[idx].name + "]") : items[idx].name;
      if (nm.length() > 21) nm = nm.substring(0, 21);
      oled.drawStr(2, y, nm.c_str());
      if (idx == sel) oled.setDrawColor(1);
    }
    present(); frameWait(t0);
  }
}
void runGemini() {
  while (true) {
    pendingAttachPath = "";
    if (!runKeyboard("Gemini Chat", geminiDraft, false, true)) return;
    if (!geminiDraft.length() && !pendingAttachPath.length()) continue;

    String answer;
    if (pendingAttachPath.length() > 0) {
      String low = pendingAttachPath; low.toLowerCase();
      if (low.endsWith(".png") || low.endsWith(".jpg") || low.endsWith(".jpeg")) {
        File f = SD.open(pendingAttachPath);
        if (!f) { showScreen("Error", "Cannot open image", ""); waitAnyKey(); continue; }
        size_t sz = f.size();
        if (sz > 30000) { f.close(); showScreen("Too big", "Max 30 KB", ""); waitAnyKey(); continue; }
        uint8_t* buf = (uint8_t*)malloc(sz);
        if (!buf) { f.close(); showScreen("OOM", ""); waitAnyKey(); continue; }
        f.read(buf, sz); f.close();
        String mime = low.endsWith(".png") ? "image/png" : "image/jpeg";
        showScreen("Gemini", "Analyzing...");
        answer = askAI(geminiDraft, buf, sz, mime);
        free(buf);
      } else {
        File f = SD.open(pendingAttachPath);
        if (!f) { showScreen("Error", "Cannot open file", ""); waitAnyKey(); continue; }
        String fileText;
        while (f.available() && fileText.length() < 4000) fileText += (char)f.read();
        f.close();
        showScreen("Gemini", "Thinking...");
        answer = askAI(geminiDraft + "\n\n--- File ---\n" + fileText);
      }
    } else {
      showScreen("Gemini", "Thinking...");
      answer = askAI(geminiDraft);
    }
    showAnswer(answer);
  }
}

// ============ WI-FI ============
bool connectWifi(const String& ssid, const String& pass) {
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t t0 = millis();
  bool ok=false, cancelled=false;
  while (millis()-t0<15000) {
    pollButtons();
    if (quitPressed()) { cancelled=true; break; }
    if (WiFi.status()==WL_CONNECTED) { ok=true; break; }
    String dots=""; int d=(millis()/350)%4;
    for (int i=0;i<d;i++) dots += '.';
    showScreen("Connecting", ssid.substring(0,20), "Please wait"+dots);
    delay(60);
  }
  if (ok) {
    prefs.putString("ssid",ssid); prefs.putString("pass",pass);
    showScreen("Connected", ssid.substring(0,20), WiFi.localIP().toString());
    delay(1500);
  } else {
    WiFi.disconnect(false);
    showScreen(cancelled?"Cancelled":"Failed","Check password","or signal");
    delay(1500);
  }
  return ok;
}
void runWifiScan() {
  waitRelease();
  showScreen("Wi-Fi","Scanning...");
  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();
  netCount = 0;
  for (int i=0;i<n && netCount<16;i++) {
    String s = WiFi.SSID(i);
    if (!s.length()) continue;
    nets[netCount].ssid = s; nets[netCount].rssi = WiFi.RSSI(i);
    nets[netCount].secure = WiFi.encryptionType(i)!=WIFI_AUTH_OPEN;
    netCount++;
  }
  WiFi.scanDelete();
  for (int i=0;i<netCount;i++) for (int j=i+1;j<netCount;j++)
    if (nets[j].rssi>nets[i].rssi) { NetInfo t=nets[i]; nets[i]=nets[j]; nets[j]=t; }
  const int ROWS=4; int sel=0, top=0;
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) return;
    if (nav(BI_UP) && sel>0) sel--;
    if (nav(BI_DOWN) && sel<netCount-1) sel++;
    if (sel<top) top=sel;
    if (sel>=top+ROWS) top=sel-ROWS+1;
    if (evDown[BI_OK] && netCount>0) {
      NetInfo& nt = nets[sel];
      String pass=""; bool go=true;
      if (nt.secure) go = runKeyboard("Password", pass, true);
      if (go && connectWifi(nt.ssid,pass)) return;
      waitRelease(); continue;
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawUTF8(0,12,"Wi-Fi");
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    String cnt = String(netCount)+" found";
    oled.drawStr(128-oled.getStrWidth(cnt.c_str()),11,cnt.c_str());
    oled.drawHLine(0,15,128);
    if (netCount==0) oled.drawStr(4,36,"No networks");
    for (int i=0;i<ROWS;i++) {
      int idx = top+i; if (idx>=netCount) break;
      int y = 18+i*11;
      NetInfo& nt = nets[idx];
      bool s = idx==sel;
      if (s) { oled.drawRBox(1,y-1,126,10,2); oled.setDrawColor(0); }
      String name = nt.ssid;
      while (oled.getUTF8Width(name.c_str())>84 && name.length()) dropLastChar(name);
      oled.drawUTF8(5,y+7,name.c_str());
      drawBars(106,y,rssiLevel(nt.rssi));
      if (nt.secure) drawLock(116,y+1);
      oled.setDrawColor(1);
    }
    present(); frameWait(t0);
  }
}
void reconnectSaved() {
  String s = prefs.getString("ssid","");
  if (!s.length()) { showScreen("Wi-Fi","No saved network"); delay(1200); return; }
  connectWifi(s, prefs.getString("pass",""));
}
void forgetNetwork() {
  prefs.remove("ssid"); prefs.remove("pass");
  WiFi.disconnect(false,true);
  showScreen("Wi-Fi","Network forgotten"); delay(1000);
}
void waitAnyKey() {
  waitRelease();
  while (true) {
    pollButtons();
    if (quitPressed() || evDown[BI_OK]) return;
    present(); delay(15);
  }
}
void showAbout() {
  float t = temperatureRead();
  String l1 = "WEB: http://192.168.4.1";
  String l2 = String("AP: ") + AP_NAME;
  String l3 = "CPU "+String(t,1)+" C / Heap "+String(ESP.getFreeHeap()/1024)+" KB";
  showScreen("About / File Manager", l1, l2, l3);
  waitAnyKey();
}
bool confirmBox(const char* title, const char* a, const char* b) {
  waitRelease();
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evDown[BI_OK]) return true;
    if (quitPressed()) return false;
    showScreen(title,a,b,"OK: yes   BACK: no");
    frameWait(t0);
  }
}

// ============ RECORDS (с переключением языка по OK) ============
void runRecords() {
  waitRelease();
  int sel=0, top=0;
  const int ROWS=5;
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) return;
    // OK cycles language
    if (evDown[BI_OK]) {
      cfg.lang = (cfg.lang+1)%3;
      saveCfg();
      SFX(SF_CLICK);
      continue;
    }
    if (nav(BI_UP)) sel = (sel+REC_N-1)%REC_N;
    if (nav(BI_DOWN)) sel = (sel+1)%REC_N;
    if (sel<top) top=sel;
    if (sel>=top+ROWS) top=sel-ROWS+1;
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    const char* lngTag = cfg.lang==0?"EN":cfg.lang==1?"RU":"UA";
    oled.drawStr(0,12,"Records");
    oled.setFont(u8g2_font_4x6_tr);
    oled.drawStr(68,9,lngTag);
    oled.drawStr(92,9,"BEST");
    oled.drawStr(118,9,"RUN");
    oled.drawHLine(0,16,128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i=0;i<ROWS;i++) {
      int idx = top+i; if (idx>=REC_N) break;
      int y = 27+i*9;
      if (idx==sel) { oled.drawRBox(0,y-7,128,9,2); oled.setDrawColor(0); }
      oled.drawUTF8(3,y,appName(idx));
      String best = idx==APP_GEMINI ? String("-") : String(getBest(idx));
      oled.drawStr(104-oled.getStrWidth(best.c_str()),y,best.c_str());
      String runs = String(getPlays(idx));
      oled.drawStr(125-oled.getStrWidth(runs.c_str()),y,runs.c_str());
      oled.setDrawColor(1);
    }
    time_t bestTs = (time_t)prefs.getLong64((String("d")+String(sel)).c_str(),0);
    if (bestTs>1700000000) {
      struct tm bt;
      if (localtime_r(&bestTs,&bt)) {
        char dateLine[24];
        snprintf(dateLine,sizeof(dateLine),"%02d.%02d.%04d",bt.tm_mday,bt.tm_mon+1,bt.tm_year+1900);
        oled.setDrawColor(0); oled.drawBox(0,54,128,10); oled.setDrawColor(1);
        oled.drawStr(2,62,dateLine);
      }
    }
    present(); frameWait(t0);
  }
}
void resetScores() {
  if (!confirmBox("Reset scores","Clear all best","scores?")) return;
  for (int id=0;id<REC_N;id++) {
    char k[8]; snprintf(k,sizeof k,"b%d",id); prefs.remove(k);
    char dk[8]; snprintf(dk,sizeof dk,"d%d",id); prefs.remove(dk);
  }
  prefs.remove("ttW"); prefs.remove("ttL"); prefs.remove("ttD");
  ttW=ttL=ttD=0;
  showScreen("Scores","Best scores cleared"); delay(900);
}

// ============ ДОКУМЕНТАЦИЯ ИГРЫ ============
String docLines[24];
int wrapDocText(const char* src) {
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  String t = String(src);
  t.replace('\n',' ');
  int n=0; String line=""; int start=0;
  while (n<24) {
    int sp = t.indexOf(' ',start);
    String w = sp<0 ? t.substring(start) : t.substring(start,sp);
    if (w.length()) {
      String cand = line.length()?line+" "+w:w;
      if (oled.getUTF8Width(cand.c_str())<=120) line=cand;
      else { if (line.length()) { docLines[n++]=line; line=w; } else { docLines[n++]=w; line=""; } }
    }
    if (sp<0) break; start = sp+1;
  }
  if (line.length() && n<24) docLines[n++]=line;
  return n;
}

void showGameDocs(int id) {
  if (id == APP_FILE_MANAGER) { showScreen("File Manager", "OK: open", "Wi-Fi: MINI-BOY-FILES", "192.168.4.1"); waitAnyKey(); return; }
  if (id < 0 || id >= APP_COUNT) return;
  waitRelease();
  const char* texts[3] = { GAME_DOCS_EN[id], GAME_DOCS_RU[id], GAME_DOCS_UA[id] };
  int n = wrapDocText(texts[cfg.lang]);
  int top = 0;
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    // OK closes help. A short press on button 6 cycles EN/RU/UA; holding it
    // for 2.5 seconds exits the help screen without changing the language.
    if (evTap[BI_OK] || evLong[BI_BACK]) return;
    if (evTap[BI_BACK]) {
      cfg.lang = (cfg.lang + 1) % 3;
      saveCfg();
      n = wrapDocText(texts[cfg.lang]);
      top = 0;
      if (cfg.sfx) SFX(SF_CLICK);
    }

    int maxTop = max(0, n-5);
    if (nav(BI_UP) && top>0) top--;
    if (nav(BI_DOWN) && top<maxTop) top++;

    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    String title = String(appName(id));
    while (oled.getUTF8Width(title.c_str()) > 100 && title.length() > 0) {
      title.remove(title.length()-1);
    }
    oled.drawUTF8(0,12,title.c_str());
    oled.setFont(u8g2_font_4x6_tr);
    const char* lngTag = cfg.lang==0?"EN":cfg.lang==1?"RU":"UA";
    oled.drawStr(120,9,lngTag);
    oled.drawHLine(0,16,128);
    oled.setFont(u8g2_font_4x6_tr);
    oled.drawStr(2,63,"OK:close  BTN6:language (hold:exit)");

    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i=0;i<5;i++) {
      int idx = top+i; if (idx>=n) break;
      oled.drawUTF8(3,26+i*7, docLines[idx].c_str());
    }
    if (n>5) {
      int track=34; int thumb=max(6, track*5/n);
      int ty = 18 + (track-thumb)*top/max(1,maxTop);
      oled.drawVLine(125,18,track);
      oled.drawBox(124,ty,3,thumb);
    }
    present(); frameWait(t0);
  }
}

const char* SET_NAMES[SET_N] = {
  "Scan Wi-Fi","Reconnect","Forget network","Show password","Brightness",
  "Invert display","Keyboard layout","Button speed","Sound effects",
  "Music theme","Volume","Show FPS","Auto-connect","Difficulty",
  "Animation speed","Show clock","Clock format","Auto time sync",
  "Set time/date","Sync time Wi-Fi","Records","Reset scores",
  "System info","About","SD card status","Boot logo","File type hints","Settings wrap",
  "Button Calibration","Reset Buttons","Factory Reset","Update via SD","Update via Web",
  "Store URL",
  "BACK"
};
const char* THEME_NAMES[THEME_COUNT] = {"OFF","TECH","CHILL","DARK","OLDSCHOOL"};
String setValue(int i) {
  switch (i) {
    case 3: return cfg.showPass?"ON":"OFF";
    case 4: return String(cfg.bright+1)+"/4";
    case 5: return cfg.invert?"ON":"OFF";
    case 6: return cfg.layout==0?"EN":cfg.layout==1?"RU":"UA";
    case 7: return cfg.rep==0?"FAST":cfg.rep==1?"NORM":"SLOW";
    case 8: return cfg.sfx?"ON":"OFF";
    case 9: return THEME_NAMES[cfg.musicTheme%THEME_COUNT];
    case 10: return String(cfg.volPercent)+"%";
    case 11: return cfg.showFps?"ON":"OFF";
    case 12: return cfg.autoConn?"ON":"OFF";
    case 13: return cfg.difficulty==0?"EASY":cfg.difficulty==1?"NORMAL":"HARD";
    case 14: return cfg.animSpeed==0?"SLOW":cfg.animSpeed==1?"NORMAL":"FAST";
    case 15: return cfg.showClock?"ON":"OFF";
    case 16: return cfg.clock24h?"24H":"12H";
    case 17: return cfg.autoTime?"ON":"OFF";
    case 24: return sdReady?"READY":"NO SD";
    case 25: return bootLogoFromSD ? "SD CARD" : "DEFAULT";
    case 26: return cfg.showFileTypes ? "ON" : "OFF";
    case 27: return cfg.settingsWrap ? "ON" : "OFF";
  }
  return "";
}
void setToggle(int i) {
  switch (i) {
    case 3: cfg.showPass=!cfg.showPass; break;
    case 4: cfg.bright=(cfg.bright+1)&3; applyDisplay(); break;
    case 5: cfg.invert=!cfg.invert; applyDisplay(); break;
    case 6: cfg.layout=(cfg.layout+1)%3; break;
    case 7: cfg.rep=(cfg.rep+1)%3; break;
    case 8: cfg.sfx=!cfg.sfx; break;
    case 9: cfg.musicTheme=(cfg.musicTheme+1)%THEME_COUNT; break;
    case 10: cfg.volPercent = cfg.volPercent >= 100 ? 0 : cfg.volPercent + 1; break;
    case 11: cfg.showFps=!cfg.showFps; break;
    case 12: cfg.autoConn=!cfg.autoConn; break;
    case 13: cfg.difficulty=(cfg.difficulty+1)%3; break;
    case 14: cfg.animSpeed=(cfg.animSpeed+1)%3; break;
    case 15: cfg.showClock=!cfg.showClock; break;
    case 16: cfg.clock24h=!cfg.clock24h; break;
    case 17: cfg.autoTime=!cfg.autoTime; clockConfigured=false; break;
    case 24: showScreen("SD card status", sdReady?"SD ready":"SD not detected", "Wi-Fi: MINI-BOY-FILES", "192.168.4.1"); waitAnyKey(); break;
    case 25: bootLogoFromSD = !bootLogoFromSD; prefs.putBool("bootlogo_sd", bootLogoFromSD); showScreen("Boot logo", bootLogoFromSD?"SD: /bootlogo.raw":"Default MINI-BOY", bootLogoFromSD && !SD.exists("/bootlogo.raw")?"File missing":"Saved", "Restart to apply"); waitAnyKey(); break;
    case 26: cfg.showFileTypes=!cfg.showFileTypes; break;
    case 27: cfg.settingsWrap=!cfg.settingsWrap; break;
    case 28: return;
  }
  saveCfg();
  if (cfg.sfx) SFX(SF_CLICK);
}
void adjustSetting(int i, int direction) {
  // Left/right adjusts the currently highlighted setting. Volume uses exact
  // 1-percent increments; PWM duty is mapped continuously in toneHz().
  switch (i) {
    case 3: cfg.showPass = direction>0; break;
    case 4: cfg.bright = (uint8_t)constrain((int)cfg.bright + direction, 0, 3); applyDisplay(); break;
    case 5: cfg.invert = direction>0; applyDisplay(); break;
    case 6: cfg.layout = (uint8_t)((cfg.layout + (direction>0?1:2)) % 3); break;
    case 7: cfg.rep = (uint8_t)constrain((int)cfg.rep + direction, 0, 2); break;
    case 8: cfg.sfx = direction>0; break;
    case 9: cfg.musicTheme = (uint8_t)((cfg.musicTheme + (direction>0?1:THEME_COUNT-1)) % THEME_COUNT); break;
    case 10: cfg.volPercent = (uint8_t)constrain((int)cfg.volPercent + direction, 0, 100); break;
    case 11: cfg.showFps = direction>0; break;
    case 12: cfg.autoConn = direction>0; break;
    case 13: cfg.difficulty = (uint8_t)constrain((int)cfg.difficulty + direction, 0, 2); break;
    case 14: cfg.animSpeed = (uint8_t)constrain((int)cfg.animSpeed + direction, 0, 2); break;
    case 15: cfg.showClock = direction>0; break;
    case 16: cfg.clock24h = direction>0; break;
    case 17: cfg.autoTime = direction>0; clockConfigured=false; break;
    case 25:
      if (direction>0) bootLogoFromSD=true; else bootLogoFromSD=false;
      prefs.putBool("bootlogo_sd", bootLogoFromSD);
      break;
    case 26: cfg.showFileTypes = direction>0; break;
    case 27: cfg.settingsWrap = direction>0; break;
    default: return;
  }
  saveCfg();
  if (cfg.sfx) SFX(SF_CLICK);
}

void editClockManually() {
  time_t now = time(nullptr);
  struct tm t;
  if (now>1700000000 && localtime_r(&now,&t)) { /* ok */ }
  else { memset(&t,0,sizeof(t)); t.tm_year=126; t.tm_mon=9; t.tm_mday=18; }
  setenv("TZ","CET-1CEST,M3.5.0/2,M10.5.0/3",1); tzset();
  int field = 0;
  waitRelease();
  while (true) {
    pollButtons();
    if (quitPressed()) return;
    if (evDown[BI_LEFT]) field = (field+5)%6;
    if (evDown[BI_RIGHT]) field = (field+1)%6;
    if (evDown[BI_UP] || evRep[BI_UP]) {
      if (field==0) t.tm_hour=(t.tm_hour+1)%24;
      if (field==1) t.tm_min=(t.tm_min+1)%60;
      if (field==2) t.tm_sec=(t.tm_sec+1)%60;
      if (field==3) t.tm_mday = t.tm_mday>=31?1:t.tm_mday+1;
      if (field==4) t.tm_mon=(t.tm_mon+1)%12;
      if (field==5) t.tm_year = t.tm_year>=150?126:t.tm_year+1;
    }
    if (evDown[BI_DOWN] || evRep[BI_DOWN]) {
      if (field==0) t.tm_hour=(t.tm_hour+23)%24;
      if (field==1) t.tm_min=(t.tm_min+59)%60;
      if (field==2) t.tm_sec=(t.tm_sec+59)%60;
      if (field==3) t.tm_mday = t.tm_mday<=1?31:t.tm_mday-1;
      if (field==4) t.tm_mon=(t.tm_mon+11)%12;
      if (field==5) t.tm_year = t.tm_year<=126?150:t.tm_year-1;
    }
    if (evDown[BI_OK]) {
      t.tm_isdst = -1;
      time_t stamp = mktime(&t);
      struct timeval tv = { .tv_sec=stamp, .tv_usec=0 };
      settimeofday(&tv,nullptr);
      rtcPresent = rtcWriteTime(stamp);
      prefs.putLong64("manualtime",(int64_t)stamp);
      clockConfigured = true;
      cfg.autoTime = false; saveCfg();
      showScreen("Time saved","Manual clock enabled","Auto sync OFF"); delay(900); return;
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0,12,"Set date/time");
    oled.drawHLine(0,16,128);
    char line1[24], line2[24];
    snprintf(line1,sizeof(line1),"%02d:%02d:%02d",t.tm_hour,t.tm_min,t.tm_sec);
    snprintf(line2,sizeof(line2),"%02d.%02d.%04d",t.tm_mday,t.tm_mon+1,t.tm_year+1900);
    oled.drawStr(16,34,line1); oled.drawStr(16,50,line2);
    int x = field<3 ? 16+field*18 : 16+(field-3)*18;
    int y = field<3 ? 36 : 52;
    oled.drawHLine(x,y,12);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,"L/R field OK save");
    present(); delay(80);
  }
}
void syncClockNow() {
  if (WiFi.status()!=WL_CONNECTED) {
    showScreen("Time sync","Wi-Fi not connected"); delay(1000); return;
  }
  setenv("TZ","CET-1CEST,M3.5.0/2,M10.5.0/3",1); tzset();
  configTime(0,0,"pool.ntp.org","time.google.com","time.cloudflare.com");
  clockConfigured = true;
  showScreen("Time sync","NTP requested","Wait for Wi-Fi time"); delay(1200);
}
void showSystemInfo() {
  waitRelease();
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed() || evDown[BI_OK]) return;
    time_t now = time(nullptr);
    struct tm lt;
    char timeLine[24] = "Clock: not set";
    if (now>1700000000 && localtime_r(&now,&lt))
      snprintf(timeLine,sizeof(timeLine),"%02d:%02d:%02d %02d.%02d.%04d",
               lt.tm_hour,lt.tm_min,lt.tm_sec,lt.tm_mday,lt.tm_mon+1,lt.tm_year+1900);
    float tempC = temperatureRead();
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0,11,"System info");
    oled.drawHLine(0,15,128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,23,(String("Chip: ")+ESP.getChipModel()).c_str());
    oled.drawStr(0,31,(String("CPU: ")+ESP.getCpuFreqMHz()+" MHz  "+String(tempC,1)+" C").c_str());
    oled.drawStr(0,39,(String("Heap: ")+ESP.getFreeHeap()/1024+"/"+ESP.getHeapSize()/1024+" KB").c_str());
    oled.drawStr(0,47,(String("Flash: ")+ESP.getFlashChipSize()/1024/1024+" MB").c_str());
    oled.drawStr(0,55,timeLine);
    oled.drawStr(0,63,(String(rtcPresent?"RTC OK":"RTC absent")+" "+
                     (WiFi.status()==WL_CONNECTED?String(WiFi.RSSI())+"dBm":"WiFi OFF")).c_str());
    present(); frameWait(t0);
  }
}
// Загрузить откалиброванные GPIO в BTN_PIN[]
void loadButtonGpio() {
  BTN_PIN[BI_UP] = cfg.btnGpioUp;
  BTN_PIN[BI_DOWN] = cfg.btnGpioDown;
  BTN_PIN[BI_LEFT] = cfg.btnGpioLeft;
  BTN_PIN[BI_RIGHT] = cfg.btnGpioRight;
  BTN_PIN[BI_OK] = cfg.btnGpioOk;
  BTN_PIN[BI_BACK] = cfg.btnGpioBack;
}
 
// Сохранить откалиброванные GPIO в NVS
void saveButtonGpio() {
  prefs.putUChar("btn_up", cfg.btnGpioUp);
  prefs.putUChar("btn_down", cfg.btnGpioDown);
  prefs.putUChar("btn_left", cfg.btnGpioLeft);
  prefs.putUChar("btn_right", cfg.btnGpioRight);
  prefs.putUChar("btn_ok", cfg.btnGpioOk);
  prefs.putUChar("btn_back", cfg.btnGpioBack);
}
 
// АДАПТИВНАЯ КАЛИБРОВКА КНОПОК
// Просит нажать каждую кнопку и автоматически определяет GPIO
void runButtonCalibration() {
  waitRelease();
  const char* btnNames[BI_COUNT] = {"UP", "DOWN", "LEFT", "RIGHT", "OK", "BACK"};
  uint8_t calibratedGpio[BI_COUNT];
  
  for (int btn = 0; btn < BI_COUNT; btn++) {
    waitRelease();
    uint32_t startTime = millis();
    uint8_t detectedGpio = 255;  // 255 = не определена
    
    while (millis() - startTime < 5000) {  // 5 секунд на нажатие
      uint32_t t0 = millis();
      pollButtons();
      
      // Ищем какой GPIO был нажат
      for (int i = 0; i < BI_COUNT; i++) {
        if (bst[i].stable && !bst[i].longDone) {  // Кнопка нажата
          detectedGpio = BTN_PIN[i];
          break;
        }
      }
      
      if (detectedGpio != 255) break;  // Обнаружили нажатие
      
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x13_t_cyrillic);
      oled.drawStr(0, 12, "Button Calibration");
      oled.drawHLine(0, 16, 128);
      
      oled.setFont(u8g2_font_5x8_t_cyrillic);
      oled.drawStr(2, 32, "Press button:");
      oled.setFont(u8g2_font_6x13_t_cyrillic);
      oled.drawStr(64 - oled.getStrWidth(btnNames[btn])/2, 50, btnNames[btn]);
      
      oled.setFont(u8g2_font_5x8_t_cyrillic);
      int timeLeft = 5 - (millis() - startTime) / 1000;
      oled.drawStr(2, 62, ("Timeout in " + String(timeLeft) + "s").c_str());
      
      present();
      frameWait(t0);
    }
    
    if (detectedGpio == 255) {
      // Timeout - не нажал кнопку
      showScreen("Calibration Failed", "Button not pressed!", "Try again", "");
      delay(1500);
      return;
    }
    
    calibratedGpio[btn] = detectedGpio;
    
    // Показываем что обнаружили
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 12, "Detected!");
    oled.drawHLine(0, 16, 128);
    
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2, 32, (String(btnNames[btn]) + " = GPIO" + String(detectedGpio)).c_str());
    oled.drawStr(2, 50, "Waiting for release...");
    
    present();
    waitRelease();
    delay(300);
  }
  
  // Сохраняем откалиброванные GPIO
  cfg.btnGpioUp = calibratedGpio[BI_UP];
  cfg.btnGpioDown = calibratedGpio[BI_DOWN];
  cfg.btnGpioLeft = calibratedGpio[BI_LEFT];
  cfg.btnGpioRight = calibratedGpio[BI_RIGHT];
  cfg.btnGpioOk = calibratedGpio[BI_OK];
  cfg.btnGpioBack = calibratedGpio[BI_BACK];
  
  // Обновляем BTN_PIN[] с откалиброванными значениями
  BTN_PIN[BI_UP] = cfg.btnGpioUp;
  BTN_PIN[BI_DOWN] = cfg.btnGpioDown;
  BTN_PIN[BI_LEFT] = cfg.btnGpioLeft;
  BTN_PIN[BI_RIGHT] = cfg.btnGpioRight;
  BTN_PIN[BI_OK] = cfg.btnGpioOk;
  BTN_PIN[BI_BACK] = cfg.btnGpioBack;
  
  saveButtonGpio();
  
  showScreen("Calibration Done!", "Settings saved!", "Device will restart", "in 2 seconds...");
  delay(2000);
  ESP.restart();
}
 
// СБРОС ТОЛЬКО КАЛИБРОВКИ КНОПОК
void resetButtonCalibration() {
  waitRelease();
  bool confirm = false;
  
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) return;
    
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 12, "Reset Buttons");
    oled.drawHLine(0, 16, 128);
    
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    
    if (!confirm) {
      oled.drawStr(2, 32, "Reset button config");
      oled.drawStr(2, 42, "to defaults?");
      oled.drawStr(2, 54, "OK: Yes    BACK: No");
      
      if (evDown[BI_OK]) confirm = true;
      if (evDown[BI_BACK]) return;
    } else {
      oled.drawStr(2, 32, "ARE YOU SURE?");
      oled.drawStr(2, 44, "LEFT: No    RIGHT: Yes");
      
      if (evDown[BI_LEFT]) return;
      if (evDown[BI_RIGHT]) {
        // Сбрасываем только калибровку кнопок
        cfg.btnGpioUp = DEFAULT_BTN_GPIO[BI_UP];
        cfg.btnGpioDown = DEFAULT_BTN_GPIO[BI_DOWN];
        cfg.btnGpioLeft = DEFAULT_BTN_GPIO[BI_LEFT];
        cfg.btnGpioRight = DEFAULT_BTN_GPIO[BI_RIGHT];
        cfg.btnGpioOk = DEFAULT_BTN_GPIO[BI_OK];
        cfg.btnGpioBack = DEFAULT_BTN_GPIO[BI_BACK];
        
        loadButtonGpio();
        saveButtonGpio();
        
        showScreen("Reset Complete", "Buttons to defaults", "Device will restart", "");
        delay(1500);
        ESP.restart();
      }
      if (evDown[BI_BACK]) return;
    }
    
    present();
    frameWait(t0);
  }
}
 
// FACTORY RESET (ВСЁ, включая все настройки)
void runFactoryReset() {
  waitRelease();
  bool confirm = false;
  
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) return;
    
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 12, "Factory Reset");
    oled.drawHLine(0, 16, 128);
    
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    
    if (!confirm) {
      oled.drawStr(2, 32, "Clear ALL data?");
      oled.drawStr(2, 42, "(WiFi, settings, scores)");
      oled.drawStr(2, 54, "OK: Yes    BACK: No");
      
      if (evDown[BI_OK]) confirm = true;
      if (evDown[BI_BACK]) return;
    } else {
      oled.drawStr(2, 24, "⚠️ FINAL CONFIRMATION");
      oled.drawStr(2, 36, "This will erase");
      oled.drawStr(2, 46, "ALL data permanently!");
      oled.drawStr(2, 56, "LEFT: No    RIGHT: Yes");
      
      if (evDown[BI_LEFT]) return;
      if (evDown[BI_RIGHT]) {
        // ПОЛНЫЙ сброс
        prefs.clear();
        
        showScreen("Factory Reset", "All data erased!", "Device will restart", "");
        delay(2000);
        ESP.restart();
      }
      if (evDown[BI_BACK]) return;
    }
    
    present();
    frameWait(t0);
  }
}

void editStoreUrl() {
  String url = prefs.getString("store_url", "http://192.168.178.22:8000");

  // Значение по умолчанию — подставь IP компьютера, если не задано
  if (!url.length()) url = "http://192.168.178.22:8000";

  if (!runKeyboard("Store URL", url, false)) return;

  url.trim();
  if (!url.length()) {
    prefs.remove("store_url");
    showScreen("Store URL", "Cleared", "Heartbeat disabled", "");
    waitAnyKey();
    return;
  }

  // Простая валидация
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    showScreen("Store URL", "Must start with", "http:// or https://", "BACK: cancel");
    waitAnyKey();
    return;
  }

  // Убираем trailing slash
  while (url.endsWith("/")) url.remove(url.length() - 1);

  prefs.putString("store_url", url);

  showScreen("Store URL saved", url, "Sending heartbeat...", "");
  sendHeartbeat();
  delay(500);
  showScreen("Store URL saved", url, "OK if server received it", "BACK: exit");
  waitAnyKey();
}

void runSettings() {
  waitRelease();
  int sel=0, top=0;
  const int ROWS=5;
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) return;
    if (nav(BI_UP)) sel = cfg.settingsWrap ? (sel+SET_N-1)%SET_N : max(0,sel-1);
    if (nav(BI_DOWN)) sel = cfg.settingsWrap ? (sel+1)%SET_N : min(SET_N-1,sel+1);
    if (sel<top) top=sel;
    if (sel>=top+ROWS) top=sel-ROWS+1;
    if (evDown[BI_LEFT]) { adjustSetting(sel, -1); }
    if (evDown[BI_RIGHT]) { adjustSetting(sel, +1); }
    if (evDown[BI_OK]) {
      switch (sel) {
        case 0: runWifiScan(); break;
        case 1: reconnectSaved(); break;
        case 2: forgetNetwork(); break;
        case 18: editClockManually(); break;
        case 19: syncClockNow(); break;
        case 20: runRecords(); break;
        case 21: resetScores(); break;
        case 22: showSystemInfo(); break;
        case 23: showAbout(); break;
        case 24: showScreen("SD card status", sdReady?"SD ready":"SD not detected", "Wi-Fi: MINI-BOY-FILES", "192.168.4.1"); waitAnyKey(); break;
        case 25: bootLogoFromSD = !bootLogoFromSD; prefs.putBool("bootlogo_sd", bootLogoFromSD); showScreen("Boot logo", bootLogoFromSD?"SD: /bootlogo.raw":"Default MINI-BOY", bootLogoFromSD && (!sdReady || !SD.exists("/bootlogo.raw"))?"File missing":"Saved", "Restart to apply"); waitAnyKey(); break;
        case 26: cfg.showFileTypes = !cfg.showFileTypes; prefs.putBool("showft", cfg.showFileTypes); break;
        case 27: cfg.settingsWrap = !cfg.settingsWrap; prefs.putBool("setwrap", cfg.settingsWrap); break;
        case 28: runButtonCalibration(); break;        // Адаптивная калибровка
        case 29: resetButtonCalibration(); break;      // Сброс только кнопок
        case 30: runFactoryReset(); break;             // Сброс ВСЕГО с подтверждением
        case 31: runSdFirmwareUpdate(); break;    
        case 32: runStore(); break;
        case 33: editStoreUrl(); waitRelease(); break;
        case 34: return;                    
        default: setToggle(sel); break;
      }
      if (sel<=2 || sel>=13) waitRelease();
      continue;
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawUTF8(0,12,"Settings");
    drawWifiStatus(116,2);
    oled.drawHLine(0,16,128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i=0;i<ROWS;i++) {
      int idx = top+i; if (idx>=SET_N) break;
      int y = 25+i*9;
      if (idx==sel) { oled.drawRBox(0,y-7,128,9,2); oled.setDrawColor(0); }
      oled.drawStr(4,y,SET_NAMES[idx]);
      String v = setValue(idx);
      if (v.length()) oled.drawStr(124-oled.getStrWidth(v.c_str()),y,v.c_str());
      oled.setDrawColor(1);
    }
    present(); frameWait(t0);
  }
}

// =====================================================
//  ИГРЫ
// =====================================================

// -------- Общие хелперы для игр --------
int obW(uint8_t t) { return t==3 ? 13 : (t==1?9:(t==2?6:5)); }
int obH(uint8_t t) { return t==3 ? 7 : (t==2?15:10); }
void drawCactus(int x, uint8_t t) {
  if (t==3) {
    // Small pterodactyl silhouette. Its height varies by animation frame.
    int y = GROUND_Y - 22 + ((millis()/180)%2 ? 2 : 0);
    oled.drawDisc(x+5,y,2); oled.drawTriangle(x+6,y-1,x+12,y,x+6,y+1);
    oled.drawLine(x+4,y,x+1,y-4); oled.drawLine(x+4,y,x+1,y+3);
    oled.drawLine(x+7,y+2,x+9,y+5); oled.drawPixel(x+5,y-1);
  } else if (t==0) {
    oled.drawRBox(x+1,GROUND_Y-10,3,10,1);
    oled.drawRBox(x,GROUND_Y-6,2,3,1);
    oled.drawRBox(x+3,GROUND_Y-8,2,3,1);
  } else if (t==1) {
    oled.drawRBox(x,GROUND_Y-10,3,10,1);
    oled.drawRBox(x+6,GROUND_Y-10,3,10,1);
    oled.drawBox(x+2,GROUND_Y-4,5,2);
  } else {
    oled.drawRBox(x+1,GROUND_Y-15,4,15,1);
    oled.drawRBox(x,GROUND_Y-9,2,4,1);
    oled.drawRBox(x+4,GROUND_Y-12,2,5,1);
  }
}
void drawDino(int x, int bottom, bool run, int frameN) {
  int top = bottom-14;
  oled.drawTriangle(x+2,top+8,x-1,top+5,x+2,top+11);
  oled.drawRBox(x+2,top+5,9,7,2);
  oled.drawRBox(x+5,top,8,7,2);
  oled.setDrawColor(0); oled.drawPixel(x+10,top+2); oled.setDrawColor(1);
  if (run && (frameN/3)%2==0) {
    oled.drawBox(x+3,top+11,3,3); oled.drawBox(x+8,top+12,3,2);
  } else if (run) {
    oled.drawBox(x+3,top+12,3,2); oled.drawBox(x+8,top+11,3,3);
  } else {
    oled.drawBox(x+3,top+11,3,3); oled.drawBox(x+8,top+11,3,3);
  }
}
void drawBird(int bx, int by, bool flap) {
  oled.drawDisc(bx,by,4);
  oled.setDrawColor(0); oled.drawPixel(bx+1,by-2);
  if (flap) oled.drawLine(bx-3,by,bx+1,by-3);
  else oled.drawLine(bx-3,by+1,bx+1,by+1);
  oled.setDrawColor(1);
  oled.drawTriangle(bx+3,by-2,bx+6,by,bx+3,by+2);
}
void drawPipe(int x, int gt, int gap, int pw) {
  int gb = gt+gap;
  oled.drawBox(x,0,pw,gt-4);
  oled.setDrawColor(0);
  for (int xx=x+3;xx<x+pw-2;xx+=3) oled.drawVLine(xx,1,gt-6);
  oled.setDrawColor(1);
  oled.drawRBox(x-2,gt-5,pw+4,5,1);
  oled.drawRBox(x-2,gb,pw+4,5,1);
  oled.drawBox(x,gb+5,pw,GROUND_Y-gb-5);
  oled.setDrawColor(0);
  for (int xx=x+3;xx<x+pw-2;xx+=3) oled.drawVLine(xx,gb+6,GROUND_Y-gb-7);
  oled.setDrawColor(1);
}

// -------- DINO --------
void runDino() {
  waitRelease();
  playMusicTheme(TECH_DINO,OLD_DINO,CHILL_DINO,DARK_DINO,TECH_DINO,110);
  const int DX=14;
  float dy=0, vy=0; bool onGround=true;
  float score=0, speed=2.2f, scroll=0;
  float cloudX[2]={30,95};
  bool obOn[3]={false,false,false};
  float obX[3]={0,0,0}; uint8_t obT[3]={0,0,0};
  float untilNext=60;
  bool previousWasPtero=false;
  bool dead=false; uint32_t deadAt=0;
  int best = getBest(APP_DINO);
  int frameN=0;
  bool base = cfg.invert, lastInv = cfg.invert;
  auto reset = [&]() {
    dy=0; vy=0; onGround=true; score=0; speed=2.2f;
    for (int i=0;i<3;i++) obOn[i]=false;
    untilNext=75; previousWasPtero=false; dead=false;
  };
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    frameN++;
    if (!dead) {
      uint8_t diff = difficultyFor(APP_DINO);
      float difficultyScale = diff==0 ? 0.82f : (diff==2 ? 1.12f : 1.0f);
      speed = min(4.8f, (2.2f + score/300.0f) * difficultyScale);
      if (onGround && (evDown[BI_OK] || evDown[BI_UP])) {
        vy=5.6f; onGround=false; SFX(SF_JUMP);
      }
      if (!onGround) {
        vy -= 0.42f; dy += vy;
        if (dy<=0) { dy=0; vy=0; onGround=true; }
      }
      scroll += speed;
      for (int c=0;c<2;c++) {
        cloudX[c] -= speed*0.3f;
        if (cloudX[c]<-14) cloudX[c]=140;
      }
      for (int i=0;i<3;i++) {
        if (!obOn[i]) continue;
        obX[i] -= speed;
        if (obX[i]<-12) obOn[i]=false;
      }
      untilNext -= speed;
      if (untilNext<=0) {
        for (int i=0;i<3;i++) {
          if (!obOn[i]) {
            obOn[i]=true; obX[i]=130;
            bool allowPtero = dinoPterodactyl && score >= dinoNightScore;
            bool choosePtero = allowPtero && random(100) < 28 && !previousWasPtero;
            obT[i] = choosePtero ? 3 : (uint8_t)random(3);
            previousWasPtero = choosePtero;
            break;
          }
        }
        // Keep a generous reaction window; after a flying enemy, add extra space.
        untilNext = (previousWasPtero ? random(105,145) : random(72,112)) + speed*14;
      }
      int dL=DX+2, dR=DX+10;
      int dB=GROUND_Y-(int)dy, dT=dB-12;
      for (int i=0;i<3;i++) {
        if (!obOn[i]) continue;
        int ox=(int)obX[i]; int w=obW(obT[i]), h=obH(obT[i]);
        bool hitX = dR>ox+1 && dL<ox+w-1;
        bool hitY = obT[i]==3 ? (dB>GROUND_Y-25 && dT<GROUND_Y-14)
                              : (dB>GROUND_Y-h+1 && dT<GROUND_Y);
        if (hitX && hitY) {
          dead=true; deadAt=millis();
          if ((int)score>best) best=(int)score;
          setBest(APP_DINO,(int)score);
          SFX(SF_DIE); break;
        }
      }
      score += speed*0.1f;
    } else if (evDown[BI_OK] && millis()-deadAt>600) reset();
    bool flash = dead && millis()-deadAt<720 && ((millis()-deadAt)/90)%2==0;
    bool night = ((int)score/dinoNightScore)%2==1;
    bool wantInv = (night!=flash)!=base;
    if (wantInv!=lastInv) { oled.sendF("c",wantInv?0xA7:0xA6); lastInv=wantInv; }
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    String hi = "HI "+padNum(best,5);
    String sc = padNum((int)score,5);
    oled.drawStr(0,7,hi.c_str());
    oled.drawStr(128-oled.getStrWidth(sc.c_str()),7,sc.c_str());
    drawCloud((int)cloudX[0],16); drawCloud((int)cloudX[1],24);
    oled.drawHLine(0,GROUND_Y+1,128);
    for (int i=0;i<10;i++) {
      int px = ((i*37-(int)scroll)%128+128)%128;
      oled.drawHLine(px,GROUND_Y+4,2+i%3);
    }
    for (int i=0;i<3;i++) if (obOn[i]) drawCactus((int)obX[i],obT[i]);
    drawDino(DX,GROUND_Y-(int)dy,onGround&&!dead,frameN);
    if (dead && millis()-deadAt>350) overlayBox("GAME OVER","OK: retry");
    present(); frameWait(t0);
  }
  applyDisplay();
}

// -------- FLAPPY (облегчён) --------
void runFlappy() {
  waitRelease();
  playMusicTheme(TECH_FLAPPY,OLD_FLAPPY,CHILL_FLAPPY,DARK_FLAPPY,TECH_FLAPPY,120);
  const int BX=30, PW=12, PSP=54;
  const int GAP=constrain(flappyGap + (difficultyFor(APP_FLAPPY)==0 ? 4 : (difficultyFor(APP_FLAPPY)==2 ? -4 : 0)), 20, 40);
  float by=28, vy=0, scroll=0;
  float px[3]; int gapTop[3]; bool passed[3];
  int score=0;
  int best = getBest(APP_FLAPPY);
  bool dead=false; uint32_t deadAt=0;
  int frameN=0;
  auto reset = [&]() {
    by=28; vy=0; score=0; dead=false;
    for (int i=0;i<3;i++) { px[i]=130+i*PSP; gapTop[i]=random(7,max(8,GROUND_Y-GAP-6)); passed[i]=false; }
  };
  reset();
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    frameN++;
    if (!dead) {
      if (evDown[BI_OK] || evDown[BI_UP]) { vy=-2.7f; SFX(SF_JUMP); }
      vy += 0.22f;
      if (vy>3.2f) vy=3.2f;
      by += vy;
      float speed = min(2.0f, (1.2f + score*0.03f) * (difficultyFor(APP_FLAPPY)==0 ? 0.88f : (difficultyFor(APP_FLAPPY)==2 ? 1.12f : 1.0f)));
      scroll += speed*0.5f;
      for (int i=0;i<3;i++) {
        px[i] -= speed;
        if (px[i] < -PW-2) {
          px[i] += 3*PSP;
          gapTop[i] = random(7,max(8,GROUND_Y-GAP-6));
          passed[i] = false;
        }
        if (!passed[i] && px[i]+PW < BX-4) { passed[i]=true; score++; SFX(SF_SCORE); }
        if (BX+4 > px[i] && BX-4 < px[i]+PW) {
          if (by-4 < gapTop[i] || by+4 > gapTop[i]+GAP) dead=true;
        }
      }
      if (by-4 < 0 || by+4 > GROUND_Y) dead=true;
      if (dead) { deadAt=millis(); if (score>best) best=score; setBest(APP_FLAPPY,score); SFX(SF_DIE); }
    } else if (evDown[BI_OK] && millis()-deadAt>500) reset();
    oled.clearBuffer();
    for (int c=0;c<2;c++) {
      int cx = 150 - ((int)(scroll*0.4f)+c*70)%170;
      drawCloud(cx, 18+c*12);
    }
    for (int i=0;i<3;i++) drawPipe((int)px[i],gapTop[i],GAP,PW);
    drawBird(BX,(int)by,(frameN/3)%2);
    oled.drawHLine(0,GROUND_Y,128);
    for (int x=-((int)scroll%6);x<128;x+=6) oled.drawLine(x,63,x+4,59);
    oled.setDrawColor(0); oled.drawBox(0,0,128,9); oled.setDrawColor(1);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    String hi = "BEST "+padNum(best,3);
    String sc = padNum(score,3);
    oled.drawStr(2,7,hi.c_str());
    oled.drawStr(126-oled.getStrWidth(sc.c_str()),7,sc.c_str());
    if (dead && millis()-deadAt>300)
      overlayBox("GAME OVER", ("Score "+String(score)).c_str());
    present(); frameWait(t0);
  }
}

// -------- TETRIS (полный код из v4, без изменений) --------
static uint8_t tBoard[16][10];
static uint8_t tBoardColor[16][10];
const uint8_t TET_SHAPES[7][4][4] = {
  { {0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0} },
  { {1,1,0,0},{1,1,0,0},{0,0,0,0},{0,0,0,0} },
  { {0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0} },
  { {0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0} },
  { {1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0} },
  { {1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0} },
  { {0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0} }
};
void tetrisClearBoard() {
  for (int y=0;y<16;y++) for (int x=0;x<10;x++) { tBoard[y][x]=0; tBoardColor[y][x]=0; }
}
bool tetrisCanPlace(const uint8_t shape[4][4], int x, int y) {
  for (int sy=0;sy<4;sy++) for (int sx=0;sx<4;sx++) {
    if (!shape[sy][sx]) continue;
    int bx=x+sx, by=y+sy;
    if (bx<0||bx>=10||by>=16) return false;
    if (by>=0 && tBoard[by][bx]) return false;
  }
  return true;
}
void tetrisRotateMatrix(const uint8_t input[4][4], uint8_t out[4][4]) {
  for (int y=0;y<4;y++) for (int x=0;x<4;x++) out[y][x]=input[3-x][y];
}
void tetrisGetCurrentShape(const TetrisPiece& p, uint8_t out[4][4]) {
  for (int y=0;y<4;y++) for (int x=0;x<4;x++) out[y][x]=TET_SHAPES[p.type][y][x];
  for (int r=0;r<(p.rot&3);r++) {
    uint8_t tmp[4][4]; tetrisRotateMatrix(out,tmp);
    for (int y=0;y<4;y++) for (int x=0;x<4;x++) out[y][x]=tmp[y][x];
  }
}
void tetrisSpawnPiece(TetrisPiece& p) {
  p.type = random(0,7); p.x = 3; p.y = -1; p.rot = 0;
}
void tetrisMergePiece(const uint8_t shape[4][4], int x, int y) {
  for (int sy=0;sy<4;sy++) for (int sx=0;sx<4;sx++) {
    if (!shape[sy][sx]) continue;
    int bx=x+sx, by=y+sy;
    if (by>=0 && by<16 && bx>=0 && bx<10) { tBoard[by][bx]=1; tBoardColor[by][bx]=1; }
  }
}
static const int TETRIS_CELL = 3;
static const int TETRIS_X = 4;
static const int TETRIS_Y = 12;
void tetrisDrawCell(int x, int y, bool active) {
  const int px = TETRIS_X + x*TETRIS_CELL;
  const int py = TETRIS_Y + y*TETRIS_CELL;
  if (active) oled.drawBox(px,py,TETRIS_CELL-1,TETRIS_CELL-1);
  else oled.drawFrame(px,py,TETRIS_CELL-1,TETRIS_CELL-1);
}
void tetrisDrawBoard() {
  oled.drawFrame(TETRIS_X-1,TETRIS_Y-1,10*TETRIS_CELL+2,16*TETRIS_CELL+2);
  for (int by=0;by<16;by++) for (int bx=0;bx<10;bx++) {
    if (tBoard[by][bx]) {
      int px = TETRIS_X + bx*TETRIS_CELL;
      int py = TETRIS_Y + by*TETRIS_CELL;
      oled.drawBox(px,py,TETRIS_CELL-1,TETRIS_CELL-1);
    }
  }
}
void tetrisDrawPiece(const TetrisPiece& p) {
  uint8_t shape[4][4];
  for (int y=0;y<4;y++) for (int x=0;x<4;x++) shape[y][x]=TET_SHAPES[p.type][y][x];
  for (int r=0;r<(p.rot&3);r++) {
    uint8_t tmp[4][4]; tetrisRotateMatrix(shape,tmp);
    for (int y=0;y<4;y++) for (int x=0;x<4;x++) shape[y][x]=tmp[y][x];
  }
  for (int y=0;y<4;y++) for (int x=0;x<4;x++) {
    if (!shape[y][x]) continue;
    int bx=p.x+x, by=p.y+y;
    if (by<0||by>=16||bx<0||bx>=10) continue;
    tetrisDrawCell(bx,by,true);
  }
}
bool tetrisTryRotate(TetrisPiece& p) {
  uint8_t rotated[4][4];
  tetrisGetCurrentShape(p,rotated);
  uint8_t tmp[4][4]; tetrisRotateMatrix(rotated,tmp);
  const int kicks[][2] = {{0,0},{-1,0},{1,0},{0,-1},{-2,0},{2,0}};
  for (uint8_t i=0;i<sizeof(kicks)/sizeof(kicks[0]);i++) {
    int nx = p.x + kicks[i][0];
    int ny = p.y + kicks[i][1];
    if (tetrisCanPlace(tmp,nx,ny)) {
      p.x=nx; p.y=ny; p.rot=(p.rot+1)&3; return true;
    }
  }
  return false;
}
int tetrisClearLines() {
  int cleared = 0;
  for (int y=15;y>=0;y--) {
    bool full = true;
    for (int x=0;x<10;x++) if (!tBoard[y][x]) { full=false; break; }
    if (!full) continue;
    for (int yy=y;yy>0;yy--) for (int x=0;x<10;x++) tBoard[yy][x]=tBoard[yy-1][x];
    for (int x=0;x<10;x++) tBoard[0][x]=0;
    cleared++; y++;
  }
  return cleared;
}
void tetrisDrawGameOver(int score, int best) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x13_t_cyrillic);
  oled.drawStr(20,16,"GAME OVER");
  oled.drawHLine(4,21,120);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  String scoreLine = "SCORE "+String(score);
  String bestLine = "BEST  "+String(best);
  oled.drawStr(18,34,scoreLine.c_str());
  oled.drawStr(18,44,bestLine.c_str());
  oled.drawStr(2,58,"OK: AGAIN  BACK: EXIT");
  present();
}
void runTetris() {
  waitRelease();
  playMusicTheme(TECH_TETRIS,OLD_TETRIS,CHILL_TETRIS,DARK_TETRIS,OLD_TETRIS,105);
  tetrisClearBoard();
  TetrisPiece cur; tetrisSpawnPiece(cur);
  int score=0; int best = getBest(APP_TETRIS); int level=1;
  bool gameOver=false;
  uint32_t lastDrop=millis(), lastStep=millis();
  uint8_t shape[4][4];
  tetrisGetCurrentShape(cur,shape);
  while (true) {
    const uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) { audioStop(); return; }
    if (gameOver) {
      tetrisDrawGameOver(score,best);
      if (evDown[BI_OK] || evTap[BI_OK]) {
        tetrisClearBoard();
        tetrisSpawnPiece(cur);
        tetrisGetCurrentShape(cur,shape);
        score=0; level=1;
        gameOver = !tetrisCanPlace(shape,cur.x,cur.y);
        lastDrop=lastStep=millis();
        SFX(SF_CLICK);
      }
      frameWait(t0); continue;
    }
    tetrisGetCurrentShape(cur,shape);
    if (nav(BI_LEFT) && tetrisCanPlace(shape,cur.x-1,cur.y)) { --cur.x; SFX(SF_MOVE); }
    if (nav(BI_RIGHT) && tetrisCanPlace(shape,cur.x+1,cur.y)) { ++cur.x; SFX(SF_MOVE); }
    if (evDown[BI_UP] || evDown[BI_OK]) {
      if (tetrisTryRotate(cur)) SFX(SF_ROT);
      tetrisGetCurrentShape(cur,shape);
    }
    if (held(BI_DOWN) && millis()-lastStep>=65) {
      if (tetrisCanPlace(shape,cur.x,cur.y+1)) { ++cur.y; lastStep=millis(); }
    }
    uint32_t dropDelay = 1040u - (uint32_t)min(level-1,15)*35u;
    if (dropDelay<500u) dropDelay=500u;
    if (millis()-lastDrop >= dropDelay) {
      lastDrop = millis();
      if (tetrisCanPlace(shape,cur.x,cur.y+1)) ++cur.y;
      else {
        tetrisMergePiece(shape,cur.x,cur.y);
        const int lines = tetrisClearLines();
        if (lines>0) {
          static const int points[5] = {0,100,300,500,800};
          score += points[constrain(lines,0,4)]*level;
          level = 1 + score/500;
          SFX(SF_SCORE);
        } else SFX(SF_HIT);
        tetrisSpawnPiece(cur);
        tetrisGetCurrentShape(cur,shape);
        if (!tetrisCanPlace(shape,cur.x,cur.y)) {
          if (score>best) { best=score; setBest(APP_TETRIS,score); }
          SFX(SF_DIE); gameOver=true;
        }
      }
    }
    oled.clearBuffer();
    oled.drawFrame(0,0,39,63);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(45,8,"TETRIS");
    oled.drawHLine(43,11,82);
    oled.drawStr(45,22,("SCORE "+String(score)).c_str());
    oled.drawStr(45,32,("LEVEL "+String(level)).c_str());
    oled.drawStr(45,42,("BEST  "+String(best)).c_str());
    oled.drawStr(45,53,"UP/OK ROT");
    oled.drawStr(45,62,"DN DROP");
    tetrisDrawBoard(); tetrisDrawPiece(cur);
    present(); frameWait(t0);
  }
}

// -------- SNAKE --------
void runSnake() {
  waitRelease();
  playMusicTheme(TECH_SNAKE,OLD_SNAKE,CHILL_SNAKE,DARK_SNAKE,TECH_SNAKE,120);
  const int W=16, H=8;
  int sx=4, sy=4; int dir=0;
  int dx[4]={1,0,-1,0}, dy[4]={0,1,0,-1};
  int snakeX[128], snakeY[128], len=4;
  for (int i=0;i<len;i++) { snakeX[i]=sx-i; snakeY[i]=sy; }
  int foodX=random(1,W-1), foodY=random(1,H-1);
  int score=0, best=getBest(APP_SNAKE);
  uint32_t last=millis();
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (evDown[BI_LEFT] && dir!=0) dir=2;
    if (evDown[BI_RIGHT] && dir!=2) dir=0;
    if (evDown[BI_UP] && dir!=1) dir=3;
    if (evDown[BI_DOWN] && dir!=3) dir=1;
    if (evDown[BI_OK]) { overlayBox("PAUSE","OK resume"); waitRelease(); }
    if (millis()-last>=160) {
      last=millis();
      int nx=snakeX[0]+dx[dir], ny=snakeY[0]+dy[dir];
      if (nx<0||nx>=W||ny<0||ny>=H) {
        SFX(SF_DIE); if (score>best) setBest(APP_SNAKE,score);
        overlayBox("GAME OVER",("Score "+String(score)).c_str());
        waitRelease(); return;
      }
      for (int i=0;i<len;i++) if (snakeX[i]==nx&&snakeY[i]==ny) {
        SFX(SF_DIE); if (score>best) setBest(APP_SNAKE,score);
        overlayBox("CRASH",("Score "+String(score)).c_str());
        waitRelease(); return;
      }
      for (int i=len;i>0;i--) { snakeX[i]=snakeX[i-1]; snakeY[i]=snakeY[i-1]; }
      snakeX[0]=nx; snakeY[0]=ny;
      if (nx==foodX && ny==foodY) {
        len++; score+=10; SFX(SF_EAT);
        bool ok; int tries=0;
        do {
          ok=true; foodX=random(0,W); foodY=random(0,H);
          for (int i=0;i<len;i++) if (snakeX[i]==foodX&&snakeY[i]==foodY) { ok=false; break; }
        } while (!ok && ++tries<200);
      }
    }
    oled.clearBuffer();
    oled.drawFrame(0,0,128,56);
    int cx=2, cy=2, cs=6;
    for (int i=0;i<len;i++) {
      int px=cx+snakeX[i]*cs, py=cy+snakeY[i]*cs;
      if (i==0) oled.drawRBox(px,py,cs-1,cs-1,2);
      else oled.drawBox(px,py,cs-1,cs-1);
    }
    oled.drawDisc(cx+foodX*cs+(cs/2), cy+foodY*cs+(cs/2), 2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)+" HI "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- PONG --------
void runPong() {
  waitRelease();
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,160);
  int paddleL=28, paddleR=28;
  const int PW=2, PH=10;
  float ballX=64, ballY=28, vX=2.0f, vY=1.2f;
  int scoreL=0, scoreR=0, best=getBest(APP_PONG);
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (held(BI_UP)) paddleL -= 3;
    if (held(BI_DOWN)) paddleL += 3;
    if (paddleL<2) paddleL=2; if (paddleL>46) paddleL=46;
    if (ballY < paddleR+PH/2) paddleR -= 2; else paddleR += 2;
    paddleR = constrain(paddleR,2,46);
    ballX += vX; ballY += vY;
    if (ballY<2 || ballY>56-2) { vY=-vY; SFX(SF_HIT); }
    if (ballX<=6 && ballY>=paddleL && ballY<=paddleL+PH) {
      vX=-vX; ballX=8; vX*=1.03; SFX(SF_HIT);
    }
    if (ballX>=122 && ballY>=paddleR && ballY<=paddleR+PH) {
      vX=-vX; ballX=120; vX*=1.03; SFX(SF_HIT);
    }
    if (ballX<0) { scoreR++; ballX=64; ballY=28; vX=2.0; vY=1.2; SFX(SF_DIE); }
    if (ballX>128) { scoreL++; ballX=64; ballY=28; vX=-2.0; vY=1.2; SFX(SF_SCORE); }
    if (scoreL>=5 || scoreR>=5) {
      int sc = (scoreL>scoreR)?scoreL:scoreR;
      if (sc>best) setBest(APP_PONG,sc);
      overlayBox("MATCH END",("Score "+String(scoreL)+":"+String(scoreR)).c_str());
      waitRelease(); return;
    }
    oled.clearBuffer();
    for (int y=0;y<56;y+=4) oled.drawPixel(64,y+2);
    oled.drawBox(2,paddleL,PW,PH);
    oled.drawBox(124,paddleR,PW,PH);
    oled.drawDisc((int)ballX,(int)ballY,1);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,(String(scoreL)+" : "+String(scoreR)).c_str());
    present(); frameWait(t0);
  }
}

// -------- BREAKOUT (с новыми фишками: 2-HP кирпичи + расширение платформы) --------
void runBreak() {
  waitRelease();
  playMusicTheme(TECH_TETRIS,OLD_TETRIS,CHILL_TETRIS,DARK_TETRIS,TECH_TETRIS,140);
  int paddleX=50;
  float bx=64, by=48, vx=2.2f, vy=-2.0f;
  int PW=24; const int PH=4;
  const int BR=4, BC=8;
  bool bricks[BR][BC];
  uint8_t brickHP[BR][BC];
  for (int r=0;r<BR;r++) for (int c=0;c<BC;c++) {
    bricks[r][c]=true;
    brickHP[r][c] = (r<2) ? 2 : 1; // верхние 2 ряда двухпрочные
  }
  int score=0, best=getBest(APP_BRICK);
  bool paddleWide=false;
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (held(BI_LEFT)) paddleX -= 3;
    if (held(BI_RIGHT)) paddleX += 3;
    paddleX = constrain(paddleX, 2, 128-PW-2);
    bx += vx; by += vy;
    if (bx<2 || bx>126) { vx=-vx; SFX(SF_HIT); }
    if (by<2) { vy=-vy; SFX(SF_HIT); }
    if (by>=48 && by<=52 && bx>=paddleX && bx<=paddleX+PW) {
      vy = -fabs(vy); vx += (bx-(paddleX+PW/2))*0.04f; SFX(SF_HIT);
    }
    int br = (int)((by-4)/6);
    int bc = (int)((bx-2)/15);
    if (br>=0 && br<BR && bc>=0 && bc<BC && bricks[br][bc]) {
      brickHP[br][bc]--;
      if (brickHP[br][bc]<=0) { bricks[br][bc]=false; score+=10; }
      else { score+=5; }
      vy = -vy; SFX(SF_SCORE);
      if (score>=100 && !paddleWide) { paddleWide=true; PW=40; }
    }
    if (by>64) {
      if (score>best) setBest(APP_BRICK,score);
      overlayBox("GAME OVER",("Score "+String(score)).c_str());
      waitRelease(); return;
    }
    bool any=false;
    for (int r=0;r<BR;r++) for (int c=0;c<BC;c++) if (bricks[r][c]) any=true;
    if (!any) {
      setBest(APP_BRICK,score);
      overlayBox("YOU WIN",("Score "+String(score)).c_str());
      waitRelease(); return;
    }
    oled.clearBuffer();
    for (int r=0;r<BR;r++) for (int c=0;c<BC;c++) {
      if (!bricks[r][c]) continue;
      int px = 2+c*15, py = 4+r*6;
      if (brickHP[r][c]>=2) {
        // двухпрочный: пунктирный
        for (int xx=0;xx<13;xx+=2) oled.drawPixel(px+xx,py);
        for (int xx=0;xx<13;xx+=2) oled.drawPixel(px+xx,py+3);
        oled.drawPixel(px,py+1); oled.drawPixel(px+12,py+1);
        oled.drawPixel(px,py+2); oled.drawPixel(px+12,py+2);
      } else {
        oled.drawBox(px,py,13,4);
      }
    }
    oled.drawRBox(paddleX,54,PW,PH,2);
    oled.drawDisc((int)bx,(int)by,2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)+" HI "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- INVADERS --------
void runInvade() {
  waitRelease();
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,140);
  const int N=24;
  float ex[N]; int ey[N]; bool alive[N];
  for (int i=0;i<N;i++) { ex[i]=10+(i%8)*13; ey[i]=6+(i/8)*8; alive[i]=true; }
  float shipX=64; float shotX=-1, shotY=0;
  float dir=0.6f; int score=0, best=getBest(APP_INVADE);
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (held(BI_LEFT)) shipX -= 3;
    if (held(BI_RIGHT)) shipX += 3;
    shipX = constrain(shipX,6,120);
    if (evDown[BI_OK] && shotX<0) { shotX=shipX; shotY=52; SFX(SF_SHOOT); }
    bool change=false;
    for (int i=0;i<N;i++) {
      ex[i] += dir;
      if (ex[i]<6 || ex[i]>120) change=true;
    }
    if (change) { dir=-dir; for (int i=0;i<N;i++) ey[i]+=6; }
    if (shotX>=0) { shotY -= 4; if (shotY<0) shotX=-1; }
    for (int i=0;i<N;i++) if (alive[i] && shotX>=0) {
      if (abs(shotX-ex[i])<6 && abs(shotY-ey[i])<4) { alive[i]=false; shotX=-1; score+=10; SFX(SF_HIT); }
    }
    for (int i=0;i<N;i++) if (alive[i] && ey[i]>46) {
      if (score>best) setBest(APP_INVADE,score);
      overlayBox("INVADERS WIN","GAME OVER"); waitRelease(); return;
    }
    bool any=false;
    for (int i=0;i<N;i++) if (alive[i]) any=true;
    if (!any) { setBest(APP_INVADE,score); overlayBox("YOU WIN",("Score "+String(score)).c_str()); waitRelease(); return; }
    oled.clearBuffer();
    for (int i=0;i<N;i++) if (alive[i]) oled.drawBox((int)ex[i],ey[i],8,4);
    oled.drawRBox((int)shipX-4,54,8,4,2);
    if (shotX>=0) oled.drawBox((int)shotX,(int)shotY,2,4);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)+" HI "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- ASTEROIDS (пофикшены: 3 жизни, 5 астероидов, медленнее) --------
void runAster() {
  waitRelease();
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,160);
  float sx=64, sy=32, sv=0, sa=0;
  const int MAXA=5;
  Rock ast[MAXA];
  for (int i=0;i<MAXA;i++) {
    // Спавним подальше от центра, где корабль
    int ox, oy;
    do {
      ox = random(0,128); oy = random(0,64);
    } while ((ox-64)*(ox-64) + (oy-32)*(oy-32) < 900);
    ast[i].x = ox; ast[i].y = oy;
    ast[i].vx = random(-10,11)/10.0;
    ast[i].vy = random(-10,11)/10.0;
    ast[i].sz = random(2,4); ast[i].on = true;
  }
  float shotX=-1, shotY=0;
  int score=0, best=getBest(APP_ASTER), lives=3;
  uint32_t invulnUntil = 0;
  while (true) {
    uint32_t t0=millis(), now=millis();
    pollButtons();
    if (quitPressed()) break;
    if (held(BI_LEFT)) sa -= 0.08f;
    if (held(BI_RIGHT)) sa += 0.08f;
    if (evDown[BI_OK] && shotX<0) { shotX=sx; shotY=sy; SFX(SF_SHOOT); }
    if (held(BI_UP)) sv += 0.08f;
    sv = max(-3.0f, min(3.0f, sv));
    sx += sv * cos(sa);
    sy += sv * sin(sa);
    if (sx<0) sx+=128; if (sx>=128) sx-=128;
    if (sy<0) sy+=64; if (sy>=64) sy-=64;
    for (int i=0;i<MAXA;i++) if (ast[i].on) {
      ast[i].x += ast[i].vx; ast[i].y += ast[i].vy;
      if (ast[i].x<0) ast[i].x+=128; if (ast[i].x>=128) ast[i].x-=128;
      if (ast[i].y<0) ast[i].y+=64; if (ast[i].y>=64) ast[i].y-=64;
      if (now < invulnUntil) continue;
      float dx=ast[i].x-sx, dy=ast[i].y-sy;
      if (dx*dx+dy*dy < (ast[i].sz+2)*(ast[i].sz+2)) {
        SFX(SF_DIE);
        lives--;
        if (lives<=0) {
          if (score>best) setBest(APP_ASTER,score);
          overlayBox("GAME OVER",("Score "+String(score)).c_str());
          waitRelease(); return;
        }
        // Респавн в центре + неуязвимость
        sx=64; sy=32; sv=0;
        invulnUntil = now + 2000;
        break;
      }
    }
    if (shotX>=0) {
      shotX += 4*cos(sa); shotY += 4*sin(sa);
      if (shotX<0||shotX>=128||shotY<0||shotY>=64) shotX=-1;
      for (int i=0;i<MAXA;i++) if (ast[i].on && shotX>=0) {
        float dx=ast[i].x-shotX, dy=ast[i].y-shotY;
        if (dx*dx+dy*dy < (ast[i].sz+1)*(ast[i].sz+1)) {
          ast[i].on=false; shotX=-1; score+=10; SFX(SF_BOOM);
        }
      }
    }
    bool any=false;
    for (int i=0;i<MAXA;i++) if (ast[i].on) any=true;
    if (!any) {
      setBest(APP_ASTER,score);
      overlayBox("CLEAR",("Score "+String(score)).c_str());
      waitRelease(); return;
    }
    oled.clearBuffer();
    int tx=(int)sx, ty=(int)sy;
    int hx = tx + (int)(6*cos(sa)), hy = ty + (int)(6*sin(sa));
    oled.drawLine(tx-(int)(3*cos(sa+1.57)), ty-(int)(3*sin(sa+1.57)), hx, hy);
    oled.drawLine(tx+(int)(3*cos(sa+1.57)), ty+(int)(3*sin(sa+1.57)), hx, hy);
    oled.drawLine(tx-(int)(3*cos(sa+1.57)), ty-(int)(3*sin(sa+1.57)),
                  tx+(int)(3*cos(sa+1.57)), ty+(int)(3*sin(sa+1.57)));
    for (int i=0;i<MAXA;i++) if (ast[i].on) oled.drawCircle((int)ast[i].x,(int)ast[i].y,ast[i].sz);
    if (shotX>=0) oled.drawBox((int)shotX,(int)shotY,2,2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)+" L"+String(lives)+" HI "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- MINESWEEPER --------
void runMines() {
  waitRelease();
  const int W=8, H=6;
  bool mine[H][W]; int state[H][W];
  for (int r=0;r<H;r++) for (int c=0;c<W;c++) { mine[r][c]=false; state[r][c]=-1; }
  const int mcount=8;
  for (int i=0;i<mcount;i++) {
    int r,c;
    do { r=random(H); c=random(W); } while (mine[r][c]);
    mine[r][c]=true;
  }
  auto countAround = [&](int rr, int cc) {
    int n=0;
    for (int y=rr-1;y<=rr+1;y++) for (int x=cc-1;x<=cc+1;x++)
      if (y>=0&&y<H&&x>=0&&x<W && mine[y][x]) n++;
    return n;
  };
  int cur=0, score=0, best=getBest(APP_MINES);
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    int r=cur/W, c=cur%W;
    if (nav(BI_LEFT) && c>0) cur--;
    if (nav(BI_RIGHT) && c<W-1) cur++;
    if (nav(BI_UP) && r>0) cur -= W;
    if (nav(BI_DOWN) && r<H-1) cur += W;
    if (evDown[BI_OK]) {
      if (state[r][c]==-1) {
        if (mine[r][c]) {
          SFX(SF_DIE); overlayBox("BOOM","You hit a mine"); waitRelease(); return;
        }
        int qx[W*H], qy[W*H], qs=0, qe=0;
        qx[qe]=c; qy[qe++]=r;
        while (qs<qe) {
          int cx=qx[qs], ry=qy[qs++];
          if (state[ry][cx]!=-1) continue;
          int cnt = countAround(ry,cx);
          state[ry][cx]=cnt; score++;
          if (cnt==0)
            for (int yy=ry-1;yy<=ry+1;yy++)
              for (int xx=cx-1;xx<=cx+1;xx++)
                if (yy>=0&&yy<H&&xx>=0&&xx<W && state[yy][xx]==-1) {
                  qx[qe]=xx; qy[qe++]=yy;
                }
        }
        SFX(SF_OPEN);
      }
    }
    if (evTap[BI_BACK]) {
      if (state[r][c]==-1) state[r][c]=9;
      else if (state[r][c]==9) state[r][c]=-1;
    }
    int covered=0;
    for (int y=0;y<H;y++) for (int x=0;x<W;x++) if (state[y][x]==-1||state[y][x]==9) covered++;
    if (covered==mcount) { if (score>best) setBest(APP_MINES,score); overlayBox("CLEAR","You win"); waitRelease(); return; }
    oled.clearBuffer();
    for (int y=0;y<H;y++) for (int x=0;x<W;x++) {
      int px=6+x*14, py=6+y*8;
      if (state[y][x]==-1) {
        oled.drawFrame(px,py,12,6);
        if (cur==y*W+x) oled.drawRBox(px,py,12,6,2);
      } else if (state[y][x]==9) {
        oled.drawRBox(px,py,12,6,2);
        oled.setDrawColor(0); oled.drawStr(px+4,py+4,"F"); oled.setDrawColor(1);
      } else {
        oled.drawBox(px,py,12,6);
        if (state[y][x]>0) { oled.setDrawColor(0); oled.drawStr(px+4,py+5,String(state[y][x]).c_str()); oled.setDrawColor(1); }
      }
    }
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("FOUND "+String(score)+" HI "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- 2048 --------
void run2048() {
  waitRelease();
  int grid[4][4] = {{0}};
  int nextTile = random(10)<9 ? 2 : 4;
  auto spawn = [&]() {
    int tries=0;
    while (tries<50) {
      int r=random(4), c=random(4);
      if (!grid[r][c]) { grid[r][c]=nextTile; nextTile=random(10)<9?2:4; return; }
      tries++;
    }
  };
  spawn(); spawn();
  int score=0, best=getBest(APP_2048);
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    int dir=-1;
    if (evDown[BI_LEFT]) dir=0;
    if (evDown[BI_UP]) dir=1;
    if (evDown[BI_RIGHT]) dir=2;
    if (evDown[BI_DOWN]) dir=3;
    if (dir!=-1) {
      bool moved=false;
      if (dir==0) {
        for (int r=0;r<4;r++) {
          int line[4]={0}, idx=0;
          for (int c=0;c<4;c++) if (grid[r][c]) line[idx++]=grid[r][c];
          for (int i=0;i<idx-1;i++)
            if (line[i]==line[i+1]) { line[i]*=2; score+=line[i]; line[i+1]=0;
              for (int j=i+1;j<idx-1;j++) line[j]=line[j+1]; idx--; i--; }
          for (int c=0;c<4;c++) { int v=c<idx?line[c]:0; if (grid[r][c]!=v) { grid[r][c]=v; moved=true; } }
        }
      } else if (dir==2) {
        for (int r=0;r<4;r++) {
          int line[4]={0}, idx=0;
          for (int c=3;c>=0;c--) if (grid[r][c]) line[idx++]=grid[r][c];
          for (int i=0;i<idx-1;i++)
            if (line[i]==line[i+1]) { line[i]*=2; score+=line[i]; line[i+1]=0;
              for (int j=i+1;j<idx-1;j++) line[j]=line[j+1]; idx--; i--; }
          for (int c=3,j=0;c>=0;c--,j++) { int v=j<idx?line[j]:0; if (grid[r][c]!=v) { grid[r][c]=v; moved=true; } }
        }
      } else if (dir==1) {
        for (int c=0;c<4;c++) {
          int line[4]={0}, idx=0;
          for (int r=0;r<4;r++) if (grid[r][c]) line[idx++]=grid[r][c];
          for (int i=0;i<idx-1;i++)
            if (line[i]==line[i+1]) { line[i]*=2; score+=line[i]; line[i+1]=0;
              for (int j=i+1;j<idx-1;j++) line[j]=line[j+1]; idx--; i--; }
          for (int r=0;r<4;r++) { int v=r<idx?line[r]:0; if (grid[r][c]!=v) { grid[r][c]=v; moved=true; } }
        }
      } else if (dir==3) {
        for (int c=0;c<4;c++) {
          int line[4]={0}, idx=0;
          for (int r=3;r>=0;r--) if (grid[r][c]) line[idx++]=grid[r][c];
          for (int i=0;i<idx-1;i++)
            if (line[i]==line[i+1]) { line[i]*=2; score+=line[i]; line[i+1]=0;
              for (int j=i+1;j<idx-1;j++) line[j]=line[j+1]; idx--; i--; }
          for (int r=3,j=0;r>=0;r--,j++) { int v=j<idx?line[j]:0; if (grid[r][c]!=v) { grid[r][c]=v; moved=true; } }
        }
      }
      if (moved) { spawn(); SFX(SF_SCORE); }
    }
    bool full=true;
    for (int r=0;r<4;r++) for (int c=0;c<4;c++) if (!grid[r][c]) full=false;
    if (full) {
      bool can=false;
      for (int r=0;r<4;r++) for (int c=0;c<4;c++) {
        if (c<3 && grid[r][c]==grid[r][c+1]) can=true;
        if (r<3 && grid[r][c]==grid[r+1][c]) can=true;
      }
      if (!can) {
        if (score>best) setBest(APP_2048,score);
        overlayBox("GAME OVER",("S "+String(score)).c_str());
        waitRelease(); return;
      }
    }
    oled.clearBuffer();
    oled.drawFrame(6,4,116,52);
    for (int r=0;r<4;r++) for (int c=0;c<4;c++) {
      int px=8+c*28, py=6+r*12;
      if (grid[r][c]) {
        oled.drawRBox(px,py,24,10,3);
        oled.setFont(u8g2_font_6x13_tr);
        String s = String(grid[r][c]);
        oled.drawStr(px+(24-oled.getStrWidth(s.c_str()))/2, py+8, s.c_str());
        oled.setFont(u8g2_font_5x8_t_cyrillic);
      } else oled.drawFrame(px,py,24,10);
    }
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)+" HI "+String(best)).c_str());
    oled.drawFrame(104,0,22,8); oled.setFont(u8g2_font_4x6_tr);
    oled.drawStr(106,6,String(nextTile).c_str());
    present(); frameWait(t0);
  }
}

// -------- SIMON --------
void runSimon() {
  waitRelease();
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,150);
  const int SEQ_MAX=12;
  int seq[SEQ_MAX]; int len=1; int idx=0;
  int score=0, best=getBest(APP_SIMON);
  for (int i=0;i<SEQ_MAX;i++) seq[i]=random(4);
  bool show=true;
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (show) {
      for (int i=0;i<len;i++) {
        oled.clearBuffer();
        int b=seq[i];
        if (b==0) oled.drawRBox(10,10,48,22,3);
        if (b==1) oled.drawRBox(70,10,48,22,3);
        if (b==2) oled.drawRBox(10,36,48,22,3);
        if (b==3) oled.drawRBox(70,36,48,22,3);
        SFX(SPADS[b][0].f?SPADS[b]:SP0);
        present(); delay(450);
        oled.clearBuffer(); present(); delay(150);
      }
      show=false; idx=0;
    } else {
      int pressed=-1;
      if (evDown[BI_LEFT]) pressed=2;
      if (evDown[BI_RIGHT]) pressed=3;
      if (evDown[BI_UP]) pressed=0;
      if (evDown[BI_OK]) pressed=1;
      if (pressed!=-1) {
        SFX(SPADS[pressed][0].f?SPADS[pressed]:SP0);
        if (pressed != seq[idx]) {
          SFX(SF_DIE); overlayBox("WRONG","Try again");
          if (score>best) setBest(APP_SIMON,score);
          waitRelease(); return;
        }
        idx++;
        if (idx>=len) {
          len++; score++;
          if (len>SEQ_MAX) { setBest(APP_SIMON,score); overlayBox("WIN","Simon master"); waitRelease(); return; }
          show=true; delay(300);
        }
      }
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,8,"SIMON");
    oled.drawStr(2,62,("S "+String(score)+" HI "+String(best)).c_str());
    oled.drawRFrame(10,10,48,22,2); oled.drawRFrame(70,10,48,22,2);
    oled.drawRFrame(10,36,48,22,2); oled.drawRFrame(70,36,48,22,2);
    present(); frameWait(t0);
  }
}

// -------- FROGGER (исправлен полностью) --------
void runFrogger() {
  waitRelease();
  playMusicTheme(CHILL_FLAPPY,OLD_FLAPPY,CHILL_FLAPPY,DARK_FLAPPY,OLD_FLAPPY,160);

  const int LANES = 5;
  const int LANE_H = 9;
  const int ROAD_TOP = 14;

  int frogX = 64, frogY = 58;
  int score = 0, lives = 3;
  int best = getBest(APP_FROGGER);

  struct Car { float x; float v; int w; };
  Car cars[LANES][3];

  auto resetCars = [&]() {
    for (int l=0;l<LANES;l++) {
      float dir = (l%2==0) ? 1.0f : -1.0f;
      float speed = (1.0f + l*0.15f) * (cfg.difficulty==0?0.7f:cfg.difficulty==2?1.3f:1.0f);
      for (int c=0;c<3;c++) {
        cars[l][c].x = random(0,128);
        cars[l][c].v = dir * speed;
        cars[l][c].w = random(8,14);
      }
    }
  };
  auto resetFrog = [&]() { frogX = 64; frogY = 58; };

  resetCars();
  bool dead = false;
  uint32_t deadAt = 0;

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;

    if (!dead) {
      int newX = frogX, newY = frogY;
      if (nav(BI_LEFT)) newX -= 8;
      if (nav(BI_RIGHT)) newX += 8;
      if (nav(BI_UP)) newY -= 9;
      if (nav(BI_DOWN)) newY += 9;
      frogX = constrain(newX, 4, 124);
      frogY = constrain(newY, 6, 58);

      // Win — достиг верха
      if (frogY <= 8) {
        score += 10;
        if (score>best) { best=score; setBest(APP_FROGGER,score); }
        SFX(SF_SCORE);
        resetFrog();
        for (int l=0;l<LANES;l++) for (int c=0;c<3;c++) cars[l][c].v *= 1.05f;
      }

      // Update cars and check collision
      for (int l=0;l<LANES;l++) {
        int laneY = ROAD_TOP + l*LANE_H;
        for (int c=0;c<3;c++) {
          cars[l][c].x += cars[l][c].v;
          if (cars[l][c].v > 0 && cars[l][c].x > 130)
            cars[l][c].x = -cars[l][c].w - random(0,30);
          if (cars[l][c].v < 0 && cars[l][c].x < -cars[l][c].w - 10)
            cars[l][c].x = 130 + random(0,30);

          int cx1 = (int)cars[l][c].x;
          int cx2 = cx1 + cars[l][c].w;
          bool laneOverlap = (abs(frogY - (laneY+3)) < 5);
          bool xOverlap = (frogX+3 > cx1) && (frogX-3 < cx2);
          if (laneOverlap && xOverlap) {
            dead = true;
            deadAt = millis();
            lives--;
            SFX(SF_DIE);
          }
        }
      }
    } else if (millis() - deadAt > 800) {
      if (lives <= 0) {
        if (evDown[BI_OK]) {
          resetCars(); resetFrog();
          score = 0; lives = 3; dead = false;
        }
      } else {
        resetFrog(); dead = false;
      }
    }

    oled.clearBuffer();
    // Рисуем дорогу
    for (int l=0;l<LANES;l++) {
      int y = ROAD_TOP + l*LANE_H;
      oled.drawHLine(0, y+LANE_H-1, 128);
    }
    // Рисуем машины
    for (int l=0;l<LANES;l++) {
      int y = ROAD_TOP + l*LANE_H;
      for (int c=0;c<3;c++) {
        int cx = (int)cars[l][c].x;
        oled.drawRBox(cx, y, cars[l][c].w, LANE_H-2, 1);
      }
    }
    // Лягушка
    oled.drawRBox(frogX-3, frogY-3, 7, 7, 2);
    oled.setDrawColor(0);
    oled.drawPixel(frogX-2, frogY-1);
    oled.drawPixel(frogX+2, frogY-1);
    oled.setDrawColor(1);
    // Целевая линия сверху
    oled.drawHLine(0, 8, 128);

    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0, 63, ("S "+String(score)+" L"+String(lives)+" HI "+String(best)).c_str());

    if (dead) {
      overlayBox(lives<=0?"GAME OVER":"HIT!",
                 lives<=0?"OK: retry":"Continue...");
    }
    present(); frameWait(t0);
  }
}

// -------- TTT (улучшенный ИИ: минимакс + случайность) --------
int tttWinner(int* board) {
  static const int lines[8][3] = {{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  for (int i=0;i<8;i++)
    if (board[lines[i][0]] && board[lines[i][0]]==board[lines[i][1]] && board[lines[i][1]]==board[lines[i][2]])
      return board[lines[i][0]];
  return 0;
}
int tttMinimax(int* board, bool isMax) {
  int w = tttWinner(board);
  if (w==2) return 10;
  if (w==1) return -10;
  bool full = true;
  for (int i=0;i<9;i++) if (board[i]==0) { full=false; break; }
  if (full) return 0;
  int best = isMax ? -100 : 100;
  for (int i=0;i<9;i++) {
    if (board[i]) continue;
    board[i] = isMax ? 2 : 1;
    int score = tttMinimax(board, !isMax);
    board[i] = 0;
    if (isMax) best = max(best, score);
    else best = min(best, score);
  }
  return best;
}
int tttAIPick(int* board) {
  // 15% случайных ходов для непредсказуемости
  if (random(100) < 15) {
    int opts[9], n=0;
    for (int i=0;i<9;i++) if (!board[i]) opts[n++]=i;
    return n ? opts[random(n)] : -1;
  }
  int bestScore = -100, bestMove = -1;
  for (int i=0;i<9;i++) {
    if (board[i]) continue;
    board[i] = 2;
    int score = tttMinimax(board, false);
    board[i] = 0;
    if (score > bestScore) { bestScore = score; bestMove = i; }
  }
  return bestMove;
}
void runTTT() {
  waitRelease();
  int board[9];
  for (int i=0;i<9;i++) board[i]=0;
  int cur = 4, player = 1;
  int best = getBest(APP_TTT);
  int gamesWon = prefs.getInt("ttW",0);
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    int r = cur/3, c = cur%3;
    if (nav(BI_LEFT) && c>0) cur--;
    if (nav(BI_RIGHT) && c<2) cur++;
    if (nav(BI_UP) && r>0) cur -= 3;
    if (nav(BI_DOWN) && r<2) cur += 3;
    if (evDown[BI_OK] && board[cur]==0 && player==1) {
      board[cur]=1;
      SFX(SF_CLICK);
      int w = tttWinner(board);
      if (w==1) {
        gamesWon++; prefs.putInt("ttW", gamesWon);
        if (gamesWon > best) { best=gamesWon; setBest(APP_TTT,best); }
        SFX(SF_WIN);
        overlayBox("YOU WIN!","OK to restart");
        waitRelease();
        for (int i=0;i<9;i++) board[i]=0;
        player=1; continue;
      }
      bool full = true;
      for (int i=0;i<9;i++) if (board[i]==0) full=false;
      if (full) {
        overlayBox("DRAW","OK to restart");
        waitRelease();
        for (int i=0;i<9;i++) board[i]=0;
        player=1; continue;
      }
      player = 2;
    }
    // AI ход
    if (player==2) {
      int pick = tttAIPick(board);
      if (pick>=0) {
        board[pick]=2;
        SFX(SF_MOVE);
        int w = tttWinner(board);
        if (w==2) {
          SFX(SF_DIE);
          overlayBox("AI WINS","OK to restart");
          waitRelease();
          for (int i=0;i<9;i++) board[i]=0;
          player=1; continue;
        }
        bool full = true;
        for (int i=0;i<9;i++) if (board[i]==0) full=false;
        if (full) {
          overlayBox("DRAW","OK to restart");
          waitRelease();
          for (int i=0;i<9;i++) board[i]=0;
          player=1; continue;
        }
        player = 1;
      }
    }
    oled.clearBuffer();
    oled.drawFrame(10,6,108,48);
    oled.drawVLine(10+36,6,48);
    oled.drawVLine(10+72,6,48);
    oled.drawHLine(10,6+16,108);
    oled.drawHLine(10,6+32,108);
    for (int i=0;i<9;i++) {
      int rr=i/3, cc=i%3;
      int x=12+cc*36, y=10+rr*16;
      if (board[i]==1) {
        oled.drawLine(x+4,y+2,x+24,y+12);
        oled.drawLine(x+24,y+2,x+4,y+12);
      } else if (board[i]==2) {
        oled.drawCircle(x+14,y+7,6);
      }
      if (i==cur && player==1) {
        oled.drawFrame(x-1,y-1,32,14);
      }
    }
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("WINS "+String(gamesWon)+" HI "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- SNAKE MAZE --------
// A fair grid maze: the snake starts in a guaranteed open corridor, food is
// placed only on empty cells, and a missed turn cannot cause an immediate wall hit.
void runSnakeMaze() {
  waitRelease();
  playMusicTheme(TECH_SNAKE,OLD_SNAKE,CHILL_SNAKE,DARK_SNAKE,TECH_SNAKE,110);
  const int GW=20, GH=8, CS=6, MAXLEN=GW*GH;
  const int DX[4]={1,0,-1,0};
  const int DY[4]={0,1,0,-1};
  uint8_t maze[GH][GW];
  int snX[MAXLEN], snY[MAXLEN];
  int len=3, dir=0, pend=0, score=0, best=getBest(APP_SNAKE_MAZE);
  int foodX=15, foodY=3;
  bool dead=false, won=false;
  uint32_t lastStep=0, deadAt=0;

  auto buildMaze = [&]() {
    for (int r=0;r<GH;r++) for (int c=0;c<GW;c++) {
      bool wall = (r==0 || r==GH-1 || c==0 || c==GW-1);
      // Repeating vertical dividers, each with an intentional opening.
      if (c==4 && r!=2 && r!=5) wall=true;
      if (c==8 && r!=1 && r!=6) wall=true;
      if (c==12 && r!=3 && r!=6) wall=true;
      if (c==16 && r!=2 && r!=5) wall=true;
      maze[r][c]=wall?1:0;
    }
    // A guaranteed start corridor for three snake segments and first move.
    for (int c=7;c<=13;c++) maze[4][c]=0;
    maze[3][8]=maze[2][8]=maze[1][8]=0;
    maze[5][12]=maze[6][12]=0;
    maze[3][16]=maze[2][16]=0;
  };
  auto placeFood = [&]() {
    for (int tries=0;tries<300;tries++) {
      int x=random(1,GW-1), y=random(1,GH-1);
      if (maze[y][x]) continue;
      bool occupied=false;
      for (int i=0;i<len;i++) if (snX[i]==x && snY[i]==y) { occupied=true; break; }
      if (!occupied) { foodX=x; foodY=y; return; }
    }
    foodX=15; foodY=3;
  };
  auto resetGame = [&]() {
    buildMaze(); len=3; dir=0; pend=0; score=0; dead=false; won=false;
    snX[0]=10; snY[0]=4; snX[1]=9; snY[1]=4; snX[2]=8; snY[2]=4;
    placeFood(); lastStep=millis();
  };
  auto finishGame = [&]() {
    dead=true; deadAt=millis();
    if (score>best) best=score;
    setBest(APP_SNAKE_MAZE,score); SFX(SF_DIE);
  };
  resetGame();
  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;
    if (!dead) {
      if (evDown[BI_RIGHT] && dir!=2) pend=0;
      if (evDown[BI_DOWN]  && dir!=3) pend=1;
      if (evDown[BI_LEFT]  && dir!=0) pend=2;
      if (evDown[BI_UP]    && dir!=1) pend=3;
      uint32_t stepMs = difficultyFor(APP_SNAKE_MAZE)==0 ? 190 : (difficultyFor(APP_SNAKE_MAZE)==2 ? 115 : 150);
      if ((uint32_t)(millis()-lastStep)>=stepMs) {
        lastStep=millis(); dir=pend;
        int hx=snX[0]+DX[dir], hy=snY[0]+DY[dir];
        bool eating=(hx==foodX && hy==foodY);
        bool wall=hx<0 || hx>=GW || hy<0 || hy>=GH || maze[hy][hx];
        bool self=false;
        // Moving into the old tail is legal when not growing this step.
        for (int i=0;i<len-(eating?0:1);i++) if (snX[i]==hx && snY[i]==hy) { self=true; break; }
        if (wall || self) finishGame();
        else {
          if (eating && len<MAXLEN) len++;
          for (int i=len-1;i>0;i--) { snX[i]=snX[i-1]; snY[i]=snY[i-1]; }
          snX[0]=hx; snY[0]=hy;
          if (eating) { score+=10; SFX(SF_EAT); placeFood(); }
        }
      }
    } else if (evDown[BI_OK]) resetGame();

    oled.clearBuffer();
    oled.drawFrame(0,0,120,48);
    for (int r=0;r<GH;r++) for (int c=0;c<GW;c++) if (maze[r][c]) {
      oled.drawBox(c*CS+1,r*CS+1,4,4);
      oled.drawPixel(c*CS+2,r*CS+2);
    }
    oled.drawFrame(foodX*CS+1,foodY*CS+1,4,4);
    oled.drawPixel(foodX*CS+2,foodY*CS+2);
    for (int i=len-1;i>0;i--) oled.drawRBox(snX[i]*CS+1,snY[i]*CS+1,4,4,1);
    oled.drawDisc(snX[0]*CS+3,snY[0]*CS+3,2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("FOOD "+String(score/10)+" HI "+String(best/10)).c_str());
    if (dead) overlayBox("CRASHED","OK: retry / BACK: exit");
    present(); frameWait(t0);
  }
}

// -------- MEMORY --------
void runMemory() {
  waitRelease();
  const int CX=4, CY=4, TOTAL=CX*CY;
  int mem[TOTAL];
  for (int i=0;i<TOTAL;i++) mem[i]=i/2;
  for (int i=TOTAL-1;i>0;i--) { int j=random(i+1), t=mem[i]; mem[i]=mem[j]; mem[j]=t; }
  uint8_t open[TOTAL]={0};
  int cur=0, pairs=0, best=getBest(APP_MEMORY), first=-1, second=-1;
  uint32_t hideAt=0;
  bool waitingToHide=false;
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (!waitingToHide) {
      int r=cur/CX, c=cur%CX;
      if (nav(BI_LEFT)&&c>0) cur--;
      if (nav(BI_RIGHT)&&c<CX-1) cur++;
      if (nav(BI_UP)&&r>0) cur-=CX;
      if (nav(BI_DOWN)&&r<CY-1) cur+=CX;
      if (evDown[BI_OK] && !open[cur]) {
        if (first<0) { first=cur; SFX(SF_OPEN); }
        else if (cur!=first) {
          second = cur;
          if (mem[first]==mem[second]) {
            open[first]=open[second]=1;
            pairs++;
            first=second=-1;
            SFX(SF_SCORE);
            if (pairs>best) { best=pairs; setBest(APP_MEMORY,best); }
            if (pairs>=TOTAL/2) {
              overlayBox("YOU WIN!","All pairs found");
              present(); delay(1200); break;
            }
          } else {
            waitingToHide=true;
            hideAt=millis()+700;
            SFX(SF_BOOM);
          }
        }
      }
    } else if ((int32_t)(millis()-hideAt)>=0) {
      open[first]=open[second]=0;
      first=second=-1;
      waitingToHide=false;
    }
    oled.clearBuffer();
    oled.drawFrame(10,5,108,52);
    for (int i=0;i<TOTAL;i++) {
      int rr=i/CX, cc=i%CX;
      int x=14+cc*27, y=10+rr*13;
      if (i==cur) { oled.drawRBox(x,y,24,10,3); oled.setDrawColor(0); }
      else oled.drawFrame(x,y,24,10);
      if (open[i]||i==first||i==second) {
        oled.setFont(u8g2_font_6x13_tr);
        String label = String(mem[i]+1);
        oled.drawStr(x+9,y+8,label.c_str());
        oled.setFont(u8g2_font_5x8_t_cyrillic);
      }
      oled.setDrawColor(1);
    }
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("PAIRS "+String(pairs)+"/8 BEST "+String(best)).c_str());
    present(); frameWait(t0);
  }
}

// -------- SKATER --------
void runSkater() {
  waitRelease();
  playMusicTheme(TECH_SNAKE,OLD_SNAKE,CHILL_SNAKE,DARK_SNAKE,TECH_SNAKE,100);
  float px=32, py=40, vy=0;
  int score=0, best=getBest(APP_SKATE);
  float obX[5];
  bool obOn[5]={false,false,false,false,false};
  for (int i=0;i<5;i++) obX[i]=150+i*40;
  bool dead=false, started=false;
  while (true) {
    uint32_t t0=millis();
    pollButtons();
    if (quitPressed()) break;
    if (!dead) {
      if (!started && (evDown[BI_UP] || evDown[BI_OK])) { started=true; SFX(SF_JUMP); }
      if (started) {
        // Jump again whenever grounded; previously the game had no jump after starting.
        if ((evDown[BI_UP] || evDown[BI_OK]) && py >= 39.5f) { vy=-4.4f; SFX(SF_JUMP); }
        float spd = 2.0f + score*0.025f;
        if (held(BI_LEFT)) px -= 2;
        if (held(BI_RIGHT)) px += 2;
        if (px<0) px=0; if (px>110) px=110;
        vy += 0.35f;
        py += vy;
        if (py>=40) { py=40; vy=0; }
        for (int i=0;i<5;i++) {
          obX[i] -= spd;
          if (obX[i] < -20) { obX[i]=130; obOn[i]=true; }
          if (obOn[i] && px+8>obX[i] && px+2<obX[i]+8 && py+4>=36) {
            dead=true;
            if (score>best) best=score;
            setBest(APP_SKATE,score);
            SFX(SF_DIE);
          }
        }
        score++;
      }
    } else if (evDown[BI_OK]) {
      px=32; py=40; vy=0; score=0; dead=false; started=false;
    }
    oled.clearBuffer();
    oled.drawHLine(0,44,128);
    for (int i=0;i<5;i++) {
      if (obX[i]<130 && obX[i]>-20) oled.drawRBox((int)obX[i],36,10,8,2);
    }
    oled.drawRBox((int)px,(int)py-4,10,8,2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("SCORE "+String(score/10)+" HI "+String(best)).c_str());
    if (!started) oled.drawStr(40,24,"JUMP!");
    if (dead) overlayBox("CRASHED!","OK: retry");
    present(); frameWait(t0);
  }
}

// -------- TANK (рандомизация стен) --------
void runTank() {
  waitRelease();
  playMusicTheme(TECH_DINO, OLD_DINO, CHILL_DINO, DARK_DINO, TECH_DINO, 120);

  // Mini tank battle: a fixed arena, generated corridor maze, continuous held-button
  // movement, directional shots, enemy pursuit and enemy fire.
  const int ARENA_X = 2, ARENA_Y = 17, ARENA_W = 124, ARENA_H = 45;
  const int CELL = 8, COLS = 15, ROWS = 5;
  struct Wall { int x, y, w, h; } walls[20];
  int wallCount = 0;
  struct Enemy { float x, y; int8_t dir; bool on; uint32_t nextShot; } en[4];
  struct Bullet { float x, y, vx, vy; bool on; bool enemy; } bullets[8];

  int tx = 12, ty = 54, dir = 0; // 0 up, 1 right, 2 down, 3 left
  int score = 0, best = getBest(APP_TANK), lives = 3, level = 1;
  bool over = false;
  uint32_t lastShot = 0, lastEnemyMove = 0, lastEnemySpawn = 0;

  auto tankRectHitsWall = [&](float x, float y, int halfW, int halfH) -> bool {
    if (x-halfW < ARENA_X+1 || x+halfW > ARENA_X+ARENA_W-1 ||
        y-halfH < ARENA_Y+1 || y+halfH > ARENA_Y+ARENA_H-1) return true;
    for (int i=0; i<wallCount; i++) {
      Wall &w = walls[i];
      if (x+halfW > w.x && x-halfW < w.x+w.w &&
          y+halfH > w.y && y-halfH < w.y+w.h) return true;
    }
    return false;
  };

  auto buildMaze = [&]() {
    wallCount = 0;
    // Generate wall segments on a cell grid, leaving the bottom-left spawn,
    // top entry and a few connected corridors open.
    bool blocked[ROWS][COLS] = {};
    for (int r=0; r<ROWS; r++) {
      for (int c=0; c<COLS; c++) {
        if (r==ROWS-1 && c<3) continue;
        if (r==0 && c>COLS-4) continue;
        if (random(100) < (level<3 ? 23 : 31)) blocked[r][c] = true;
      }
    }
    // Carve guaranteed corridors: bottom lane, central vertical lane and exit lane.
    for (int c=0; c<COLS; c++) blocked[ROWS-1][c] = false;
    int mid = COLS/2;
    for (int r=0; r<ROWS; r++) blocked[r][mid] = false;
    for (int r=0; r<ROWS; r++) blocked[r][COLS-2] = false;
    blocked[ROWS-1][1] = false; blocked[ROWS-2][1] = false;
    for (int r=0; r<ROWS; r++) for (int c=0; c<COLS; c++) {
      if (!blocked[r][c] || wallCount >= 20) continue;
      walls[wallCount++] = { ARENA_X+4+c*CELL, ARENA_Y+3+r*CELL, 6, 6 };
    }
  };

  auto resetRound = [&]() {
    tx=12; ty=54; dir=0; score=0; lives=3; level=1; over=false;
    for (int i=0;i<8;i++) bullets[i].on=false;
    for (int i=0;i<4;i++) en[i] = { (float)(ARENA_X+ARENA_W-12-i*18), (float)(ARENA_Y+8+(i%2)*14), 2, i<2, millis()+900u+(uint32_t)i*300u };
    buildMaze();
    lastShot=lastEnemyMove=lastEnemySpawn=millis();
  };

  for (int i=0;i<8;i++) bullets[i] = {0,0,0,0,false,false};
  for (int i=0;i<4;i++) en[i] = {0,0,2,false,0};
  buildMaze();
  for (int i=0;i<4;i++) en[i] = { (float)(ARENA_X+ARENA_W-12-i*18), (float)(ARENA_Y+8+(i%2)*14), 2, i<2, millis()+900u+(uint32_t)i*300u };

  while (true) {
    uint32_t t0=millis(), now=millis();
    pollButtons();
    if (quitPressed()) break;

    if (!over) {
      // Continuous movement while a direction button is held. Axis-separated
      // collision checks prevent tanks sticking diagonally to corners.
      int dx=0, dy=0;
      if (held(BI_LEFT))  { dx=-1; dir=3; }
      if (held(BI_RIGHT)) { dx= 1; dir=1; }
      if (held(BI_UP))    { dy=-1; dir=0; }
      if (held(BI_DOWN))  { dy= 1; dir=2; }
      int moveSpeed = (held(BI_OK) ? 2 : 1); // hold OK while moving for a quick dash; fire uses tap/repeat below
      // Use stable held state for fast movement; ordinary movement is still responsive.
      if (dx && !tankRectHitsWall(tx+dx*moveSpeed, ty, 3, 3)) tx += dx*moveSpeed;
      if (dy && !tankRectHitsWall(tx, ty+dy*moveSpeed, 3, 3)) ty += dy*moveSpeed;

      // Shoot in the current facing direction. BI_OK can fire repeatedly while held.
      if (held(BI_OK) && now-lastShot >= 220) {
        for (int i=0;i<8;i++) if (!bullets[i].on) {
          float vx=0, vy=0;
          if (dir==0) vy=-2.8f; else if (dir==1) vx=2.8f;
          else if (dir==2) vy=2.8f; else vx=-2.8f;
          bullets[i] = { (float)tx, (float)ty, vx, vy, true, false };
          lastShot=now; SFX(SF_SHOOT); break;
        }
      }

      // Move bullets and resolve wall/enemy hits.
      for (int i=0;i<8;i++) if (bullets[i].on) {
        bullets[i].x += bullets[i].vx; bullets[i].y += bullets[i].vy;
        bool outside = bullets[i].x<ARENA_X+2 || bullets[i].x>ARENA_X+ARENA_W-2 || bullets[i].y<ARENA_Y+2 || bullets[i].y>ARENA_Y+ARENA_H-2;
        bool wallHit=false;
        for (int w=0;w<wallCount;w++) if (bullets[i].x>=walls[w].x && bullets[i].x<=walls[w].x+walls[w].w && bullets[i].y>=walls[w].y && bullets[i].y<=walls[w].y+walls[w].h) wallHit=true;
        if (outside || wallHit) { bullets[i].on=false; continue; }
        if (!bullets[i].enemy) {
          for (int j=0;j<4;j++) if (en[j].on && fabsf(bullets[i].x-en[j].x)<5 && fabsf(bullets[i].y-en[j].y)<5) {
            en[j].on=false; bullets[i].on=false; score++; SFX(SF_HIT);
            if (score>best) { best=score; setBest(APP_TANK,score); }
            if (score>0 && score%5==0) { level=min(9,level+1); lives=min(5,lives+1); buildMaze(); }
            break;
          }
        } else if (fabsf(bullets[i].x-tx)<5 && fabsf(bullets[i].y-ty)<5) {
          bullets[i].on=false; lives--; SFX(SF_DIE);
          if (lives<=0) { over=true; }
        }
      }

      // Enemies periodically step through the maze toward the player and fire.
      if (now-lastEnemyMove >= (uint32_t)max(95, 230-level*12)) {
        lastEnemyMove=now;
        for (int i=0;i<4;i++) if (en[i].on) {
          float ex=en[i].x, ey=en[i].y;
          int stepX=(tx>ex+2)?1:((tx<ex-2)?-1:0);
          int stepY=(ty>ey+2)?1:((ty<ey-2)?-1:0);
          bool moved=false;
          if (random(2)==0 && stepX && !tankRectHitsWall(ex+stepX,ey,3,3)) { en[i].x+=stepX; moved=true; }
          if ((!moved || random(3)==0) && stepY && !tankRectHitsWall(en[i].x,ey+stepY,3,3)) { en[i].y+=stepY; moved=true; }
          if (!moved && stepX && !tankRectHitsWall(ex+stepX,ey,3,3)) en[i].x+=stepX;
          if (fabsf(en[i].x-tx)<6 && fabsf(en[i].y-ty)<6) {
            lives--; en[i].on=false; SFX(SF_DIE); if(lives<=0) over=true;
          }
          if (now>=en[i].nextShot) {
            for (int bidx=0;bidx<8;bidx++) if (!bullets[bidx].on) {
              float vx=0,vy=0;
              if (fabsf(tx-en[i].x)>fabsf(ty-en[i].y)) vx=(tx>en[i].x)?1.8f:-1.8f;
              else vy=(ty>en[i].y)?1.8f:-1.8f;
              bullets[bidx]={en[i].x,en[i].y,vx,vy,true,true};
              en[i].nextShot=now+(uint32_t)random(1000,1900); break;
            }
          }
        }
      }
      if (now-lastEnemySpawn > (uint32_t)max(650,1700-level*90)) {
        lastEnemySpawn=now;
        for (int i=0;i<4;i++) if (!en[i].on) {
          float sx=ARENA_X+ARENA_W-12, sy=ARENA_Y+8+(i%3)*12;
          if (!tankRectHitsWall(sx,sy,3,3)) en[i]={sx,sy,2,true,now+900};
          break;
        }
      }
      if (score>=20) { over=true; SFX(SF_WIN); }
    } else if (evDown[BI_OK]) {
      resetRound();
    }

    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    char hud[32]; snprintf(hud,sizeof(hud),"S%d H%d L%d",score,lives,level);
    oled.drawStr(0,8,hud);
    oled.drawFrame(ARENA_X,ARENA_Y,ARENA_W,ARENA_H);
    // Maze blocks are drawn with a bevel pattern to read as solid obstacles.
    for(int i=0;i<wallCount;i++) {
      Wall &w=walls[i]; oled.drawBox(w.x,w.y,w.w,w.h);
      oled.drawHLine(w.x,w.y,w.w); oled.drawVLine(w.x,w.y,w.h);
      oled.drawPixel(w.x+w.w-1,w.y+w.h-1);
    }
    for(int i=0;i<8;i++) if(bullets[i].on) {
      if (fabsf(bullets[i].vx)>0) oled.drawBox((int)bullets[i].x-1,(int)bullets[i].y,4,2);
      else oled.drawBox((int)bullets[i].x,(int)bullets[i].y-1,2,4);
    }
    // Distinct enemy tank silhouettes, tracks and directional turret.
    for(int i=0;i<4;i++) if(en[i].on) {
      int ex=(int)en[i].x, ey=(int)en[i].y;
      oled.drawFrame(ex-4,ey-4,8,8); oled.drawBox(ex-2,ey-2,4,4);
      oled.drawPixel(ex-3,ey-5); oled.drawPixel(ex+3,ey-5);
      oled.drawPixel(ex-3,ey+4); oled.drawPixel(ex+3,ey+4);
      if (fabsf(tx-ex)>fabsf(ty-ey)) oled.drawHLine(ex+(tx>ex?1:-5),ey,5);
      else oled.drawVLine(ex,ey+(ty>ey?1:-5),5);
    }
    // Player tank has a clear body and a turret matching the facing direction.
    oled.drawFrame(tx-4,ty-4,8,8); oled.drawBox(tx-2,ty-2,4,4);
    if(dir==0) oled.drawVLine(tx,ty-7,5); else if(dir==1) oled.drawHLine(tx+1,ty,6);
    else if(dir==2) oled.drawVLine(tx,ty+2,5); else oled.drawHLine(tx-6,ty,6);
    if(over) overlayBox(score>=20?"VICTORY!":"GAME OVER","OK: retry / BACK: exit");
    present(); frameWait(t0);
  }
}

// -------- CATCH STARS (медленнее звёзды, быстрее платформа) --------
void runStars() {
  waitRelease();
  float bagX=60;
  float stars[10][3];
  int starCount=0, score=0, best=getBest(APP_STARS), lives=3, combo=0;
  uint32_t nextStar = millis()+600;
  for (int i=0;i<10;i++) stars[i][0]=-1;
  while (true) {
    uint32_t t0=millis(), now=millis();
    pollButtons();
    if (quitPressed()) break;
    if (lives>0) {
      // Платформа в 2 раза быстрее
      float move = (cfg.difficulty==0?3.2f:cfg.difficulty==2?5.6f:4.2f);
      if (held(BI_LEFT)) bagX -= move;
      if (held(BI_RIGHT)) bagX += move;
      bagX = constrain((int)bagX, 9, 119);
      if (now>=nextStar && starCount<10) {
        int slot=-1;
        for (int i=0;i<10;i++) if (stars[i][0]<0) { slot=i; break; }
        if (slot>=0) {
          stars[slot][0]=random(5,123);
          stars[slot][1]=6;
          stars[slot][2]=(random(0,6)==0)?1:0;
          starCount++;
        }
        nextStar = now + random(cfg.difficulty==0?650:400, cfg.difficulty==0?1050:750);
      }
      for (int i=0;i<10;i++) {
        if (stars[i][0]<0) continue;
        // Медленнее падают
        float fallSpeed = (stars[i][2]>0 ? 0.5f : 0.4f) * (cfg.difficulty==2?1.25f:1.0f);
        stars[i][1] += fallSpeed;
        if (stars[i][1]>=53 && stars[i][1]<=61 && abs((int)stars[i][0]-(int)bagX)<10) {
          if (stars[i][2]>0) { score+=3; combo+=2; }
          else { score++; combo++; }
          stars[i][0]=-1; starCount--;
          SFX(SF_EAT);
        } else if (stars[i][1]>63) {
          // Missing a star is allowed; the game rewards catching many, not perfection.
          stars[i][0]=-1; starCount--;
          combo=0;
        }
      }
      if (score>best) { best=score; setBest(APP_STARS,best); }
    } else if (evDown[BI_OK]) {
      bagX=60; starCount=0; score=0; lives=3; combo=0; nextStar=now+600;
      for (int i=0;i<10;i++) stars[i][0]=-1;
    }
    oled.clearBuffer();
    oled.drawRBox((int)bagX-8,56,16,6,3);
    for (int i=0;i<10;i++) if (stars[i][0]>=0 && stars[i][1]<61) {
      int x=stars[i][0], y=stars[i][1];
      if (stars[i][2]>0) { oled.drawFrame(x-2,y-2,5,5); oled.drawPixel(x,y-2); }
      else { oled.drawPixel(x,y); oled.drawPixel(x,y+2); oled.drawPixel(x-1,y+1); oled.drawPixel(x+1,y+1); }
    }
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,8,("S "+String(score)+" L "+String(lives)+" C "+String(combo)).c_str());
    if (lives<=0) overlayBox("GAME OVER","OK retry / BACK exit");
    present(); frameWait(t0);
  }
}

// -------- REACTION (с временем в мс и защитой от фальстарта) --------
void runReaction() {
  waitRelease();
  int tries = 0;
  int bestMs = 9999;
  uint32_t waitUntil = millis() + random(1500, 4000);
  bool waiting = true;
  bool ready = false;
  bool falseStart = false;
  uint32_t showTime = 0;
  uint32_t falseStartAt = 0;

  while (true) {
    uint32_t t0 = millis(), now = millis();
    pollButtons();
    if (quitPressed()) break;

    if (waiting && now >= waitUntil) {
      waiting = false;
      ready = true;
      showTime = now;
      SFX(SF_OPEN);
    }

    if (waiting && evDown[BI_OK]) {
      falseStart = true;
      falseStartAt = now;
      SFX(SF_DIE);
    }

    if (ready && evDown[BI_OK]) {
      uint32_t ms = now - showTime;
      if (ms < bestMs) { bestMs = ms; setBest(APP_REACTION, bestMs); }
      tries++;
      ready = false;
      if (tries >= 5) {
        char buf[32];
        snprintf(buf,sizeof(buf),"Best: %d ms", bestMs);
        oled.clearBuffer();
        oled.setFont(u8g2_font_6x13_t_cyrillic);
        oled.drawStr(10,20,"RESULTS");
        oled.setFont(u8g2_font_5x8_t_cyrillic);
        oled.drawStr(20,40,buf);
        oled.drawStr(20,52,"OK: restart");
        present();
        waitRelease();
        while (true) {
          pollButtons();
          if (evDown[BI_OK]) {
            tries=0; bestMs=9999;
            waiting=true; ready=false;
            waitUntil = millis() + random(1500,4000);
            break;
          }
          if (quitPressed()) return;
          present(); delay(30);
        }
      } else {
        waiting = true;
        waitUntil = now + random(1500, 4000);
      }
    }

    if (falseStart && now - falseStartAt > 1200) falseStart = false;

    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    if (falseStart) {
      oled.drawStr(20,20,"FALSE START!");
      oled.setFont(u8g2_font_5x8_t_cyrillic);
      oled.drawStr(40,40,"Wait...");
    } else if (waiting) {
      oled.drawStr(20,20,"Wait...");
    } else if (ready) {
      uint32_t elapsed = now - showTime;
      oled.drawStr(20,20,"PRESS!");
      oled.setFont(u8g2_font_5x8_t_cyrillic);
      oled.drawStr(40,40,(String(elapsed)+" ms").c_str());
    }
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("TRIES "+String(tries)+"/5 BEST "+String(bestMs)+"ms").c_str());
    present(); frameWait(t0);
  }
}

// -------- LABYRINTH (новый случайный лабиринт с каждым уровнем) --------
void runLabyrinth() {
  waitRelease();
  playMusicTheme(TECH_SNAKE,OLD_SNAKE,CHILL_SNAKE,DARK_SNAKE,TECH_SNAKE,140);
  const int MW=16, MH=8;
  uint8_t maze[8][16];

  int level = prefs.getInt("mazeLevel",1);
  if (level<1) level=1;

  auto generateMaze = [&]() {
    memset(maze,1,sizeof(maze));
    const int CW=7, CH=3;
    bool visited[CH][CW] = {};
    struct Cell { int x,y; };
    Cell stack[CW*CH];
    int sp = 0;
    stack[sp++] = {0,0};
    visited[0][0] = true;
    maze[1][1] = 0;
    while (sp>0) {
      Cell cur = stack[sp-1];
      int dirs[4] = {0,1,2,3};
      for (int i=3;i>0;--i) { int j=random(i+1), t=dirs[i]; dirs[i]=dirs[j]; dirs[j]=t; }
      bool advanced = false;
      for (int k=0;k<4;k++) {
        int nx=cur.x, ny=cur.y;
        if (dirs[k]==0) ny--;
        if (dirs[k]==1) nx++;
        if (dirs[k]==2) ny++;
        if (dirs[k]==3) nx--;
        if (nx<0||nx>=CW||ny<0||ny>=CH||visited[ny][nx]) continue;
        int x1=1+cur.x*2, y1=1+cur.y*2;
        int x2=1+nx*2, y2=1+ny*2;
        maze[y1][x1]=0;
        maze[(y1+y2)/2][(x1+x2)/2]=0;
        maze[y2][x2]=0;
        visited[ny][nx] = true;
        stack[sp++] = {nx,ny};
        advanced = true;
        break;
      }
      if (!advanced) --sp;
    }
    // Выход всегда в правом нижнем проходе
    maze[5][13]=0;
    maze[5][14]=0;
    maze[5][15]=0;
    // Открытые проходы: больше на низком уровне, меньше на высоком
    int openings = (level<3)?3:(level<6)?1:0;
    for (int n=0;n<openings;n++) {
      int x = random(2,14), y = random(1,7);
      if (maze[y][x]) maze[y][x] = 0;
    }
  };

  generateMaze();

  int px=1, py=1;
  int score = 0;
  int best = getBest(APP_LABYRINTH);
  bool escaped = false;

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;

    if (!escaped) {
      bool moved = false;
      // Используем nav() — одиночный шаг + автоповтор
      if (nav(BI_UP)    && py>0     && !maze[py-1][px]) { py--; moved=true; }
      if (nav(BI_DOWN)  && py<MH-1  && !maze[py+1][px]) { py++; moved=true; }
      if (nav(BI_LEFT)  && px>0     && !maze[py][px-1]) { px--; moved=true; }
      if (nav(BI_RIGHT) && px<MW-1  && !maze[py][px+1]) { px++; moved=true; }
      if (moved) score++;
      if (px == MW-1 && py == 5) {
        escaped = true;
        // Уровень увеличиваем только после победы — сохраняем в NVS
        level++;
        if (level>999) level=999;
        prefs.putInt("mazeLevel", level);
        if (level > best) { best=level; setBest(APP_LABYRINTH,level); }
        SFX(SF_WIN);
      }
    } else if (evDown[BI_OK]) {
      // Новый уровень — новый случайный лабиринт
      generateMaze();
      px=1; py=1; score=0; escaped=false;
    }

    oled.clearBuffer();
    oled.drawFrame(4,2,120,48);
    for (int r=0;r<MH;r++) for (int c=0;c<MW;c++)
      if (maze[r][c]) oled.drawBox(4+c*7+1, 2+r*6+1, 3, 3);
    oled.drawFrame(4+(MW-1)*7+1, 2+5*6+1, 5, 5);
    oled.drawDisc(4+px*7+4, 2+py*6+4, 2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("LV "+String(level)+" STEPS "+String(score)).c_str());
    if (escaped) overlayBox("ESCAPED!","OK: new maze");
    present(); frameWait(t0);
  }
}

// -------- JUMPER (ПОЛНОСТЬЮ ПЕРЕПИСАН: камера следит) --------
void runJumper() {
  waitRelease();
  playMusicTheme(TECH_DINO,OLD_DINO,CHILL_DINO,DARK_DINO,TECH_DINO,130);

  const int NUM_PLAT = 6;
  float jx = 60, jy = 50;
  float vy = 0;
  int score = 0;
  int best = getBest(APP_JUMPER);
  float platforms[NUM_PLAT][2]; // x, y (экранные)
  bool dead = false;

  auto resetJumper = [&]() {
    jx = 60; jy = 50; vy = -5.0f;
    score = 0; dead = false;
    for (int i=0;i<NUM_PLAT;i++) {
      platforms[i][0] = random(10, 110);
      platforms[i][1] = 60 - i * 10;
    }
  };
  resetJumper();

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;

    if (!dead) {
      float moveStep = difficultyFor(APP_JUMPER)==0 ? 2.8f : (difficultyFor(APP_JUMPER)==2 ? 2.1f : 2.5f);
      if (held(BI_LEFT)) jx -= moveStep;
      if (held(BI_RIGHT)) jx += moveStep;
      if (jx < 5) jx = 5;
      if (jx > 118) jx = 118;

      // Физика
      vy += (difficultyFor(APP_JUMPER)==0 ? 0.29f : 0.35f);
      jy += vy;

      // Коллизии с платформами (только при падении)
      for (int i=0;i<NUM_PLAT;i++) {
        if (vy > 0 &&
            jy + 3 >= platforms[i][1] && jy + 3 <= platforms[i][1] + 4 &&
            jx + 4 >= platforms[i][0] && jx - 4 <= platforms[i][0] + 14) {
          vy = (difficultyFor(APP_JUMPER)==0 ? -6.0f : -5.5f);
          jy = platforms[i][1] - 3;
          score++;
          SFX(SF_JUMP);
        }
      }

      // Камера — держим персонажа в экране
      if (jy < 25) {
        float diff = 25 - jy;
        jy = 25;
        for (int i=0;i<NUM_PLAT;i++) {
          platforms[i][1] += diff;
          // Рециклинг платформ
          if (platforms[i][1] > 64) {
            platforms[i][1] = -5;
            platforms[i][0] = random(5, 115);
          }
        }
      }

      // Смерть — упал вниз
      if (jy > 64) {
        dead = true;
        if (score > best) best = score;
        setBest(APP_JUMPER, score);
        SFX(SF_DIE);
      }
    } else if (evDown[BI_OK]) {
      resetJumper();
    }

    oled.clearBuffer();
    for (int i=0;i<NUM_PLAT;i++) {
      int px = (int)platforms[i][0];
      int py = (int)platforms[i][1];
      if (py >= -3 && py <= 64) oled.drawRBox(px, py, 14, 3, 1);
    }
    oled.drawDisc((int)jx, (int)jy, 3);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("JUMPS "+String(score)+" HI "+String(best)).c_str());
    if (dead) overlayBox("FELL!","OK: retry");
    present(); frameWait(t0);
  }
}

// =====================================================
//  МЕНЮ (с long-press OK → документация)
// =====================================================
const int STAR_X[8] = {6,22,40,58,84,104,120,30};
const int STAR_Y[8] = {16,40,22,58,20,46,30,62};

void drawMenu(float cur, int frameN) {
  oled.clearBuffer();
  for (int i=0;i<8;i++)
    if (((frameN/6)+i)%5==0) oled.drawPixel(STAR_X[i],STAR_Y[i]);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  oled.drawStr(2,8,"MINI BOY v5.1");
  String cnt = padNum(menuSel+1,2)+"/"+String(menuAppCount);
  oled.drawStr(60,8,cnt.c_str());
  drawWifiStatus(116,0);
  const int cy = 28;
  for (int i=0;i<menuAppCount;i++) {
    float d = (float)i - cur;
    if (d > menuAppCount/2.0f)  d -= menuAppCount;
    if (d < -menuAppCount/2.0f) d += menuAppCount;
    float off = d * CAR_SPACING;
    if (off < -70 || off > 70) continue;
    float ao = off<0?-off:off;
    float k = ao / CAR_SPACING;
    if (k > 1) k = 1;
    int cx = 64 + (int)off;
    int size = (int)(36 - 8*k);
    int x = cx - size/2, y = cy - size/2;
    oled.drawRFrame(x, y, size, size, 6);
    if (k < 0.5f) oled.drawRFrame(x+2, y+2, size-4, size-4, 4);
    if (k < 0.9f) drawIcon(i, cx, cy);
  }
  // Подписи видимых боковых иконок: короткие ASCII-теги читаются даже
  // на краях дисплея, где полное название не помещается.
  oled.setFont(u8g2_font_4x6_tr);
  for (int i=0;i<menuAppCount;i++) {
    int d=i-menuSel;
    if (d > menuAppCount/2) d-=menuAppCount;
    if (d < -menuAppCount/2) d+=menuAppCount;
    if (d==0 || d < -1 || d > 1) continue;
    int cx=64+d*CAR_SPACING;
    if (cx<10 || cx>118) continue;
    const char* tag="APP";
    switch(i) {
      case APP_DINO: tag="DINO"; break; case APP_FLAPPY: tag="FLAP"; break;
      case APP_TETRIS: tag="TETR"; break; case APP_SNAKE: tag="SNAK"; break;
      case APP_PONG: tag="PONG"; break; case APP_BRICK: tag="BRK"; break;
      case APP_INVADE: tag="INV"; break; case APP_ASTER: tag="ASTR"; break;
      case APP_MINES: tag="MINE"; break; case APP_2048: tag="2048"; break;
      case APP_SIMON: tag="SIMN"; break; case APP_FROGGER: tag="FROG"; break;
      case APP_TTT: tag="TTT"; break; case APP_GEMINI: tag="AI"; break;
      case APP_SNAKE_MAZE: tag="MAZE"; break; case APP_MEMORY: tag="MEM"; break;
      case APP_SKATE: tag="SK8"; break; case APP_TANK: tag="TANK"; break;
      case APP_STARS: tag="STAR"; break; case APP_REACTION: tag="REACT"; break;
      case APP_LABYRINTH: tag="LAB"; break; case APP_JUMPER: tag="JUMP"; break;
      case APP_PIXEL_QUEST: tag="MARIO"; break; case APP_SETTINGS: tag="SET"; break;
      case APP_FILE_MANAGER: tag="FILES"; break;
      case APP_NOTES: tag="NOTE"; break;
    }
    int tw=oled.getStrWidth(tag);
    oled.drawStr(cx-tw/2,49,tag);
  }
  // UTF-8-safe selected title: use a compact font and drawUTF8, never remove
  // arbitrary bytes from a Cyrillic string.
  oled.setFont(u8g2_font_5x8_t_cyrillic);
const char* name = appName(menuSel);
if (!name) name = "?";
int nameW = oled.getUTF8Width(name);
  if(nameW<=126) oled.drawUTF8((128-nameW)/2,62,name);
  else {
    // All apps also have a compact tag above; selected title stays readable.
    oled.drawUTF8(1,62,name);
  }
  present();
}

// ═══════════════════════════════════════════════════════════════════════════
//  SUPER MARIO — полноценный платформер
//  4 уровня + босс с 3 фазами. Камера X/Y, движущиеся платформы,
//  чекпоинты, сохранение прогресса, 5 типов врагов, частицы, 3 фона.
//  Интеграция: APP_PIXEL_QUEST, отображаемое имя Super Mario, case в launchApp(), иконка, docs.
// ═══════════════════════════════════════════════════════════════════════════

namespace pq {

// ─────────────────────────────────────────────────────
//  КОНСТАНТЫ
// ─────────────────────────────────────────────────────
const uint8_t  TILE        = 8;
const uint8_t  VIEW_W      = 16;
const uint8_t  VIEW_H      = 8;
const int      MAX_W       = 64;
const int      MAX_H       = 24;
const int      MAX_ENEMIES = 20;
const int      MAX_ITEMS   = 40;
const int      MAX_PARTS   = 24;
const int      MAX_PROJ    = 8;
const int      MAX_PLATS   = 8;

const float GRAVITY        = 0.30f;
const float GRAVITY_WALL   = 0.10f;
const float JUMP_V         = -4.4f;
const float JUMP_V_WALL    = -3.6f;
const float MAX_FALL       = 5.0f;
const float WALK_SPEED     = 1.5f;
const float RUN_SPEED      = 2.4f;
const float ACCEL          = 0.35f;
const float FRICTION       = 0.55f;
const float AIR_FRICTION   = 0.85f;
const float COYOTE_TIME    = 100;
const float JUMP_BUFFER    = 120;
const float CAM_LERP       = 0.20f;
const float CAM_Y_LERP     = 0.12f;
const float INVULN_TIME    = 1500;
const float STAR_TIME      = 7000;
const float BOOST_TIME     = 5000;

// ─────────────────────────────────────────────────────
//  ТИПЫ
// ─────────────────────────────────────────────────────
enum PQState : uint8_t {
  PS_INTRO, PS_PLAY, PS_PAUSE, PS_DEAD, PS_LEVEL_CLEAR,
  PS_GAME_OVER, PS_VICTORY
};

enum TileType : uint8_t {
  T_EMPTY=0, T_SOLID, T_BRICK, T_QUESTION, T_USED_Q, T_SPIKE,
  T_PLATFORM, T_MOV_H, T_MOV_V, T_CHECKPOINT, T_FLAG,
  T_DECO_BRICK, T_DECO_SKULL, T_WATER, T_LAVA
};

enum EnemyType : uint8_t {
  E_PATROL, E_EDGE, E_FAST, E_GUARD, E_HEAVY, E_FLYER, E_TURRET
};

enum ItemType : uint8_t {
  I_COIN, I_STAR, I_HEART, I_BOOST, I_GEM
};

enum ParticleType : uint8_t {
  PT_DUST, PT_SPARKLE, PT_DEBRIS, PT_BLOOD
};

enum BossPhase : uint8_t {
  BP_DORMANT, BP_PHASE1, BP_PHASE2, BP_PHASE3, BP_DYING
};

// ─────────────────────────────────────────────────────
//  СТРУКТУРЫ
// ─────────────────────────────────────────────────────
struct Player {
  float x, y, vx, vy;
  bool onGround, wasOnGround, facingRight;
  bool invuln, star, boosted;
  uint32_t invulnUntil, starUntil, boostUntil;
  uint32_t lastGroundTime, jumpBufferTime;
  uint8_t animFrame, hp, maxHp;
  uint32_t animTimer;
  bool dead, dying;
  uint32_t deadAt;
  uint8_t coyoteFrames;
};

struct Enemy {
  float x, y, vx, vy;
  int16_t originX, originY;
  uint8_t type;
  bool alive, onGround;
  int8_t dir;
  int8_t hp;
  int16_t patrolMin, patrolMax;
  uint32_t flashUntil, animTimer, shootTimer;
  uint8_t animFrame;
  float baseSpeed;
};

struct Item {
  int16_t x, y;
  uint8_t type;
  bool collected;
  uint8_t bobFrame;
  uint32_t bobTimer;
};

struct Particle {
  float x, y, vx, vy;
  uint8_t type, life, maxLife;
  bool active;
};

struct Projectile {
  float x, y, vx, vy;
  bool active, fromPlayer;
  uint8_t life;
};

struct MovingPlatform {
  int16_t tileX, tileY;
  float offset, minOff, maxOff;
  int8_t dir;
  uint8_t axis;   // 0=horiz, 1=vert
  float speed;
};

struct Boss {
  float x, y, vx, vy;
  int16_t homeX, homeY;
  int8_t hp, maxHp;
  uint8_t phase;
  uint32_t phaseTimer, attackTimer, animTimer;
  uint8_t animFrame;
  bool facingLeft;
  bool alive;
  uint32_t deathTimer;
};

// ─────────────────────────────────────────────────────
//  СПРАЙТЫ — 8 широкие, row-major, bit0=левый пиксель
//  (U8g2 drawXBMP: 1 байт на строку при ширине 8)
// ─────────────────────────────────────────────────────

// ── Герой 8×12 — стоит ──
static const uint8_t sprHeroIdle[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0xDB, 0x7E, 0x3C,
  0x7E, 0xFF, 0xFF, 0x7E, 0x66, 0x66
};
// ── Герой — бег кадр 0 ──
static const uint8_t sprHeroRun0[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0xDB, 0x7E, 0x3C,
  0x7E, 0xFF, 0xFF, 0x7E, 0xC3, 0x42
};
// ── Герой — бег кадр 1 ──
static const uint8_t sprHeroRun1[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0xDB, 0x7E, 0x3C,
  0x7E, 0xFF, 0xFF, 0x7E, 0x42, 0xC3
};
// ── Герой — прыжок ──
static const uint8_t sprHeroJump[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0xDB, 0x7E, 0x3C,
  0x7E, 0xFF, 0xFF, 0x66, 0x66, 0x00
};
// ── Герой — падение ──
static const uint8_t sprHeroFall[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0xDB, 0x7E, 0x3C,
  0x7E, 0xFF, 0xFF, 0x7E, 0x18, 0x18
};
// ── Герой — удар ──
static const uint8_t sprHeroHurt[] PROGMEM = {
  0x18, 0x3C, 0x7E, 0x5A, 0x7E, 0x3C,
  0x7E, 0x7E, 0x7E, 0x7E, 0x66, 0x66
};
// ── Герой — смерть ──
static const uint8_t sprHeroDead[] PROGMEM = {
  0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xFF,
  0x7E, 0x3C, 0x18, 0x00, 0x00, 0x00
};

// ── Враги 8×8 ──
static const uint8_t sprE_Patrol0[] PROGMEM = {0x3C,0x7E,0xFF,0xDB,0xFF,0x7E,0x3C,0x00};
static const uint8_t sprE_Patrol1[] PROGMEM = {0x3C,0x7E,0xFF,0xDB,0xFF,0x7E,0x18,0x24};
static const uint8_t sprE_Edge[]    PROGMEM = {0x18,0x3C,0x7E,0xFF,0xFF,0x7E,0x3C,0x18};
static const uint8_t sprE_Fast0[]   PROGMEM = {0x66,0xFF,0xFF,0xDB,0xFF,0x7E,0x3C,0x18};
static const uint8_t sprE_Fast1[]   PROGMEM = {0x00,0x66,0xFF,0xFF,0xDB,0xFF,0x7E,0x3C};
static const uint8_t sprE_Guard[]   PROGMEM = {0x3C,0x7E,0xFF,0xFF,0xFF,0x7E,0x3C,0x00};
static const uint8_t sprE_Heavy[]   PROGMEM = {0x7E,0xFF,0xDB,0xFF,0xFF,0xDB,0xFF,0x7E};
static const uint8_t sprE_Flyer0[]  PROGMEM = {0x18,0x3C,0x66,0xFF,0xFF,0x66,0x3C,0x18};
static const uint8_t sprE_Flyer1[]  PROGMEM = {0x3C,0x66,0xC3,0xDB,0xDB,0xC3,0x66,0x3C};
static const uint8_t sprE_Turret[]  PROGMEM = {0x00,0x3C,0x7E,0xFF,0xFF,0xFF,0x7E,0x3C};

// ── Босс 16×16 (32 байта: 2 байта/строку) ──
static const uint8_t sprBoss0[] PROGMEM = {
  0x00,0x00, 0x7E,0x7E, 0xFF,0xFF, 0xDB,0xDB, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C, 0x3C,0x3C,
  0x7E,0x7E, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xDB,0xDB, 0x7E,0x7E, 0x3C,0x3C, 0x00,0x00
};
static const uint8_t sprBoss1[] PROGMEM = {
  0x00,0x00, 0x3C,0x3C, 0x7E,0x7E, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C,
  0x7E,0x7E, 0xDB,0xDB, 0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C, 0x18,0x18, 0x00,0x00
};
static const uint8_t sprBoss2[] PROGMEM = {
  0x18,0x18, 0x3C,0x3C, 0x7E,0x7E, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E,
  0x7E,0x7E, 0xFF,0xFF, 0xDB,0xDB, 0xFF,0xFF, 0xFF,0xFF, 0x7E,0x7E, 0x3C,0x3C, 0x18,0x18
};

// ── Предметы 8×8 ──
static const uint8_t sprCoin0[] PROGMEM = {0x18,0x3C,0x66,0x66,0x66,0x66,0x3C,0x18};
static const uint8_t sprCoin1[] PROGMEM = {0x18,0x24,0x42,0x42,0x42,0x42,0x24,0x18};
static const uint8_t sprStar[]  PROGMEM = {0x18,0x3C,0xFF,0x7E,0x3C,0x66,0x42,0x00};
static const uint8_t sprHeart[] PROGMEM = {0x66,0xFF,0xFF,0xFF,0x7E,0x3C,0x18,0x00};
static const uint8_t sprBoost[] PROGMEM = {0x18,0x3C,0x18,0x3C,0x7E,0x18,0x18,0x18};
static const uint8_t sprGem[]   PROGMEM = {0x18,0x3C,0x7E,0xFF,0x7E,0x3C,0x18,0x00};

// ── Тайлы 8×8 ──
static const uint8_t sprSolid[]    PROGMEM = {0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF};
static const uint8_t sprBrick[]    PROGMEM = {0xFF,0x81,0xBD,0x81,0xBD,0x81,0xBD,0xFF};
static const uint8_t sprQuestion[] PROGMEM = {0xFF,0x81,0xBD,0x99,0xA5,0xBD,0x81,0xFF};
static const uint8_t sprUsedQ[]    PROGMEM = {0xFF,0x81,0xBD,0xBD,0xBD,0xBD,0x81,0xFF};
static const uint8_t sprSpike[]    PROGMEM = {0x18,0x18,0x3C,0x3C,0x7E,0x7E,0xFF,0xFF};
static const uint8_t sprFlag[]     PROGMEM = {0x10,0x30,0x70,0xFF,0xFF,0x70,0x30,0x10};
static const uint8_t sprCheckpoint[] = {0x10,0x10,0x10,0x10,0x10,0x38,0x7C,0xFE};
static const uint8_t sprPlat[]     PROGMEM = {0xFF,0xFF,0x81,0x81,0xFF,0xFF,0x00,0x00};
static const uint8_t sprLava[]     PROGMEM = {0xAA,0x55,0xAA,0x55,0xFF,0xFF,0xFF,0xFF};

// ─────────────────────────────────────────────────────
//  КАРТЫ УРОВНЕЙ
//  Легенда: # земля, = платформа, B кирпич, ? вопрос, ^ шип,
//            C монета, S старт, F флаг, P чекпоинт, H движ-гор, V движ-верт,
//            E патруль, D край, R быстрый, G охранник, T тяжёлый,
//            * звезда, + сердце, > ускорение, & самоцвет, ~ вода,
//            X босс, M декор-кирпич, K декор-череп
// ─────────────────────────────────────────────────────

// Уровень 1 — «Холмы», 40×12, обучающий
static const char LV1[] PROGMEM =
  "........................................"
  "........................................"
  "..........................C.C.C........."
  "..........................BBBBB........."
  ".....C..C.......C.C.....................C"
  "....BBBBBB.....BBBBB...E................"
  "........................................"
  "....E.....D...........==H==.......E....."
  "....===...##...####............####....."
  "..S.....^^^...^^^...P...D..^^^.....F...."
  "########################################"
  "########################################";

// Уровень 2 — «Пещеры», 48×12
static const char LV2[] PROGMEM =
  "................................................"
  "................................................"
  "..........C.C.C.......C.C.C.........C.C.C......."
  "..........BBBBB.......BBBBB.........BBBBB......."
  "..........*..........R....R..........+.........."
  "....C.C.........C.........C.C.C..............C.."
  "....BBBB.......BBB.......BBBBB..E..........BBB.."
  "................................................"
  "..E..D...==H==...E..D....V....E..D...==V==.R...."
  "................................................"
  "..S...^^^....P....^^^.....P...^^^....D.....F...."
  "################################################"
  "################################################";

// Уровень 3 — «Замок», 56×12
static const char LV3[] PROGMEM =
  "........................................................"
  "........................................................"
  ".....C.C.C.........C.C.C.........C.C.C.........C.C.C...."
  ".....BBBBB.........BBBBB.........BBBBB.........BBBBB...."
  ".....T.............T.............T.............T........"
  "....C.C.....&.....C.C.....&.....C.C.....&.....C.C......."
  "....BBBB..........BBBB..........BBBB..........BBBB......"
  "........................................................"
  "..E..G..===H===.D....G....==V==..D..G....===H===.E..G..."
  "...............................P........................"
  "..S...^^^....P....^^^....P...^^^....P...^^^....E....F..."
  "########################################################"
  "########################################################";

// Уровень 4 — «Тронный зал» (босс), 48×12
static const char LV4[] PROGMEM =
  "................................................"
  "................................................"
  "....M.M.M.M.M.M.M.M.M.M.M.M.M.M.M.M.M.M.M.M....."
  "................................................"
  "................................................"
  "................................................"
  ".......C.C.C...................C.C.C............"
  ".......BBBBB.......+...........BBBBB............"
  "................................................"
  "................................................"
  "..S..P...E.....P.....X.....P.....E...P.....F...."
  "################################################"
  "################################################";

struct LevelDef {
  const char* map;
  uint8_t w, h;
  const char* name;
  uint8_t bgTheme;   // 0=sky 1=cave 2=castle 3=throne
};

static const LevelDef LEVELS[4] = {
  { LV1, 40, 12, "HILLS",  0 },
  { LV2, 48, 12, "CAVES",  1 },
  { LV3, 56, 12, "CASTLE", 2 },
  { LV4, 48, 12, "BOSS",   3 }
};
const uint8_t LEVEL_COUNT = 4;

// ─────────────────────────────────────────────────────
//  ГЛОБАЛЬНОЕ СОСТОЯНИЕ
// ─────────────────────────────────────────────────────
PQState gState = PS_INTRO;
uint8_t gLevel = 0;
uint8_t gUnlockedLevel = 0;

Player gPlayer;
Enemy  gEnemies[MAX_ENEMIES];
Item   gItems[MAX_ITEMS];
Particle gParticles[MAX_PARTS];
Projectile gProj[MAX_PROJ];
MovingPlatform gPlats[MAX_PLATS];
Boss   gBoss;
int gEnemyCount = 0, gItemCount = 0, gPlatCount = 0;

uint8_t gMap[MAX_H][MAX_W];
bool    gQUsed[MAX_H][MAX_W];
uint8_t gMapW = 0, gMapH = 0;

float gCamX = 0, gCamY = 0;
float gCamTargetX = 0, gCamTargetY = 0;

int gScore = 0;
int gCoins = 0;
int gBest = 0;
int gCoinsBest = 0;
int gLevelsCleared = 0;
int gCompletions = 0;

uint32_t gStateTimer = 0;
uint32_t gLevelStart = 0;
uint32_t gFrame = 0;
uint32_t gInvulnBlink = 0;
bool gPauseOkLock = false;

int16_t gCheckpointX = 0, gCheckpointY = 0;
bool    gHasCheckpoint = false;

// ─────────────────────────────────────────────────────
//  ЗВУК Super Mario: используем общий неблокирующий аудиодвижок.
//  Это не даёт двум независимым генераторам одновременно менять LEDC.
// ─────────────────────────────────────────────────────
const Step PQ_JUMP_SFX[]       = {{740,28},{990,30},{1320,35}};
const Step PQ_COIN_SFX[]       = {{1320,35},{1760,45},{2093,55}};
const Step PQ_BRICK_SFX[]      = {{240,45},{180,65}};
const Step PQ_HURT_SFX[]       = {{330,45},{247,60},{165,90}};
const Step PQ_DIE_SFX[]        = {{660,80},{494,90},{330,110},{220,180}};
const Step PQ_POWER_SFX[]      = {{880,45},{1175,45},{1568,70},{1760,90}};
const Step PQ_STOMP_SFX[]      = {{260,35},{390,45}};
const Step PQ_CHECKPOINT_SFX[] = {{784,50},{988,50},{1175,70},{1568,100}};
const Step PQ_BOSS_SFX[]       = {{220,65},{165,80},{110,110}};
const Step PQ_LEVEL_SFX[]      = {{784,70},{988,70},{1175,70},{1568,180}};
const Step PQ_VICTORY_SFX[]    = {{523,90},{659,90},{784,90},{1046,110},{1318,260}};

void pushTone(uint16_t f, uint16_t ms) {
  // Совместимость с прежними вызовами: каждый звук проигрывается через
  // общий SFX-движок, который обслуживается в present()/audioTick().
  static Step one[1];
  one[0].f=f; one[0].ms=ms;
  sfxPlay(one,1);
}
void soundTick() { audioTick(); }
void sJump()      { sfxPlay(PQ_JUMP_SFX, sizeof(PQ_JUMP_SFX)/sizeof(PQ_JUMP_SFX[0])); }
void sCoin()      { sfxPlay(PQ_COIN_SFX, sizeof(PQ_COIN_SFX)/sizeof(PQ_COIN_SFX[0])); }
void sBrick()     { sfxPlay(PQ_BRICK_SFX, sizeof(PQ_BRICK_SFX)/sizeof(PQ_BRICK_SFX[0])); }
void sHurt()      { sfxPlay(PQ_HURT_SFX, sizeof(PQ_HURT_SFX)/sizeof(PQ_HURT_SFX[0])); }
void sDie()       { sfxPlay(PQ_DIE_SFX, sizeof(PQ_DIE_SFX)/sizeof(PQ_DIE_SFX[0])); }
void sPower()     { sfxPlay(PQ_POWER_SFX, sizeof(PQ_POWER_SFX)/sizeof(PQ_POWER_SFX[0])); }
void sStomp()     { sfxPlay(PQ_STOMP_SFX, sizeof(PQ_STOMP_SFX)/sizeof(PQ_STOMP_SFX[0])); }
void sCheckpoint(){ sfxPlay(PQ_CHECKPOINT_SFX, sizeof(PQ_CHECKPOINT_SFX)/sizeof(PQ_CHECKPOINT_SFX[0])); }
void sBossHit()   { sfxPlay(PQ_BOSS_SFX, sizeof(PQ_BOSS_SFX)/sizeof(PQ_BOSS_SFX[0])); }
void sLevelWin()  { sfxPlay(PQ_LEVEL_SFX, sizeof(PQ_LEVEL_SFX)/sizeof(PQ_LEVEL_SFX[0])); }
void sVictory()   { sfxPlay(PQ_VICTORY_SFX, sizeof(PQ_VICTORY_SFX)/sizeof(PQ_VICTORY_SFX[0])); }

// ─────────────────────────────────────────────────────
//  УТИЛИТЫ
// ─────────────────────────────────────────────────────
inline int absI(int v) { return v < 0 ? -v : v; }
inline float fmin(float a, float b) { return a < b ? a : b; }
inline float fmax(float a, float b) { return a > b ? a : b; }
inline float fclamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline uint8_t tileAt(int tx, int ty) {
  if (tx < 0 || tx >= gMapW || ty < 0 || ty >= gMapH) return T_SOLID;
  return gMap[ty][tx];
}
inline bool tileSolid(int tx, int ty) {
  uint8_t t = tileAt(tx, ty);
  return t == T_SOLID || t == T_BRICK || t == T_QUESTION || t == T_USED_Q;
}
inline bool tilePlatform(int tx, int ty) {
  return tileAt(tx, ty) == T_PLATFORM;
}
inline bool tileHazard(int tx, int ty) {
  uint8_t t = tileAt(tx, ty);
  return t == T_SPIKE || t == T_LAVA;
}

// ─────────────────────────────────────────────────────
//  ЧАСТИЦЫ
// ─────────────────────────────────────────────────────
void spawnParticle(float x, float y, uint8_t type, int count) {
  int spawned = 0;
  for (int i = 0; i < MAX_PARTS && spawned < count; i++) {
    if (!gParticles[i].active) {
      gParticles[i].x = x;
      gParticles[i].y = y;
      gParticles[i].type = type;
      gParticles[i].life = 20 + (gFrame % 10);
      gParticles[i].maxLife = gParticles[i].life;
      gParticles[i].active = true;
      switch (type) {
        case PT_DUST:
          gParticles[i].vx = ((int)(gFrame % 5) - 2) * 0.3f;
          gParticles[i].vy = -0.6f - (gFrame % 3) * 0.2f;
          break;
        case PT_SPARKLE:
          gParticles[i].vx = ((int)(gFrame % 7) - 3) * 0.4f;
          gParticles[i].vy = ((int)(gFrame % 5) - 2) * 0.4f;
          break;
        case PT_DEBRIS:
          gParticles[i].vx = ((int)(gFrame % 9) - 4) * 0.6f;
          gParticles[i].vy = -1.5f - (gFrame % 4) * 0.3f;
          break;
        case PT_BLOOD:
          gParticles[i].vx = ((int)(gFrame % 7) - 3) * 0.5f;
          gParticles[i].vy = -1.2f - (gFrame % 3) * 0.2f;
          break;
      }
      spawned++;
    }
  }
}
void updateParticles() {
  for (int i = 0; i < MAX_PARTS; i++) {
    Particle& p = gParticles[i];
    if (!p.active) continue;
    p.x += p.vx;
    p.y += p.vy;
    p.vy += 0.18f;
    if (p.life > 0) p.life--;
    else p.active = false;
    if (p.y > 90) p.active = false;
  }
}

// ─────────────────────────────────────────────────────
//  ПРОЕКТИЛИ
// ─────────────────────────────────────────────────────
void spawnProj(float x, float y, float vx, float vy, bool fromPlayer) {
  for (int i = 0; i < MAX_PROJ; i++) {
    if (!gProj[i].active) {
      gProj[i] = { x, y, vx, vy, true, fromPlayer, 90 };
      return;
    }
  }
}
void updateProjectiles() {
  for (int i = 0; i < MAX_PROJ; i++) {
    Projectile& p = gProj[i];
    if (!p.active) continue;
    p.x += p.vx;
    p.y += p.vy;
    if (p.life > 0) p.life--; else p.active = false;

    int tx = (int)(p.x / TILE);
    int ty = (int)(p.y / TILE);
    if (tileSolid(tx, ty)) p.active = false;
    if (p.y > 100 || p.y < -20) p.active = false;

    // попадание в игрока
    if (!p.fromPlayer && !gPlayer.dead) {
      float pl = gPlayer.x, pr = gPlayer.x + 7;
      float pt = gPlayer.y, pb = gPlayer.y + 11;
      if (p.x > pl && p.x < pr && p.y > pt && p.y < pb) {
        p.active = false;
        if (!gPlayer.invuln && !gPlayer.star) {
          gPlayer.hp--;
          gPlayer.invuln = true;
          gPlayer.invulnUntil = millis() + INVULN_TIME;
          sHurt();
          spawnParticle(p.x, p.y, PT_BLOOD, 3);
          if (gPlayer.hp <= 0) {
            gPlayer.dying = true;
            gPlayer.deadAt = millis();
            sDie();
          }
        }
      }
    }
  }
}

// ─────────────────────────────────────────────────────
//  КАМЕРА
// ─────────────────────────────────────────────────────
void updateCamera() {
  gCamTargetX = gPlayer.x - 60;
  gCamTargetY = gPlayer.y - 30;

  float maxX = gMapW * TILE - 128.0f;
  float maxY = gMapH * TILE - 56.0f;
  if (maxX < 0) maxX = 0;
  if (maxY < 0) maxY = 0;

  gCamTargetX = fclamp(gCamTargetX, 0, maxX);
  gCamTargetY = fclamp(gCamTargetY, 0, maxY);

  gCamX += (gCamTargetX - gCamX) * CAM_LERP;
  gCamY += (gCamTargetY - gCamY) * CAM_Y_LERP;

  gCamX = fclamp(gCamX, 0, maxX);
  gCamY = fclamp(gCamY, 0, maxY);
}

// ─────────────────────────────────────────────────────
//  ФИЗИКА ИГРОКА
// ─────────────────────────────────────────────────────
void playerHurt(float fromX) {
  if (gPlayer.invuln || gPlayer.star || gPlayer.dead) return;
  gPlayer.hp--;
  gPlayer.invuln = true;
  gPlayer.invulnUntil = millis() + INVULN_TIME;
  gPlayer.vx = (gPlayer.x < fromX) ? -2.0f : 2.0f;
  gPlayer.vy = -2.5f;
  sHurt();
  spawnParticle(gPlayer.x + 3, gPlayer.y + 6, PT_BLOOD, 4);
  if (gPlayer.hp <= 0) {
    gPlayer.dying = true;
    gPlayer.deadAt = millis();
    sDie();
  }
}

void playerMoveAxis(float dx, float dy) {
  // X
  if (dx != 0) {
    gPlayer.x += dx;
    int tx0 = (int)(gPlayer.x / TILE);
    int tx1 = (int)((gPlayer.x + 7) / TILE);
    int ty0 = (int)(gPlayer.y / TILE);
    int ty1 = (int)((gPlayer.y + 11) / TILE);
    for (int ty = ty0; ty <= ty1; ty++) {
      for (int tx = tx0; tx <= tx1; tx++) {
        if (tileSolid(tx, ty)) {
          if (dx > 0) gPlayer.x = tx * TILE - 7.01f;
          else        gPlayer.x = (tx + 1) * TILE + 0.01f;
          gPlayer.vx = 0;
          return;
        }
      }
    }
  }

  // Y
  if (dy != 0) {
    gPlayer.y += dy;
    gPlayer.onGround = false;
    int tx0 = (int)(gPlayer.x / TILE);
    int tx1 = (int)((gPlayer.x + 7) / TILE);
    int ty0 = (int)(gPlayer.y / TILE);
    int ty1 = (int)((gPlayer.y + 11) / TILE);

    for (int ty = ty0; ty <= ty1; ty++) {
      for (int tx = tx0; tx <= tx1; tx++) {
        if (tileSolid(tx, ty)) {
          if (dy > 0) {
            gPlayer.y = ty * TILE - 12.01f;
            gPlayer.vy = 0;
            gPlayer.onGround = true;
          } else {
            gPlayer.y = (ty + 1) * TILE + 0.01f;
            gPlayer.vy = 0;
            uint8_t t = gMap[ty][tx];
            if (t == T_BRICK) {
              gMap[ty][tx] = T_EMPTY;
              gScore += 5;
              spawnParticle(tx * TILE + 4, ty * TILE + 4, PT_DEBRIS, 5);
              sBrick();
            } else if (t == T_QUESTION && !gQUsed[ty][tx]) {
              gQUsed[ty][tx] = true;
              gMap[ty][tx] = T_USED_Q;
              gScore += 15;
              if (gItemCount < MAX_ITEMS) {
                gItems[gItemCount++] = { (int16_t)(tx*TILE+4), (int16_t)(ty*TILE), I_COIN, false, 0, 0 };
              }
              sCoin();
            }
          }
          return;
        }
      }
    }

    // платформы сверху
    if (dy > 0) {
      int pty = (int)((gPlayer.y + 12) / TILE);
      for (int tx = tx0; tx <= tx1; tx++) {
        if (tilePlatform(tx, pty)) {
          if (gPlayer.y + 12 > pty * TILE + 2) {
            gPlayer.y = pty * TILE - 12.01f;
            gPlayer.vy = 0;
            gPlayer.onGround = true;
            return;
          }
        }
      }
    }
  }
}

void updatePlayer() {
  if (gPlayer.dead) return;
  uint32_t now = millis();

  // таймеры бонусов
  if (gPlayer.invuln && now >= gPlayer.invulnUntil) gPlayer.invuln = false;
  if (gPlayer.star   && now >= gPlayer.starUntil)   gPlayer.star = false;
  if (gPlayer.boosted && now >= gPlayer.boostUntil) gPlayer.boosted = false;

  // смерть: анимация падения
  if (gPlayer.dying) {
    gPlayer.vy += GRAVITY * 0.5f;
    gPlayer.y += gPlayer.vy;
    if (now - gPlayer.deadAt > 1200) {
      gPlayer.dead = true;
    }
    return;
  }

  // горизонталь
  float targetVx = 0;
  float baseSpeed = gPlayer.boosted ? RUN_SPEED * 1.35f : RUN_SPEED;
  if (cfg.difficulty==0) baseSpeed *= 1.06f;
  else if (cfg.difficulty==2) baseSpeed *= 0.94f;
  // Горизонтальное движение определяется удержанием LEFT/RIGHT; OK не замедляет героя.

  if (held(BI_LEFT))  { targetVx = -baseSpeed; gPlayer.facingRight = false; }
  if (held(BI_RIGHT)) { targetVx =  baseSpeed; gPlayer.facingRight = true; }

  if (targetVx != 0) {
    if (targetVx > gPlayer.vx) gPlayer.vx = fmin(gPlayer.vx + ACCEL, targetVx);
    else                       gPlayer.vx = fmax(gPlayer.vx - ACCEL, targetVx);
  } else {
    gPlayer.vx *= gPlayer.onGround ? FRICTION : AIR_FRICTION;
    if (abs(gPlayer.vx) < 0.05f) gPlayer.vx = 0;
  }

  // coyote time
  if (gPlayer.onGround) {
    gPlayer.lastGroundTime = now;
    gPlayer.coyoteFrames = 6;
  } else if (gPlayer.coyoteFrames > 0) {
    gPlayer.coyoteFrames--;
  }

  // jump buffer
  if (evDown[BI_UP] || evDown[BI_OK]) {
    gPlayer.jumpBufferTime = now;
  }

  // прыжок
  if (now - gPlayer.jumpBufferTime < JUMP_BUFFER && gPlayer.coyoteFrames > 0) {
    gPlayer.vy = JUMP_V * (cfg.difficulty==0 ? 1.12f : (cfg.difficulty==2 ? 0.94f : 1.0f));
    gPlayer.coyoteFrames = 0;
    gPlayer.jumpBufferTime = 0;
    gPlayer.onGround = false;
    sJump();
    spawnParticle(gPlayer.x + 3, gPlayer.y + 12, PT_DUST, 3);
  }

  // гравитация
  gPlayer.vy += GRAVITY;
  if (gPlayer.vy > MAX_FALL) gPlayer.vy = MAX_FALL;

  // движение с коллизиями
  playerMoveAxis(gPlayer.vx, 0);
  playerMoveAxis(0, gPlayer.vy);

  // Опасности проверяем по всей нижней части хитбокса, а не только по двум
  // центральным пикселям: касание шипов/лавы больше не проходит незаметно.
  int tx0 = (int)((gPlayer.x + 1) / TILE);
  int tx1 = (int)((gPlayer.x + 6) / TILE);
  int tyTop = (int)((gPlayer.y + 7) / TILE);
  int tyBottom = (int)((gPlayer.y + 11) / TILE);
  bool touchingHazard = false;
  for (int tyCheck = tyTop; tyCheck <= tyBottom; tyCheck++) {
    for (int tx = tx0; tx <= tx1; tx++) {
      if (tileHazard(tx, tyCheck)) touchingHazard = true;
    }
  }
  if (touchingHazard) {
    bool onSpike = false;
    for (int tyCheck = tyTop; tyCheck <= tyBottom; tyCheck++) {
      for (int tx = tx0; tx <= tx1; tx++) {
        if (tileAt(tx, tyCheck) == T_SPIKE) onSpike = true;
      }
    }
    if (onSpike && !gPlayer.star && !gPlayer.dying && !gPlayer.dead) {
      // Шипы смертельно опасны: нельзя просто отскочить от них и пройти дальше.
      gPlayer.hp = 0;
      gPlayer.dying = true;
      gPlayer.deadAt = now;
      sDie();
      spawnParticle(gPlayer.x + 4, gPlayer.y + 8, PT_DEBRIS, 5);
    } else if (touchingHazard) {
      playerHurt(gPlayer.x + 3);
    }
  }

  // Нижняя граница уровня — не проходной коридор. Если физика всё же
  // пропустила игрока через пол, сразу фиксируем падение и смерть.
  if (gPlayer.y > (gMapH * TILE) - 12) {
    gPlayer.hp = 0;
    gPlayer.dying = true;
    gPlayer.deadAt = now;
    sDie();
  }

  // чекпоинт
  for (int ty2 = (int)(gPlayer.y / TILE); ty2 <= (int)((gPlayer.y + 11) / TILE); ty2++) {
    for (int tx2 = (int)(gPlayer.x / TILE); tx2 <= (int)((gPlayer.x + 7) / TILE); tx2++) {
      if (tx2 < 0 || tx2 >= gMapW || ty2 < 0 || ty2 >= gMapH) continue;
      if (gMap[ty2][tx2] == T_CHECKPOINT) {
        if (!gHasCheckpoint || gCheckpointX != tx2 || gCheckpointY != ty2) {
          gCheckpointX = tx2;
          gCheckpointY = ty2;
          gHasCheckpoint = true;
          sCheckpoint();
          spawnParticle(tx2 * TILE + 4, ty2 * TILE, PT_SPARKLE, 5);
        }
      }
      if (gMap[ty2][tx2] == T_FLAG) {
        if (gState == PS_PLAY) {
          gState = PS_LEVEL_CLEAR;
          gStateTimer = millis();
          sLevelWin();
          gLevelsCleared++;
        }
      }
    }
  }

  // анимация
  gPlayer.animTimer += 33;
  if (gPlayer.onGround && abs(gPlayer.vx) > 0.2f) {
    if (gPlayer.animTimer >= 100) {
      gPlayer.animTimer = 0;
      gPlayer.animFrame = (gPlayer.animFrame + 1) % 2;
    }
  } else {
    gPlayer.animTimer = 0;
    gPlayer.animFrame = 0;
  }
}

// ─────────────────────────────────────────────────────
//  ВРАГИ
// ─────────────────────────────────────────────────────
void enemyUpdateGround(Enemy& e, float speed) {
  e.vx = e.dir * speed;
  e.x += e.vx;

  // патрульные границы
  if (e.x < e.patrolMin) { e.dir = 1;  e.x = e.patrolMin; }
  if (e.x > e.patrolMax) { e.dir = -1; e.x = e.patrolMax; }

  // край
  if (e.type == E_EDGE || e.type == E_FAST) {
    int ftx = (int)((e.dir > 0 ? e.x + 8 : e.x) / TILE);
    int fty = (int)((e.y + 8) / TILE);
    if (!tileSolid(ftx, fty) && !tilePlatform(ftx, fty)) e.dir = -e.dir;
  }

  // стена
  int wtx = (int)((e.dir > 0 ? e.x + 8 : e.x) / TILE);
  int wty = (int)((e.y + 4) / TILE);
  if (tileSolid(wtx, wty)) e.dir = -e.dir;

  // гравитация
  e.vy += GRAVITY;
  if (e.vy > MAX_FALL) e.vy = MAX_FALL;
  e.y += e.vy;
  e.onGround = false;

  int ty0 = (int)(e.y / TILE);
  int ty1 = (int)((e.y + 8) / TILE);
  int tx0 = (int)(e.x / TILE);
  int tx1 = (int)((e.x + 8) / TILE);
  for (int ty = ty0; ty <= ty1; ty++) {
    for (int tx = tx0; tx <= tx1; tx++) {
      if (tileSolid(tx, ty)) {
        if (e.vy > 0) { e.y = ty * TILE - 8.01f; e.vy = 0; e.onGround = true; }
        else if (e.vy < 0) { e.y = (ty + 1) * TILE + 0.01f; e.vy = 0; }
      }
    }
  }
}

void updateEnemies() {
  uint32_t now = millis();
  for (int i = 0; i < gEnemyCount; i++) {
    Enemy& e = gEnemies[i];
    if (!e.alive) continue;

    float spd = e.baseSpeed;
    if (e.type == E_FAST) spd = 1.4f;
    else if (e.type == E_HEAVY) spd = 0.55f;
    else if (e.type == E_GUARD) spd = 0.7f;
    else if (e.type == E_EDGE) spd = 0.8f;

    if (e.type == E_FLYER) {
      e.x += e.dir * spd;
      if (e.x < e.patrolMin) { e.dir = 1; e.x = e.patrolMin; }
      if (e.x > e.patrolMax) { e.dir = -1; e.x = e.patrolMax; }
      e.y = e.originY + sinf((now + i * 500) * 0.003f) * 12;
    } else if (e.type == E_TURRET) {
      if (now - e.shootTimer > 2000) {
        e.shootTimer = now;
        float dx = (gPlayer.x - e.x);
        float dy = (gPlayer.y - e.y);
        float len = sqrtf(dx*dx + dy*dy);
        if (len > 0 && len < 100) {
          spawnProj(e.x + 4, e.y + 4, dx/len*1.5f, dy/len*1.5f, false);
        }
      }
    } else {
      enemyUpdateGround(e, spd);
    }

    // столкновение с игроком
    if (!gPlayer.dead && !gPlayer.dying) {
      float pl = gPlayer.x, pr = gPlayer.x + 7;
      float pt = gPlayer.y, pb = gPlayer.y + 11;
      if (pr > e.x && pl < e.x + 8 && pb > e.y && pt < e.y + 8) {
        if (gPlayer.star) {
          e.alive = false;
          gScore += 50;
          spawnParticle(e.x + 4, e.y + 4, PT_SPARKLE, 6);
          sBrick();
        } else if (gPlayer.vy > 0 && pb < e.y + 6 && !gPlayer.onGround) {
          // stomp
          e.hp--;
          if (e.hp <= 0) {
            e.alive = false;
            gScore += 25;
            sStomp();
            spawnParticle(e.x + 4, e.y + 4, PT_DUST, 4);
          } else {
            e.flashUntil = now + 300;
          }
          gPlayer.vy = JUMP_V * 0.65f;
        } else {
          playerHurt(e.x + 4);
        }
      }
    }

    // анимация
    e.animTimer += 33;
    if (e.animTimer > 200) { e.animTimer = 0; e.animFrame ^= 1; }
  }
}

// ─────────────────────────────────────────────────────
//  БОСС
// ─────────────────────────────────────────────────────
void bossUpdate() {
  if (!gBoss.alive) return;
  uint32_t now = millis();

  gBoss.facingLeft = (gPlayer.x < gBoss.x);
  gBoss.animTimer += 33;
  if (gBoss.animTimer > 200) {
    gBoss.animTimer = 0;
    gBoss.animFrame = (gBoss.animFrame + 1) % 2;
  }

  // определить фазу по HP
  uint8_t newPhase;
  if (gBoss.hp > 12) newPhase = BP_PHASE1;
  else if (gBoss.hp > 6) newPhase = BP_PHASE2;
  else if (gBoss.hp > 0) newPhase = BP_PHASE3;
  else newPhase = BP_DYING;

  if (newPhase != gBoss.phase) {
    gBoss.phase = newPhase;
    gBoss.phaseTimer = now;
    spawnParticle(gBoss.x + 8, gBoss.y + 8, PT_SPARKLE, 8);
    sfxPlay(PQ_CHECKPOINT_SFX, sizeof(PQ_CHECKPOINT_SFX)/sizeof(PQ_CHECKPOINT_SFX[0]));
  }

  // движение
  if (gBoss.phase == BP_PHASE1) {
    gBoss.x = gBoss.homeX + sinf(now * 0.002f) * 20;
    if (now - gBoss.attackTimer > 1500) {
      gBoss.attackTimer = now;
      float dx = gPlayer.x - gBoss.x;
      float dy = gPlayer.y - gBoss.y;
      float len = sqrtf(dx*dx + dy*dy);
      if (len > 0) spawnProj(gBoss.x + 8, gBoss.y + 8, dx/len*1.8f, dy/len*1.8f, false);
    }
  } else if (gBoss.phase == BP_PHASE2) {
    gBoss.y = gBoss.homeY + sinf(now * 0.004f) * 15;
    if (now - gBoss.attackTimer > 900) {
      gBoss.attackTimer = now;
      for (int i = 0; i < 3; i++) {
        float ang = (i - 1) * 0.4f + atan2f(gPlayer.y - gBoss.y, gPlayer.x - gBoss.x);
        spawnProj(gBoss.x + 8, gBoss.y + 8, cosf(ang) * 2.0f, sinf(ang) * 2.0f, false);
      }
    }
  } else if (gBoss.phase == BP_PHASE3) {
    gBoss.x = gBoss.homeX + sinf(now * 0.003f) * 30;
    gBoss.y = gBoss.homeY + cosf(now * 0.003f) * 8;
    if (now - gBoss.attackTimer > 600) {
      gBoss.attackTimer = now;
      for (int i = 0; i < 4; i++) {
        float ang = (i * PI / 2) + now * 0.001f;
        spawnProj(gBoss.x + 8, gBoss.y + 8, cosf(ang) * 2.2f, sinf(ang) * 2.2f, false);
      }
    }
  } else if (gBoss.phase == BP_DYING) {
    if (gBoss.deathTimer == 0) gBoss.deathTimer = now;
    gBoss.y += 0.5f;
    if ((now / 100) % 2) spawnParticle(gBoss.x + 8, gBoss.y + 8, PT_SPARKLE, 2);
    if (now - gBoss.deathTimer > 2000) {
      gBoss.alive = false;
      gState = PS_VICTORY;
      gCompletions++;
      prefs.putInt("pqCompletions", gCompletions);
      sVictory();
    }
    return;
  }

  // столкновение с игроком
  if (!gPlayer.dead && !gPlayer.star) {
    float bx = gBoss.x + 8, by = gBoss.y + 8;
    float dx = (gPlayer.x + 3) - bx;
    float dy = (gPlayer.y + 6) - by;
    if (dx*dx + dy*dy < 100) playerHurt(bx);
  }

  // попадания снарядов игрока
  for (int i = 0; i < MAX_PROJ; i++) {
    Projectile& p = gProj[i];
    if (!p.active || !p.fromPlayer) continue;
    float dx = p.x - (gBoss.x + 8);
    float dy = p.y - (gBoss.y + 8);
    if (dx*dx + dy*dy < 60) {
      p.active = false;
      gBoss.hp--;
      gScore += 5;
      sBossHit();
      spawnParticle(p.x, p.y, PT_DEBRIS, 3);
    }
  }
}

// ─────────────────────────────────────────────────────
//  ПРЕДМЕТЫ
// ─────────────────────────────────────────────────────
void updateItems() {
  uint32_t now = millis();
  for (int i = 0; i < gItemCount; i++) {
    Item& it = gItems[i];
    if (it.collected) continue;
    it.bobTimer += 33;
    if (it.bobTimer > 250) { it.bobTimer = 0; it.bobFrame = (it.bobFrame + 1) % 4; }

    float pl = gPlayer.x, pr = gPlayer.x + 7;
    float pt = gPlayer.y, pb = gPlayer.y + 11;
    if (pr > it.x - 4 && pl < it.x + 4 && pb > it.y && pt < it.y + 8) {
      it.collected = true;
      switch (it.type) {
        case I_COIN:   gScore += 10; gCoins++; sCoin();   break;
        case I_GEM:    gScore += 50; gCoins += 5; sPower(); break;
        case I_STAR:   gPlayer.star = true; gPlayer.starUntil = now + STAR_TIME; gScore += 40; sPower(); break;
        case I_HEART:
          if (gPlayer.hp < gPlayer.maxHp) gPlayer.hp++;
          gScore += 20; sPower();
          break;
        case I_BOOST:  gPlayer.boosted = true; gPlayer.boostUntil = now + BOOST_TIME; gScore += 15; sPower(); break;
      }
      spawnParticle(it.x, it.y + 4, PT_SPARKLE, 3);
    }
  }
}

// ─────────────────────────────────────────────────────
//  ДВИЖУЩИЕСЯ ПЛАТФОРМЫ
// ─────────────────────────────────────────────────────
void updateMovingPlatforms() {
  for (int i = 0; i < gPlatCount; i++) {
    MovingPlatform& p = gPlats[i];
    p.offset += p.dir * p.speed;
    if (p.offset > p.maxOff) { p.offset = p.maxOff; p.dir = -1; }
    if (p.offset < p.minOff) { p.offset = p.minOff; p.dir = 1; }

    float px = p.tileX * TILE + (p.axis == 0 ? p.offset : 0);
    float py = p.tileY * TILE + (p.axis == 1 ? p.offset : 0);

    // если игрок стоит на ней — толкаем
    if (gPlayer.onGround && !gPlayer.dead) {
      float pl = gPlayer.x, pr = gPlayer.x + 7;
      float pb = gPlayer.y + 11;
      if (pb >= py - 2 && pb <= py + 4 && pr > px && pl < px + 16) {
        if (p.axis == 0) gPlayer.x += p.dir * p.speed;
        else             gPlayer.y = py - 12.01f;
      }
    }
  }
}

// ─────────────────────────────────────────────────────
//  ЗАГРУЗКА УРОВНЯ
// ─────────────────────────────────────────────────────
void resetPlayerForLevel(int spawnX, int spawnY) {
  gPlayer.x = spawnX * TILE + 2;
  gPlayer.y = spawnY * TILE;
  gPlayer.vx = 0;
  gPlayer.vy = 0;
  gPlayer.onGround = false;
  gPlayer.facingRight = true;
  gPlayer.invuln = false;
  gPlayer.star = false;
  gPlayer.boosted = false;
  gPlayer.dead = false;
  gPlayer.dying = false;
  gPlayer.animFrame = 0;
  gPlayer.animTimer = 0;
  gPlayer.coyoteFrames = 0;
  gPlayer.jumpBufferTime = 0;
  gPlayer.hp = gPlayer.maxHp;
}

void loadLevel(uint8_t idx) {
  idx = idx % LEVEL_COUNT;
  gLevel = idx;
  const LevelDef& def = LEVELS[idx];

  memset(gMap, T_EMPTY, sizeof(gMap));
  memset(gQUsed, 0, sizeof(gQUsed));
  gEnemyCount = 0;
  gItemCount = 0;
  gPlatCount = 0;
  gCoins = 0;
  gBoss.alive = false;
  gHasCheckpoint = false;

  int spawnX = 2, spawnY = 2;
  int w = min((int)def.w, MAX_W);
  int h = min((int)def.h, MAX_H);
  gMapW = w;
  gMapH = h;

  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      char ch = (char)pgm_read_byte(def.map + y * def.w + x);
      float px = x * TILE + 4;
      float py = y * TILE;
      switch (ch) {
        case '#': gMap[y][x] = T_SOLID; break;
        case '=': gMap[y][x] = T_PLATFORM; break;
        case 'B': gMap[y][x] = T_BRICK; break;
        case '?': gMap[y][x] = T_QUESTION; break;
        case '^': gMap[y][x] = T_SPIKE; break;
        case '~': gMap[y][x] = T_WATER; break;
        case 'P': gMap[y][x] = T_CHECKPOINT; break;
        case 'F': gMap[y][x] = T_FLAG; break;
        case 'M': gMap[y][x] = T_DECO_BRICK; break;
        case 'K': gMap[y][x] = T_DECO_SKULL; break;
        case 'H':
          gMap[y][x] = T_EMPTY;
          if (gPlatCount < MAX_PLATS) {
            MovingPlatform& p = gPlats[gPlatCount++];
            p.tileX = (int16_t)x; p.tileY = (int16_t)y;
            p.offset = 0; p.minOff = -20; p.maxOff = 20;
            p.dir = 1; p.axis = 0; p.speed = 0.6f;
          }
          break;
        case 'V':
          gMap[y][x] = T_EMPTY;
          if (gPlatCount < MAX_PLATS) {
            MovingPlatform& p = gPlats[gPlatCount++];
            p.tileX = (int16_t)x; p.tileY = (int16_t)y;
            p.offset = 0; p.minOff = -20; p.maxOff = 20;
            p.dir = 1; p.axis = 1; p.speed = 0.5f;
          }
          break;
        case 'C':
          if (gItemCount < MAX_ITEMS) {
            Item& it = gItems[gItemCount++];
            it.x = (int16_t)px; it.y = (int16_t)py;
            it.type = I_COIN; it.collected = false; it.bobFrame = 0; it.bobTimer = 0;
          }
          break;
        case '*':
          if (gItemCount < MAX_ITEMS) {
            Item& it = gItems[gItemCount++];
            it.x = (int16_t)px; it.y = (int16_t)py;
            it.type = I_STAR; it.collected = false; it.bobFrame = 0; it.bobTimer = 0;
          }
          break;
        case '+':
          if (gItemCount < MAX_ITEMS) {
            Item& it = gItems[gItemCount++];
            it.x = (int16_t)px; it.y = (int16_t)py;
            it.type = I_HEART; it.collected = false; it.bobFrame = 0; it.bobTimer = 0;
          }
          break;
        case '>':
          if (gItemCount < MAX_ITEMS) {
            Item& it = gItems[gItemCount++];
            it.x = (int16_t)px; it.y = (int16_t)py;
            it.type = I_BOOST; it.collected = false; it.bobFrame = 0; it.bobTimer = 0;
          }
          break;
        case '&':
          if (gItemCount < MAX_ITEMS) {
            Item& it = gItems[gItemCount++];
            it.x = (int16_t)px; it.y = (int16_t)py;
            it.type = I_GEM; it.collected = false; it.bobFrame = 0; it.bobTimer = 0;
          }
          break;
        case 'E': case 'D': case 'R': case 'G': case 'T': case 'f': case 't': {
          if (gEnemyCount < MAX_ENEMIES) {
            Enemy& en = gEnemies[gEnemyCount++];
            en.x = px; en.y = py; en.vx = 0; en.vy = 0;
            en.originX = (int16_t)px; en.originY = (int16_t)py;
            en.alive = true; en.onGround = false; en.dir = 1; en.hp = 1;
            en.flashUntil = 0; en.animTimer = 0; en.shootTimer = 0; en.animFrame = 0;
            if (ch == 'E') { en.type = E_PATROL; en.patrolMin = (int16_t)(px-24); en.patrolMax = (int16_t)(px+24); en.baseSpeed = 0.75f; }
            else if (ch == 'D') { en.type = E_EDGE; en.patrolMin = (int16_t)(px-40); en.patrolMax = (int16_t)(px+40); en.baseSpeed = 0.80f; }
            else if (ch == 'R') { en.type = E_FAST; en.patrolMin = (int16_t)(px-32); en.patrolMax = (int16_t)(px+32); en.baseSpeed = 1.40f; }
            else if (ch == 'G') { en.type = E_GUARD; en.patrolMin = (int16_t)(px-16); en.patrolMax = (int16_t)(px+16); en.baseSpeed = 0.65f; }
            else if (ch == 'T') { en.type = E_HEAVY; en.hp = 2; en.patrolMin = (int16_t)(px-20); en.patrolMax = (int16_t)(px+20); en.baseSpeed = 0.50f; }
            else if (ch == 'f') { en.type = E_FLYER; en.patrolMin = (int16_t)(px-30); en.patrolMax = (int16_t)(px+30); en.baseSpeed = 0.90f; }
            else if (ch == 't') { en.type = E_TURRET; en.patrolMin = (int16_t)px; en.patrolMax = (int16_t)px; en.baseSpeed = 0.0f; }
          }
          break;
        }
        case 'X': {
          gMap[y][x] = T_EMPTY;
          gBoss.x = px; gBoss.y = py; gBoss.vx = 0; gBoss.vy = 0;
          gBoss.homeX = (int16_t)px; gBoss.homeY = (int16_t)py;
          gBoss.hp = 18; gBoss.maxHp = 18; gBoss.phase = BP_DORMANT;
          gBoss.phaseTimer = 0; gBoss.attackTimer = 0; gBoss.animTimer = 0;
          gBoss.animFrame = 0; gBoss.facingLeft = false; gBoss.alive = true; gBoss.deathTimer = 0;
          break;
        }
        case 'S':
          // Player is 12 px tall while a tile is 8 px: spawn one tile above
          // the marker so the player does not begin intersecting the floor.
          spawnX = x; spawnY = max(0, y - 2); gMap[y][x] = T_EMPTY;
          break;
        default:
          gMap[y][x] = T_EMPTY;
          break;
      }
    }
  }

  // Keep the final row solid, but never overwrite the row above it:
  // the level maps place S/F/flags on that row. Overwriting it erased every
  // exit flag in levels 2-4, so the game could not advance.
  if (gMapH >= 1) {
    for (int x = 0; x < gMapW; x++) gMap[gMapH - 1][x] = T_SOLID;
  }

  resetPlayerForLevel(spawnX, spawnY);

  gCamX = fmax(0, fmin(gMapW * TILE - 128.0f, gPlayer.x - 42)); // start with more level visible ahead
  gCamY = fmax(0, fmin(gMapH * TILE - 56.0f, gPlayer.y - 30));
  gCamTargetX = gCamX;
  gCamTargetY = gCamY;

  gLevelStart = millis();
  gState = PS_INTRO;
  gStateTimer = millis();
}

// ─────────────────────────────────────────────────────
//  Рендер и игровой цикл Super Mario
// ─────────────────────────────────────────────────────
void drawTile(uint8_t tile, int x, int y) {
  if (x < -8 || x >= 128 || y < -8 || y >= 64) return;
  switch (tile) {
    case T_SOLID: oled.drawXBMP(x,y,8,8,sprSolid); break;
    case T_BRICK: oled.drawXBMP(x,y,8,8,sprBrick); break;
    case T_QUESTION: oled.drawXBMP(x,y,8,8,sprQuestion); break;
    case T_USED_Q: oled.drawXBMP(x,y,8,8,sprUsedQ); break;
    case T_SPIKE: {
      // Three sharp, outlined peaks with a checker-like hazard base are more
      // legible than the old filled blob on a 1-bit OLED.
      oled.drawHLine(x,y+7,8);
      oled.drawLine(x+0,y+6,x+2,y+1); oled.drawLine(x+2,y+1,x+4,y+6);
      oled.drawLine(x+2,y+6,x+4,y+2); oled.drawLine(x+4,y+2,x+6,y+6);
      oled.drawLine(x+4,y+6,x+6,y+1); oled.drawLine(x+6,y+1,x+7,y+6);
      oled.drawPixel(x+1,y+7); oled.drawPixel(x+3,y+7); oled.drawPixel(x+5,y+7);
      break;
    }
    case T_PLATFORM: oled.drawXBMP(x,y,8,8,sprPlat); break;
    case T_CHECKPOINT: oled.drawXBMP(x,y,8,8,sprCheckpoint); break;
    case T_FLAG: oled.drawXBMP(x,y,8,8,sprFlag); break;
    case T_DECO_BRICK: oled.drawFrame(x+1,y+1,6,6); break;
    case T_DECO_SKULL: oled.drawDisc(x+4,y+4,2); break;
    case T_WATER: oled.drawHLine(x,y+5,8); break;
    case T_LAVA: oled.drawXBMP(x,y,8,8,sprLava); break;
    default: break;
  }
}

// Custom high-contrast pixel characters. On a monochrome 128x64 display,
// silhouette, outline and separated facial/gear pixels matter more than detail.
void drawHeroPixelArt(int x, int y, bool facingRight, uint8_t pose) {
  // shadow/boots
  oled.drawBox(x+1,y+10,3,2); oled.drawBox(x+6,y+10,3,2);
  // torso and overalls
  oled.drawFrame(x+2,y+5,6,5); oled.drawBox(x+3,y+6,4,3);
  oled.drawPixel(x+3,y+5); oled.drawPixel(x+6,y+5);
  // arms
  oled.drawBox(x,y+6,2,3); oled.drawBox(x+8,y+6,2,3);
  // head, hat and brim
  oled.drawBox(x+2,y+2,6,4); oled.drawBox(x+1,y+1,7,2);
  oled.drawHLine(x,y+3,9);
  // face detail changes with direction
  int eyeX = facingRight ? x+6 : x+3;
  oled.drawPixel(eyeX,y+3); oled.drawHLine(x+3,y+5,3);
  // gloves / pose accents
  if (pose==1) { oled.drawPixel(x,y+5); oled.drawPixel(x+9,y+5); }
  else if (pose==2) { oled.drawPixel(x+1,y+8); oled.drawPixel(x+8,y+8); }
  // small highlight cut-outs keep hat, face and body from merging into a blob
  oled.setDrawColor(0); oled.drawPixel(x+4,y+2); oled.drawPixel(x+5,y+2); oled.setDrawColor(1);
}

void drawEnemyPixelArt(uint8_t type, int x, int y, uint8_t frame, int8_t dir) {
  switch (type) {
    case E_PATROL:
      oled.drawRBox(x+1,y+2,6,5,2); oled.drawHLine(x+2,y+1,4);
      oled.drawPixel(x+(dir>0?5:2),y+3); oled.drawPixel(x+(dir>0?4:3),y+4);
      oled.drawPixel(x+1,y+7); oled.drawPixel(x+6,y+7);
      if(frame&1) { oled.drawPixel(x+2,y+6); oled.drawPixel(x+5,y+6); }
      break;
    case E_EDGE:
      oled.drawFrame(x+1,y+1,6,6); oled.drawLine(x+1,y+1,x+4,y+4); oled.drawLine(x+6,y+1,x+3,y+4);
      oled.drawPixel(x+2,y+6); oled.drawPixel(x+5,y+6); break;
    case E_FAST:
      oled.drawTriangle(x,y+5,x+4,y,x+7,y+5); oled.drawHLine(x+1,y+6,6);
      oled.drawPixel(x+(dir>0?5:2),y+3); if(frame&1) oled.drawPixel(x+3,y+7); break;
    case E_GUARD:
      oled.drawBox(x+1,y+2,6,5); oled.drawFrame(x+2,y+1,4,3);
      oled.drawPixel(x+3,y+3); oled.drawPixel(x+5,y+3); oled.drawHLine(x+2,y+5,4); break;
    case E_HEAVY:
      oled.drawFrame(x,y+1,8,6); oled.drawBox(x+1,y+2,6,4); oled.drawHLine(x+1,y+5,6);
      oled.drawPixel(x+2,y+7); oled.drawPixel(x+5,y+7); oled.drawPixel(x+3,y+3); oled.drawPixel(x+4,y+3); break;
    case E_FLYER:
      oled.drawLine(x,y+2,x+2,y+4); oled.drawLine(x+7,y+2,x+5,y+4); oled.drawDisc(x+4,y+4,2);
      oled.drawPixel(x+3,y+4); oled.drawPixel(x+5,y+4); if(frame&1) oled.drawHLine(x+1,y+1,6); break;
    case E_TURRET:
      oled.drawFrame(x+1,y+3,6,4); oled.drawBox(x+2,y+4,4,2);
      oled.drawLine(x+4,y+3,x+(dir>0?7:0),y+1); oled.drawPixel(x+2,y+7); oled.drawPixel(x+5,y+7); break;
  }
}

void drawBossPixelArt(int x, int y, uint8_t frame) {
  oled.drawFrame(x+2,y+3,12,11); oled.drawBox(x+4,y+5,8,7);
  oled.drawTriangle(x+2,y+5,x+0,y+0,x+5,y+3);
  oled.drawTriangle(x+13,y+3,x+15,y+0,x+12,y+5);
  oled.drawBox(x+4,y+6,2,2); oled.drawBox(x+10,y+6,2,2);
  oled.drawHLine(x+5,y+10,6); oled.drawPixel(x+6,y+9); oled.drawPixel(x+9,y+9);
  if (frame&1) { oled.drawPixel(x+3,y+14); oled.drawPixel(x+12,y+14); }
}

void drawGame() {
  oled.clearBuffer();
  // Simple parallax stars, constrained to the visible play area.
  for (int i=0;i<12;i++) {
    int sx=(i*37-(int)(gCamX*0.25f))%128; if(sx<0) sx+=128;
    int sy=10+(i*17)%35;
    oled.drawPixel(sx,sy);
  }
  int tx0=max(0,(int)(gCamX/TILE));
  int ty0=max(0,(int)(gCamY/TILE));
  for(int sy=0;sy<8;sy++) for(int sx=0;sx<17;sx++) {
    int tx=tx0+sx, ty=ty0+sy;
    if(tx>=0 && tx<gMapW && ty>=0 && ty<gMapH) {
      int px=tx*TILE-(int)gCamX, py=ty*TILE-(int)gCamY;
      drawTile(gMap[ty][tx],px,py);
    }
  }
  for(int i=0;i<gPlatCount;i++) {
    MovingPlatform &p=gPlats[i];
    int px=p.tileX*TILE-(int)gCamX+(p.axis==0?(int)p.offset:0);
    int py=p.tileY*TILE-(int)gCamY+(p.axis==1?(int)p.offset:0);
    oled.drawBox(px,py,16,3); oled.drawHLine(px,py+3,16);
  }
  for(int i=0;i<gItemCount;i++) if(!gItems[i].collected) {
    Item &it=gItems[i]; int x=it.x-(int)gCamX, y=it.y-(int)gCamY;
    if(x<-8||x>128||y<-8||y>64) continue;
    const uint8_t *spr=sprCoin0;
    if(it.type==I_STAR) spr=sprStar; else if(it.type==I_HEART) spr=sprHeart;
    else if(it.type==I_BOOST) spr=sprBoost; else if(it.type==I_GEM) spr=sprGem;
    oled.drawXBMP(x,y,8,8,spr);
  }
  for(int i=0;i<gEnemyCount;i++) if(gEnemies[i].alive) {
    Enemy &e=gEnemies[i]; int x=(int)e.x-(int)gCamX, y=(int)e.y-(int)gCamY;
    if(x<-8||x>128||y<-8||y>64) continue;
    drawEnemyPixelArt(e.type,x,y,e.animFrame,e.dir);
  }
  for(int i=0;i<MAX_PROJ;i++) if(gProj[i].active) oled.drawDisc((int)gProj[i].x-(int)gCamX,(int)gProj[i].y-(int)gCamY,1);
  for(int i=0;i<MAX_PARTS;i++) if(gParticles[i].active) oled.drawPixel((int)gParticles[i].x-(int)gCamX,(int)gParticles[i].y-(int)gCamY);
  int px=(int)gPlayer.x-(int)gCamX, py=(int)gPlayer.y-(int)gCamY;
  if(!gPlayer.invuln || ((millis()/90)&1)) {
    const uint8_t *hero=sprHeroIdle;
    if(gPlayer.dying) hero=sprHeroDead; else if(gPlayer.invuln) hero=sprHeroHurt;
    else if(!gPlayer.onGround) hero=(gPlayer.vy<0)?sprHeroJump:sprHeroFall;
    else if(fabs(gPlayer.vx)>0.3f) hero=(gPlayer.animFrame&1)?sprHeroRun1:sprHeroRun0;
    (void)hero; // sprite pointers retained for state compatibility; render detailed pixel art
    uint8_t pose = gPlayer.dying ? 2 : (!gPlayer.onGround ? 1 : (gPlayer.animFrame&1));
    drawHeroPixelArt(px-1,py,gPlayer.facingRight,pose);
  }
  if(gBoss.alive) {
    int bx=(int)gBoss.x-(int)gCamX, by=(int)gBoss.y-(int)gCamY;
    if(bx>-16&&bx<128&&by>-16&&by<64) drawBossPixelArt(bx,by,gBoss.animFrame);
    oled.drawFrame(80,1,46,5); oled.drawBox(81,2,(45*max(0,(int)gBoss.hp))/max(1,(int)gBoss.maxHp),3);
  }
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  char hud[32]; snprintf(hud,sizeof(hud),"L%u H%u C%u S%d",(unsigned)(gLevel+1),(unsigned)gPlayer.hp,(unsigned)gCoins,gScore);
  oled.setDrawColor(0); oled.drawBox(0,0,78,9); oled.setDrawColor(1); oled.drawStr(1,7,hud);
  if(gState==PS_PAUSE) overlayBox("PAUSED","OK: continue / BACK: exit");
  else if(gState==PS_INTRO) overlayBox(LEVELS[gLevel].name,"OK: start; L/R move");
  else if(gState==PS_DEAD) overlayBox("OUCH!","OK: retry / BACK: exit");
  else if(gState==PS_LEVEL_CLEAR) overlayBox("LEVEL CLEAR!","OK: next level");
  else if(gState==PS_GAME_OVER) overlayBox("GAME OVER","OK: retry / BACK: exit");
  else if(gState==PS_VICTORY) overlayBox("YOU WIN!","OK: menu / BACK: exit");
  present();
}

void pqRun() {
  gScore=0; gCoins=0; gLevelsCleared=0; gFrame=0;
  gBest=prefs.getInt("pqbest",0); gCoinsBest=prefs.getInt("pqcoins",0);
  gUnlockedLevel=(uint8_t)constrain(prefs.getUChar("pqunlock",0),0,(int)LEVEL_COUNT-1);
  loadLevel(gUnlockedLevel);
  waitRelease();
  bool wasOk=false;
  while(true) {
    uint32_t t0=millis(); pollButtons();
    if(quitPressed()) break;
    bool ok=evDown[BI_OK];
    if(gState==PS_INTRO) { if(ok) {gState=PS_PLAY; gStateTimer=millis();} }
    else if(gState==PS_PLAY) {
      if(ok && !wasOk && gPlayer.onGround) { gPlayer.vy=JUMP_V; gPlayer.onGround=false; sJump(); }
      if(evDown[BI_LEFT]) gPlayer.facingRight=false;
      if(evDown[BI_RIGHT]) gPlayer.facingRight=true;
      if(evDown[BI_UP] && gPlayer.onGround) {gPlayer.vy=JUMP_V*(cfg.difficulty==0?1.12f:(cfg.difficulty==2?0.94f:1.0f));gPlayer.onGround=false;sJump();}
      if(evDown[BI_OK] && evLong[BI_OK]) gState=PS_PAUSE;
      updatePlayer(); updateEnemies(); updateItems(); updateMovingPlatforms(); updateProjectiles(); updateParticles(); bossUpdate(); updateCamera();
      if(gPlayer.y > gMapH*TILE+8) playerHurt(gPlayer.x);
      if(gState==PS_LEVEL_CLEAR) { gLevelsCleared++; }
      if(gScore>gBest) {gBest=gScore;prefs.putInt("pqbest",gBest);}
      if(gCoins>gCoinsBest) {gCoinsBest=gCoins;prefs.putInt("pqcoins",gCoinsBest);}
    } else if(gState==PS_PAUSE) { if(ok && !wasOk) {gState=PS_PLAY;waitRelease();} }
    else if(gState==PS_DEAD) { if(ok && !wasOk) { if(gPlayer.hp>0) loadLevel(gLevel); else gState=PS_GAME_OVER; waitRelease(); } }
    else if(gState==PS_LEVEL_CLEAR) { if(ok && !wasOk) { if(gLevel+1<LEVEL_COUNT) {prefs.putUChar("pqunlock",max((int)gUnlockedLevel,(int)gLevel+1));loadLevel(gLevel+1);} else {gState=PS_VICTORY;prefs.putInt("pqcomplete",prefs.getInt("pqcomplete",0)+1);} waitRelease(); } }
    else if(gState==PS_GAME_OVER || gState==PS_VICTORY) { if(ok && !wasOk) {gScore=0;gCoins=0;loadLevel(gUnlockedLevel);waitRelease();} }
    wasOk=ok; gFrame++; drawGame(); frameWait(t0);
  }
}

} // namespace pq

// ─────────────────────────────────────────────────────
//  ТОЧКА ВХОДА (вызывается из launchApp)
// ─────────────────────────────────────────────────────
void runPixelQuest() {
  waitRelease();
  audioStop();
  ledcWrite(BUZ_PIN, 0);
  playMusicTheme(MARIO_TECH, MARIO_OLD, MARIO_CHILL, MARIO_DARK, MARIO_OLD, 125);
  pq::pqRun();
  audioStop();
  waitRelease();
  playMusicTheme(TECH_MENU, OLD_MENU, CHILL_MENU, DARK_MENU, TECH_MENU, 150);
}

// ═══════════════════════════════════════════════════════════════════════════
//  КОНЕЦ SUPER MARIO
// ═══════════════════════════════════════════════════════════════════════════

// -------- PER-APP OPTIONS --------
// Open by holding physical button 6 for 2.5 s on an app in the menu or while
// playing it. Settings are stored per app; the standard buttons remain active.
void runAppOptions(int id) {
  if (id<0 || id>=APP_COUNT || id==APP_SETTINGS) { runSettings(); return; }
  bool wasInside=insideAppOptions; insideAppOptions=true;
  waitRelease();
  int sel=0;
  const int count=(id==APP_DINO) ? 4 : (id==APP_FLAPPY ? 3 : 2);
  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (evTap[BI_BACK] || evLong[BI_BACK]) break;
    if (nav(BI_UP)) sel=(sel+count-1)%count;
    if (nav(BI_DOWN)) sel=(sel+1)%count;
    bool changeLeft=evDown[BI_LEFT], changeRight=evDown[BI_RIGHT], activate=evDown[BI_OK];
    if (changeLeft || changeRight || activate) {
      int step=changeRight?1:-1;
      if (sel==0) {
        appDifficulty[id]=(uint8_t)((appDifficulty[id]+(step>0?1:2))%3);
        char key[8]; snprintf(key,sizeof(key),"gd%02d",id); prefs.putUChar(key,appDifficulty[id]);
      } else if (id==APP_DINO && sel==1) {
        dinoPterodactyl=!dinoPterodactyl; prefs.putBool("dpt",dinoPterodactyl);
      } else if (id==APP_DINO && sel==2) {
        dinoNightScore = constrain(dinoNightScore + (step>0?100:-100),300,1500);
        prefs.putUShort("dns",dinoNightScore);
      } else if (id==APP_FLAPPY && sel==1) {
        flappyGap=constrain(flappyGap+(step>0?2:-2),22,36); prefs.putUChar("fgap",flappyGap);
      } else if (sel==count-1 && activate) break;
      if (cfg.sfx) SFX(SF_CLICK);
      waitRelease();
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    String title=String(appName(id)); if(title.length()>15) title=title.substring(0,15);
    oled.drawStr(0,12,title.c_str()); oled.drawHLine(0,16,128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int row=0;row<count;row++) {
      int y=27+row*9; String label, value;
      if(row==0) { label="Difficulty"; uint8_t d=difficultyFor(id); value=d==0?"EASY":d==1?"NORMAL":"HARD"; }
      else if(id==APP_DINO && row==1) { label="Pterodactyl"; value=dinoPterodactyl?"ON":"OFF"; }
      else if(id==APP_DINO && row==2) { label="Night starts"; value=String(dinoNightScore); }
      else if(id==APP_FLAPPY && row==1) { label="Pipe gap"; value=String(flappyGap); }
      else { label="Close options"; value="OK"; }
      if(row==sel) { oled.drawBox(0,y-7,128,9); oled.setDrawColor(0); }
      oled.drawStr(3,y,label.c_str()); oled.drawStr(94,y,value.c_str());
      if(row==sel) oled.setDrawColor(1);
    }
    oled.drawStr(0,62,"UP/DN select L/R edit");
    present(); frameWait(t0);
  }
  waitRelease(); insideAppOptions=wasInside;
}

// Small on-device filename editor. UP/DOWN changes the current character,
// LEFT/RIGHT moves the cursor, short OK accepts a character, long OK saves,
// BACK removes the last character, long BACK cancels.
String fmEditName(const String& title, String initial) {
  static const char alphabet[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";
  String value = initial;
  if (value.length() > 24) value = value.substring(0, 24);
  int cursor = value.length();
  int charIndex = 1; // A
  bool save = false;
  waitRelease();
  while (true) {
    uint32_t t0 = millis(); pollButtons();
    if (evLong[BI_BACK]) { waitRelease(); return String(); }
    if (nav(BI_LEFT)) cursor = max(0, cursor - 1);
    if (nav(BI_RIGHT)) cursor = min((int)value.length(), cursor + 1);
    if (nav(BI_UP)) charIndex = (charIndex + (int)strlen(alphabet) - 1) % (int)strlen(alphabet);
    if (nav(BI_DOWN)) charIndex = (charIndex + 1) % (int)strlen(alphabet);
    if (evTap[BI_BACK] && value.length()) { value.remove(value.length()-1); cursor=min(cursor,(int)value.length()); }
    if (evDown[BI_OK]) {
      if (cursor < 24) { value = value.substring(0,cursor) + String(alphabet[charIndex]) + value.substring(cursor); cursor++; }
    }
    if (held(BI_OK) && heldMs(BI_OK) >= 1500 && !bst[BI_OK].longDone) {
      bst[BI_OK].longDone = true; save = true; waitRelease(); break;
    }
    oled.clearBuffer(); oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,8,title.c_str()); oled.drawHLine(0,11,128);
    String shown=value; if(shown.length()>20) shown=shown.substring(max(0,cursor-18), min((int)value.length(), max(0,cursor-18)+20));
    oled.drawStr(0,27,shown.c_str());
    String charLine = String("Char: [") + alphabet[charIndex] + "]";
    oled.drawStr(0,40,charLine.c_str());
    oled.drawStr(0,52,"UP/DN char  L/R pos");
    oled.drawStr(0,63,"OK add  hold OK save"); present(); serviceFileWeb(); frameWait(t0);
  }
  if (!save) return String();
  value.trim();
  if (!value.length() || value.indexOf('/')>=0 || value.indexOf('\\')>=0 || value.indexOf("..")>=0) return String();
  return value;
}

bool fmCopyPathRecursive(const String& from, const String& to) {
  File src=SD.open(from,FILE_READ); if(!src) return false;
  bool isDir=src.isDirectory();
  if(!isDir) { src.close(); return copyFilePath(from,to); }
  src.close();
  if(SD.exists(to) || !SD.mkdir(to)) return false;
  File dir=SD.open(from,FILE_READ); if(!dir) return false;
  File child=dir.openNextFile(); bool ok=true;
  while(child && ok) {
    String n=baseName(String(child.name())); bool childDir=child.isDirectory();
    String a=from=="/"?"/"+n:from+"/"+n; String b=to=="/"?"/"+n:to+"/"+n;
    child.close(); ok=fmCopyPathRecursive(a,b); child=dir.openNextFile();
  }
  if(child) child.close(); dir.close();
  if(!ok) removeTree(to);
  return ok;
}

bool fmSetBootLogoFromPath(const String& rawPath) {
  String p=cleanPath(rawPath); if(!sdReady || !p.length()) return false;
  File f=SD.open(p,FILE_READ); if(!f || f.isDirectory() || f.size()!=1024){if(f)f.close();return false;} f.close();
  bool ok=true;
  if(p!="/bootlogo.raw") { if(SD.exists("/bootlogo.raw")) SD.remove("/bootlogo.raw"); ok=copyFilePath(p,"/bootlogo.raw"); }
  if(ok){bootLogoFromSD=true;prefs.putBool("bootlogo_sd",true);}
  return ok;
}

void fmPreviewFileOnDevice(const String& filePath, const String& fileName) {
  String low=fileName; low.toLowerCase();
  if(low.endsWith(".raw")) {
    File f=SD.open(filePath,FILE_READ);
    if(!f || f.isDirectory() || f.size()!=1024){if(f)f.close();showScreen("Image preview","RAW 128x64 only","Expected exactly 1024 bytes","BACK: return");waitAnyKey();return;}
    static uint8_t bitmap[1024]; size_t got=f.read(bitmap,sizeof(bitmap)); f.close();
    if(got!=sizeof(bitmap)){showScreen("Image preview","Read failed","Check SD card","BACK: return");waitAnyKey();return;}
    suppressSystemOverlay=true;
    while(true){uint32_t t0=millis();pollButtons();oled.clearBuffer();oled.drawXBMP(0,0,128,64,bitmap);present();frameWait(t0);
      if(evTap[BI_BACK]||evLong[BI_BACK])break;
      if(evTap[BI_OK]){suppressSystemOverlay=false;bool ok=fmSetBootLogoFromPath(filePath);showScreen("Boot logo",ok?"Saved /bootlogo.raw":"Could not save",ok?"Restart to apply":"Need RAW 1024 bytes","BACK: return");waitAnyKey();break;}
    }
    suppressSystemOverlay=false;waitRelease(); return;
  }
  if(low.endsWith(".txt")||low.endsWith(".ino")||low.endsWith(".h")||low.endsWith(".cpp")||low.endsWith(".json")||low.endsWith(".cfg")) {
    File f=SD.open(filePath,FILE_READ);String preview;preview.reserve(800);if(f){while(f.available()&&preview.length()<800)preview+=(char)f.read();f.close();}
    int pos=0;while(true){uint32_t t0=millis();pollButtons();if(evTap[BI_BACK]||evLong[BI_BACK])break;oled.clearBuffer();oled.setFont(u8g2_font_5x8_t_cyrillic);String title=fileName;if(title.length()>20)title=title.substring(0,20);oled.drawStr(0,8,title.c_str());oled.drawHLine(0,11,128);int cur=pos;for(int r=0;r<6;r++){int e=preview.indexOf('\n',cur);if(e<0)e=preview.length();String ln=preview.substring(cur,e);if(ln.length()>21)ln=ln.substring(0,21);oled.drawStr(0,21+r*7,ln.c_str());cur=e<(int)preview.length()?e+1:e;}present();if(nav(BI_DOWN)||evTap[BI_OK])pos=cur>=(int)preview.length()?0:cur;if(nav(BI_UP))pos=max(0,pos-180);serviceFileWeb();frameWait(t0);}waitRelease();return;
  }
  showScreen("File selected",fileName,"Use phone for PNG/JPG","OLED needs RAW 128x64");waitAnyKey();
}

bool fmChooseDestination(String& destination) {
  String path="/"; int sel=0;
  while(true) {
    uint32_t t0=millis(); pollButtons();
    struct DEntry { String name,path; } items[48]; int n=0;
    File dir=SD.open(path,FILE_READ);
    if(dir && dir.isDirectory()) { File f=dir.openNextFile(); while(f && n<47) { if(f.isDirectory()) { String fp=String(f.name()); if(!fp.startsWith("/")) fp=(path=="/"?"/":""+path+"/")+fp; items[n++]={baseName(fp),fp}; } f.close(); f=dir.openNextFile(); } if(f)f.close(); dir.close(); }
    oled.clearBuffer(); oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(0,8,"DESTINATION FOLDER"); oled.drawHLine(0,11,128);
    String pt=path; if(pt.length()>20)pt="..."+pt.substring(pt.length()-17); oled.drawStr(0,20,pt.c_str());
    int total=n+2; sel=constrain(sel,0,total-1);
    for(int r=0;r<4;r++){ int ix=(sel/4)*4+r; if(ix>=total)break; int y=30+r*8; String nm=ix==0?"[ USE THIS FOLDER ]":(ix==1?".. parent":items[ix-2].name); if(ix==sel){oled.drawBox(0,y-6,128,8);oled.setDrawColor(0);} oled.drawStr(2,y,nm.c_str()); if(ix==sel)oled.setDrawColor(1); }
    oled.drawStr(0,63,"UP/DN select OK choose"); present(); serviceFileWeb(); frameWait(t0);
    if(evLong[BI_BACK]) { waitRelease(); return false; }
    if(nav(BI_UP))sel=max(0,sel-1); if(nav(BI_DOWN))sel=min(total-1,sel+1);
    if(evDown[BI_OK]) { if(sel==0){destination=path;waitRelease();return true;} if(sel==1){path=parentPath(path);sel=0;} else {path=items[sel-2].path;sel=0;} waitRelease(); }
  }
}

String fmFileType(const String& name, uint32_t size, bool isDir) {
  if (isDir) return "ПАПКА";
  String low=name; low.toLowerCase();
  if (low.endsWith(".bin")) return "BIN: образ прошивки ESP";
  if (low.endsWith(".raw") && size==1024) return "BOOT LOGO: RAW 128x64";
  if (low.endsWith(".raw")) return "RAW: размер не 1024 байта";
  if (low.endsWith(".png")) return "PNG: изображение";
  if (low.endsWith(".jpg") || low.endsWith(".jpeg")) return "JPEG: изображение";
  if (low.endsWith(".gif")) return "GIF: анимация/изображение";
  if (low.endsWith(".txt") || low.endsWith(".log")) return "ТЕКСТОВЫЙ ФАЙЛ";
  if (low.endsWith(".ino")) return "ARDUINO SKETCH";
  if (low.endsWith(".h") || low.endsWith(".hpp")) return "C/C++ HEADER";
  if (low.endsWith(".cpp") || low.endsWith(".c")) return "C/C++ SOURCE";
  if (low.endsWith(".json")) return "JSON: данные/настройки";
  if (low.endsWith(".cfg") || low.endsWith(".ini")) return "ФАЙЛ НАСТРОЕК";
  if (low.endsWith(".mp3") || low.endsWith(".wav")) return "АУДИОФАЙЛ";
  if (low.endsWith(".zip") || low.endsWith(".7z")) return "АРХИВ";
  return "НЕИЗВЕСТНЫЙ ТИП";
}

void fmContextMenu(const String& currentPath, bool isDir, const String& currentName, String& currentDir) {
  const char* labels[]={"Open / preview","Rename","Copy","Move","Delete","New folder","Set as boot logo","File info / BIN","Cancel"};
  int choice=0; const int total=9; bool rawLogo=false; String checkName=currentName; checkName.toLowerCase();
  File infoFile=SD.open(currentPath, FILE_READ); uint32_t infoSize=(infoFile && !infoFile.isDirectory())?(uint32_t)infoFile.size():0; if(infoFile)infoFile.close();
  String detectedType=fmFileType(currentName,infoSize,isDir);
  bool binFile=!isDir && checkName.endsWith(".bin");
  if (binFile) labels[7]="BIN info / flash guidance";
  if(!isDir && checkName.endsWith(".raw")){File z=SD.open(currentPath,FILE_READ);rawLogo=z&&!z.isDirectory()&&z.size()==1024;if(z)z.close();}
  waitRelease();
  while(true) {
    uint32_t t0=millis(); pollButtons();
    if(evLong[BI_BACK]) { waitRelease(); return; }
    if(nav(BI_UP))choice=(choice+total-1)%total;
    if(nav(BI_DOWN))choice=(choice+1)%total;
    oled.clearBuffer(); oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(0,8,"FILE ACTIONS"); oled.drawHLine(0,11,128);
    int first=(choice/5)*5;
    for(int r=0;r<5;r++){int ix=first+r;if(ix>=total)break;int y=22+r*9;if(ix==choice){oled.drawBox(0,y-7,128,9);oled.setDrawColor(0);}String label=labels[ix];if(ix==6&&!rawLogo)label="Boot logo (RAW 1024B only)";if(label.length()>24)label=label.substring(0,24);oled.drawStr(3,y,label.c_str());if(ix==choice)oled.setDrawColor(1);}
    // No footer line: it used to cover the final action on the 64px display.
    present(); serviceFileWeb(); frameWait(t0);
    if(!evTap[BI_OK]) continue;
    waitRelease();
    if(choice==8) return;
    if(choice==7) {
      String low=currentName; low.toLowerCase();
      if(low.endsWith(".bin")) {
        showScreen("BIN firmware", detectedType, String(infoSize/1024)+" KB", "Use SD test launcher");
        waitAnyKey();
      } else {
        showScreen("File information", detectedType, String(infoSize)+" bytes", "OK/BACK return");
        waitAnyKey();
      }
      return;
    }
    if(choice==0) {
      if(isDir) { currentDir=cleanPath(currentPath); return; }
      String low=currentName; low.toLowerCase();
      if(low.endsWith(".raw")) {
        File f=SD.open(currentPath,FILE_READ);
        if(f && !f.isDirectory() && f.size()==1024) {
          static uint8_t bitmap[1024]; size_t got=f.read(bitmap,sizeof(bitmap)); f.close();
          if(got==sizeof(bitmap)) {
            while(true){uint32_t pt=millis();pollButtons();oled.clearBuffer();oled.drawXBMP(0,0,128,64,bitmap);present();serviceFileWeb();frameWait(pt);
              if(evTap[BI_BACK]||evLong[BI_BACK])break;
              if(evTap[BI_OK]){bool ok=fmSetBootLogoFromPath(currentPath);showScreen("Boot logo",ok?"Saved as /bootlogo.raw":"Invalid RAW file",ok?"Restart to apply":"Need exactly 1024 bytes","BACK: return");waitAnyKey();break;}
            }
            waitRelease();
          } else showScreen("Image preview","Could not read RAW","Expected 128x64 / 1024B","BACK: return");
        } else {if(f)f.close();showScreen("Image preview","RAW format required","128x64 / 1024 bytes","BACK: return");waitAnyKey();}
      } else if(low.endsWith(".txt")||low.endsWith(".ino")||low.endsWith(".h")||low.endsWith(".cpp")||low.endsWith(".json")||low.endsWith(".cfg")) {
        File f=SD.open(currentPath,FILE_READ); String preview; preview.reserve(600); if(f){while(f.available()&&preview.length()<600)preview+=(char)f.read();f.close();} int pos=0;
        while(true){uint32_t pt=millis();pollButtons(); if(evTap[BI_BACK]||evLong[BI_BACK])break; oled.clearBuffer();oled.setFont(u8g2_font_5x8_t_cyrillic);String title=currentName;if(title.length()>20)title=title.substring(0,20);oled.drawStr(0,8,title.c_str());oled.drawHLine(0,11,128);int cur=pos;for(int r=0;r<6;r++){int e=preview.indexOf('\n',cur);if(e<0)e=preview.length();String ln=preview.substring(cur,e);if(ln.length()>21)ln=ln.substring(0,21);oled.drawStr(0,21+r*7,ln.c_str());cur=e<(int)preview.length()?e+1:e;}present();if(nav(BI_DOWN)||evTap[BI_OK])pos=cur>=(int)preview.length()?0:cur;if(nav(BI_UP))pos=max(0,pos-180);serviceFileWeb();frameWait(pt);} waitRelease();
      } else showScreen("Selected file",currentName,"Preview on phone panel","BACK: return");
      return;
    }
    if(choice==1) { String nn=fmEditName("RENAME: hold OK save",currentName); if(nn.length()){String to=currentDir=="/"?"/"+nn:currentDir+"/"+nn; if(!SD.exists(to)&&SD.rename(currentPath,to))showScreen("Rename","Completed",nn,"OK");else showScreen("Rename","Failed","Check name/path","BACK");waitAnyKey();} return; }
    if(choice==2||choice==3) { String dest; if(!fmChooseDestination(dest))return; String to=dest=="/"?"/"+currentName:dest+"/"+currentName; bool ok=false; if(!SD.exists(to)){ok=choice==2?fmCopyPathRecursive(currentPath,to):SD.rename(currentPath,to);if(!ok&&choice==3){ok=fmCopyPathRecursive(currentPath,to);if(ok&&!removeTree(currentPath)){removeTree(to);ok=false;}}} showScreen(choice==2?"Copy":"Move",ok?"Completed":"Failed",baseName(to),"OK");waitAnyKey();return; }
    if(choice==4) { bool yes=false; showScreen("Delete file?",currentName,"Hold OK to confirm","BACK cancel"); uint32_t confirmAt=millis(); while(true){pollButtons();serviceFileWeb();if(evLong[BI_BACK]||evTap[BI_BACK]){waitRelease();break;}if(held(BI_OK)&&heldMs(BI_OK)>=1200){bst[BI_OK].longDone=true;yes=true;waitRelease();break;}if(millis()-confirmAt>10000)break;} if(yes){bool ok=removeTree(currentPath);showScreen("Delete",ok?"Completed":"Failed",currentName,"OK");waitAnyKey();}return; }
    if(choice==5) { String nn=fmEditName("NEW FOLDER: hold OK",""); if(nn.length()){String to=currentDir=="/"?"/"+nn:currentDir+"/"+nn;bool ok=!SD.exists(to)&&SD.mkdir(to);showScreen("New folder",ok?"Created":"Failed",nn,"OK");waitAnyKey();}return; }
    if(choice==6) { bool ok=rawLogo&&fmSetBootLogoFromPath(currentPath); showScreen("Boot logo",ok?"Saved as /bootlogo.raw":"RAW image required","128x64 monochrome","Restart to apply");waitAnyKey();return; }
  }
}
void runFmSettings() {
  waitRelease();
  int sel = 0;
  const int N = 3;
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evTap[BI_BACK] || evLong[BI_BACK]) return;
    if (nav(BI_UP)) sel = (sel + N - 1) % N;
    if (nav(BI_DOWN)) sel = (sel + 1) % N;
    if (evDown[BI_OK]) {
      if (sel == 0) { cfg.showFileTypes = !cfg.showFileTypes; saveCfg(); }
      else if (sel == 1) { cfg.settingsWrap = !cfg.settingsWrap; saveCfg(); }
      else if (sel == 2) return;
      if (cfg.sfx) SFX(SF_CLICK);
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 12, "FM Settings");
    oled.drawHLine(0, 16, 128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    const char* names[] = {"Show file types", "Menu wrap", "Back"};
    for (int i=0; i<N; i++) {
      int y = 28 + i*10;
      if (i == sel) { oled.drawRBox(0, y-7, 128, 9, 2); oled.setDrawColor(0); }
      oled.drawStr(4, y, names[i]);
      if (i == 0) oled.drawStr(100, y, cfg.showFileTypes ? "ON" : "OFF");
      if (i == 1) oled.drawStr(100, y, cfg.settingsWrap ? "ON" : "OFF");
      oled.setDrawColor(1);
    }
    present(); frameWait(t0);
  }
}
void runLocalFileManager() {
  waitRelease();
  if (!sdReady) { showScreen("File Manager", "SD card not detected", "Check SPI wiring", "BACK: return"); waitAnyKey(); return; }
  struct FMEntry { String name; String path; bool isDir; uint32_t size; };
  static FMEntry entries[128]; static int entryCount=0;
  String path="/"; int selected=0; bool needRefresh=true; uint32_t seenSdGeneration=sdChangeGeneration;
  auto refreshEntries=[&]() { entryCount=0; File dir=SD.open(path,FILE_READ); if(!dir||!dir.isDirectory()){if(dir)dir.close();needRefresh=false;seenSdGeneration=sdChangeGeneration;return;} File f=dir.openNextFile(); while(f&&entryCount<128){String raw=String(f.name());bool d=f.isDirectory();String full=raw.startsWith("/")?raw:(path=="/"?"/"+raw:path+"/"+raw);entries[entryCount]={baseName(full),full,d,(uint32_t)f.size()};entryCount++;f.close();f=dir.openNextFile();}if(f)f.close();dir.close();for(int i=0;i<entryCount;i++)for(int j=i+1;j<entryCount;j++)if((entries[j].isDir&&!entries[i].isDir)||(entries[j].isDir==entries[i].isDir&&entries[j].name.compareTo(entries[i].name)<0)){FMEntry t=entries[i];entries[i]=entries[j];entries[j]=t;}selected=entryCount?constrain(selected,0,entryCount-1):0;needRefresh=false;seenSdGeneration=sdChangeGeneration;};
  uint32_t okDownAt=0; bool okWasDown=false; bool longActionFired=false;
  while(true){uint32_t t0=millis();serviceFileWeb();pollButtons();if(sdChangeGeneration!=seenSdGeneration)needRefresh=true;if(needRefresh)refreshEntries();
    if(evTap[BI_BACK]||evLong[BI_BACK]){if(path!="/"){path=parentPath(path);selected=0;needRefresh=true;}else break;}
    if(entryCount>0){if(nav(BI_UP))selected=(selected+entryCount-1)%entryCount;if(nav(BI_DOWN))selected=(selected+1)%entryCount;}else selected=0;
    bool okDown=held(BI_OK); if(okDown&&!okWasDown){okDownAt=millis();longActionFired=false;}
    if(okDown&&!longActionFired&&millis()-okDownAt>=1500){longActionFired=true;bst[BI_OK].longDone=true; if(entryCount>0&&selected<entryCount){FMEntry f=entries[selected];fmContextMenu(f.path,f.isDir,f.name,path);needRefresh=true;}else{showScreen("File Manager","Folder is empty","Create folder in phone panel","BACK: return");waitAnyKey();needRefresh=true;} waitRelease();}
    if(evTap[BI_OK]&&!longActionFired&&entryCount>0&&selected<entryCount){FMEntry f=entries[selected];if(f.isDir){path=cleanPath(f.path);selected=0;needRefresh=true;}else{fmPreviewFileOnDevice(f.path,f.name);needRefresh=true;waitRelease();}}
    okWasDown=held(BI_OK);
    oled.clearBuffer();oled.setFont(u8g2_font_5x8_t_cyrillic);oled.drawStr(0,8,"FILE MANAGER");oled.drawHLine(0,11,128);String pt=path;if(pt.length()>11)pt="..."+pt.substring(pt.length()-8);oled.drawStr(0,20,pt.c_str()); if(cfg.showFileTypes && entryCount>0){String typ=fmFileType(entries[selected].name,entries[selected].size,entries[selected].isDir);if(typ.length()>21)typ=typ.substring(0,21);oled.drawStr(0,63,typ.c_str());} int first=(selected/4)*4;for(int r=0;r<4;r++){int ix=first+r;if(ix>=entryCount)break;int y=29+r*9;if(ix==selected){oled.drawRBox(0,y-7,128,9,2);oled.setDrawColor(0);}String nm=String(entries[ix].isDir?"> ":"  ")+entries[ix].name;if(nm.length()>21)nm=nm.substring(0,21);oled.drawStr(3,y,nm.c_str());if(ix==selected)oled.setDrawColor(1);}{String pos=String(entryCount)+" files";oled.drawStr(74,20,pos.c_str());}present();frameWait(t0);
  }
  waitRelease();
}
void runSdFirmwareUpdate() {
  struct BinFile { String name, fullPath; uint32_t size; };
  static BinFile bins[16];
  int count = 0;
  int sel = 0;

  waitRelease();
  if (!sdReady) {
    showScreen("OTA Update", "SD card missing", "", "BACK: exit");
    waitAnyKey();
    return;
  }

  // Во время выбора файла веб-панель и кнопки должны продолжать работать.
  otaInProgress = false;

  // Сканируем SD один раз, а не на каждом кадре.
  File dir = SD.open("/", FILE_READ);
  if (dir && dir.isDirectory()) {
    File f = dir.openNextFile();
    int scanned = 0;
    while (f && count < 16 && scanned < 100) {
      scanned++;
      String full = String(f.name());
      if (!full.startsWith("/")) full = "/" + full;
      String low = full;
      low.toLowerCase();
      if (!f.isDirectory() && low.endsWith(".bin")) {
        bins[count].name = baseName(full);
        bins[count].fullPath = full;
        bins[count].size = (uint32_t)f.size();
        count++;
      }
      f.close();
      f = dir.openNextFile();
      delay(1);
    }
    if (f) f.close();
    dir.close();
  } else if (dir) {
    dir.close();
  }

  while (true) {
    uint32_t t0 = millis();
    pollButtons();

    if (evTap[BI_BACK] || evLong[BI_BACK]) {
      waitRelease();
      return;
    }

    if (count > 0) {
      if (nav(BI_UP)) sel = (sel + count - 1) % count;
      if (nav(BI_DOWN)) sel = (sel + 1) % count;
    }

    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 12, "SD Firmware Update");
    oled.drawHLine(0, 16, 128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);

    if (count == 0) {
      oled.drawStr(4, 34, "No .bin files in root /");
      oled.drawStr(4, 48, "BACK: exit");
    } else {
      int first = (sel / 4) * 4;
      for (int row = 0; row < 4; row++) {
        int i = first + row;
        if (i >= count) break;
        int y = 26 + row * 9;
        if (i == sel) {
          oled.drawRBox(0, y - 7, 128, 9, 2);
          oled.setDrawColor(0);
        }
        String nm = bins[i].name;
        if (nm.length() > 16) nm = nm.substring(0, 16);
        oled.drawStr(3, y, nm.c_str());
        String sz = String(bins[i].size / 1024) + "K";
        oled.drawStr(105, y, sz.c_str());
        oled.setDrawColor(1);
      }
      oled.drawStr(3, 63, "OK: flash   BACK: exit");
    }
    present();
    frameWait(t0);

    if (count == 0 || !evDown[BI_OK]) continue;

    File f = SD.open(bins[sel].fullPath, FILE_READ);
    if (!f || f.isDirectory() || f.size() == 0) {
      if (f) f.close();
      showScreen("OTA Error", "File empty or missing", "", "OK/BACK: return");
      waitAnyKey();
      continue;
    }

    const uint32_t fileSize = (uint32_t)f.size();
    audioStop();
    otaInProgress = true;

    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(10, 15, "Preparing...");
    oled.sendBuffer();

    // Wi-Fi нужно выключать до Update.begin(), а не после ошибки.
    WiFi.mode(WIFI_OFF);
    delay(100);

    bool started = Update.begin(fileSize, U_FLASH);
    bool success = started;
    uint32_t written = 0;
    uint32_t lastDraw = 0;
    String otaError = started ? "" : String(Update.errorString());

    if (started) {
      uint8_t buf[1024];
      while (written < fileSize) {
        uint32_t remaining = fileSize - written;
        size_t toRead = remaining < sizeof(buf) ? (size_t)remaining : sizeof(buf);
        int n = f.read(buf, toRead);
        if (n <= 0) {
          success = false;
          otaError = "SD read error";
          break;
        }
        if (Update.write(buf, n) != (size_t)n) {
          success = false;
          otaError = String(Update.errorString());
          break;
        }
        written += n;

        if (millis() - lastDraw >= 200) {
          lastDraw = millis();
          int pct = (int)(((uint64_t)written * 100UL) / fileSize);
          oled.clearBuffer();
          oled.setFont(u8g2_font_6x13_t_cyrillic);
          oled.drawStr(5, 15, "Flashing...");
          oled.setFont(u8g2_font_5x8_t_cyrillic);
          oled.drawStr(5, 30, (String(written / 1024) + " KB").c_str());
          oled.drawFrame(5, 40, 118, 8);
          oled.drawBox(7, 42, (pct * 114) / 100, 4);
          oled.drawStr(5, 60, "DO NOT POWER OFF");
          oled.sendBuffer();
        }
        yield();
      }
    }
    f.close();

    if (success && written == fileSize && !Update.hasError()) {
      success = Update.end(true);
      if (!success) otaError = String(Update.errorString());
    } else if (success) {
      success = false;
      otaError = "Incomplete firmware write";
    }

    if (success) {
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x13_t_cyrillic);
      oled.drawStr(15, 25, "SUCCESS!");
      oled.drawStr(15, 40, "Rebooting...");
      oled.sendBuffer();
      delay(1500);
      ESP.restart();
    }

    if (started) Update.abort();
    if (!otaError.length()) otaError = String(Update.errorString());
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(10, 15, "OTA FAILED");
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    String shortError = otaError;
    if (shortError.length() > 24) shortError = shortError.substring(0, 24);
    oled.drawStr(5, 32, shortError.c_str());
    oled.drawStr(5, 45, ("Written: " + String(written) + " B").c_str());
    oled.drawStr(5, 60, "OK/BACK: return");
    oled.sendBuffer();
    waitAnyKey();

    // После неудачной попытки возвращаем панель и файловый список.
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    otaInProgress = false;
    waitRelease();
  }
}

// =====================================================
//  NOTES APP
// =====================================================
#define NOTES_MAX 32
String notesList[NOTES_MAX];
int notesCount = 0;
int notesSel = 0;

void refreshNotesList() {
  notesCount = 0;
  if (!sdReady) return;
  if (!SD.exists("/notes")) SD.mkdir("/notes");
  File dir = SD.open("/notes");
  if (!dir) return;
  File f = dir.openNextFile();
  while (f && notesCount < NOTES_MAX) {
    if (!f.isDirectory()) notesList[notesCount++] = String(f.name());
    f.close();
    f = dir.openNextFile();
  }
  if (f) f.close();
  dir.close();
  for (int i=0; i<notesCount; i++)
    for (int j=i+1; j<notesCount; j++)
      if (notesList[j] < notesList[i]) {
        String t = notesList[i]; notesList[i] = notesList[j]; notesList[j] = t;
      }
  if (notesSel >= notesCount) notesSel = max(0, notesCount-1);
}

String readNote(const String& name) {
  String path = "/notes/" + name;
  File f = SD.open(path, FILE_READ);
  if (!f) return "";
  String s;
  while (f.available()) s += (char)f.read();
  f.close();
  return s;
}

bool saveNote(const String& name, const String& content) {
  String path = "/notes/" + name;
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;
  f.print(content);
  f.close();
  return true;
}

String createNote() {
  for (int i=1; i<=999; i++) {
    char name[16];
    snprintf(name, sizeof(name), "note_%03d.txt", i);
    String path = "/notes/" + String(name);
    if (!SD.exists(path)) {
      File f = SD.open(path, FILE_WRITE);
      if (f) { f.close(); return String(name); }
    }
  }
  return "";
}

void runNotesEditor(const String& name) {
  String text = readNote(name);
  text.replace('\n', '|'); // Для однострочной клавиатуры
  runKeyboard("Edit note", text, false); // Изменяет text
  text.replace('|', '\n'); // Возвращаем переносы
  if (!saveNote(name, text)) {
    showScreen("Error", "Cannot save note", "", "BACK: exit");
    waitAnyKey();
  }
}
void runNotesActions(const String& name) {
  // Ждём, пока пользователь отпустит OK — БЕЗ таймаута.
  while (bst[BI_OK].stable) {
    pollButtons();
    delay(10);
  }
  // Небольшая пауза, чтобы отпускание не успело сгенерировать evTap
  delay(100);
  pollButtons();
  // Полная очистка всех событий на всякий случай
  for (int i = 0; i < BI_COUNT; i++) {
    evDown[i] = evTap[i] = evRep[i] = evLong[i] = false;
    bst[i].longDone = false;
  }

  const char* actions[] = {"Open", "New note", "To txt", "Format", "Gemini", "Delete", "Rename", "Back"};
  const int N = 8;
  int sel = 0;

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evTap[BI_BACK] || evLong[BI_BACK]) return;
    if (nav(BI_UP)) sel = (sel + N - 1) % N;
    if (nav(BI_DOWN)) sel = (sel + 1) % N;
    if (evDown[BI_OK]) {
      if (sel == 0) { runNotesEditor(name); return; }
      else if (sel == 1) {
        String newName = createNote();
        if (newName.length()) { runNotesEditor(newName); }
        return;
      }
      else if (sel == 2) {
        String text = readNote(name);
        String out = name; out.replace(".txt", "_export.txt");
        File f = SD.open("/notes/" + out, FILE_WRITE);
        if (f) { f.print(text); f.close(); showScreen("Export", "Saved", out.c_str()); }
        else showScreen("Error", "Write failed", "");
        waitAnyKey();
      }
      else if (sel == 3) {
        String text = readNote(name);
        while (text.indexOf("  ") >= 0) text.replace("  ", " ");
        text.trim();
        saveNote(name, text);
        showScreen("Format", "Done", "");
        waitAnyKey();
      }
      else if (sel == 4) {
        String text = readNote(name);
        showScreen("Gemini", "Sending...", "");
        String answer = askAI("Analyze this note:\n\n" + text);
        showAnswer(answer);
        return;
      }
      else if (sel == 5) {
        if (confirmBox("Delete", name.c_str(), "Are you sure?")) {
          SD.remove("/notes/" + name);
          return;
        }
      }
      else if (sel == 6) {
        String newName = name;
        if (runKeyboard("New name", newName, false) && newName.length() && newName != name) {
          if (!SD.exists("/notes/" + newName)) SD.rename("/notes/" + name, "/notes/" + newName);
          else { showScreen("Error", "File exists", ""); waitAnyKey(); }
        }
        return;
      }
      else if (sel == 7) return;
    }
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawUTF8(0, 12, "Actions");
    oled.drawHLine(0, 16, 128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i = 0; i < N; i++) {
      int y = 26 + i * 6;
      if (i == sel) { oled.drawRBox(0, y-6, 128, 8, 2); oled.setDrawColor(0); }
      oled.drawStr(3, y, actions[i]);
      if (i == sel) oled.setDrawColor(1);
    }
    present();
    frameWait(t0);
  }
}

void runNotes() {
  waitRelease();
  if (!sdReady) { showScreen("Notes", "SD not found", "", "BACK: exit"); waitAnyKey(); return; }
  refreshNotesList();

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (evTap[BI_BACK] || evLong[BI_BACK]) break;

    if (notesCount > 0) {
      if (nav(BI_UP)) notesSel = (notesSel + notesCount - 1) % notesCount;
      if (nav(BI_DOWN)) notesSel = (notesSel + 1) % notesCount;
    }

    // === ДОЛГОЕ НАЖАТИЕ OK (1.5 с) — меню действий ===
    if (held(BI_OK) && heldMs(BI_OK) >= 1500 && !bst[BI_OK].longDone) {
      bst[BI_OK].longDone = true;
      // Ждём отпускания без таймаута
      while (bst[BI_OK].stable) { pollButtons(); delay(10); }
      delay(80);
      pollButtons();
      evTap[BI_OK] = false; evDown[BI_OK] = false;

      if (notesCount > 0) {
        runNotesActions(notesList[notesSel]);
        refreshNotesList();
      }
      bst[BI_OK].longDone = false;
      continue;
    }

    // === КОРОТКОЕ НАЖАТИЕ OK (отпускание) — открыть/создать ===
    if (evTap[BI_OK]) {
      if (notesCount == 0) {
        String newName = createNote();
        if (newName.length()) {
          refreshNotesList();
          for (int i = 0; i < notesCount; i++) if (notesList[i] == newName) { notesSel = i; break; }
          runNotesEditor(newName);
          refreshNotesList();
        } else {
          showScreen("Error", "Cannot create", "check SD"); waitAnyKey();
        }
      } else {
        runNotesEditor(notesList[notesSel]);
        refreshNotesList();
      }
      bst[BI_OK].longDone = false;
    }

    // === ОТРИСОВКА ===
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawUTF8(0, 12, "Notes");
    oled.drawHLine(0, 16, 128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);

    if (notesCount == 0) {
      oled.drawStr(10, 36, "No notes");
      oled.drawStr(10, 48, "OK: create");
    } else {
      int start = max(0, notesSel - 1);
      for (int i = 0; i < 4 && start + i < notesCount; i++) {
        int idx = start + i;
        int y = 28 + i * 9;
        if (idx == notesSel) { oled.drawRBox(0, y-7, 128, 9, 2); oled.setDrawColor(0); }
        String nm = notesList[idx];
        if (nm.length() > 18) nm = nm.substring(0, 18);
        oled.drawStr(3, y, nm.c_str());
        if (idx == notesSel) oled.setDrawColor(1);
      }
    }
    oled.drawStr(0, 63, "OK: open  hold OK: actions");
    present();
    frameWait(t0);
  }
  waitRelease();
}
void runStore() {
  if (WiFi.status() != WL_CONNECTED) {
    showScreen("Store", "Wi-Fi not connected", "Check Settings");
    waitAnyKey();
    return;
  }

  String storeUrl = prefs.getString("store_url", "");
  if (!storeUrl.length()) {
    storeUrl = "http://192.168.1.100:8000";
    if (!runKeyboard("Store URL", storeUrl, false)) return;
    prefs.putString("store_url", storeUrl);
  }

  String channel = prefs.getString("store_ch", "stable");

  showScreen("Store", "Checking...", "v" + String(FIRMWARE_VERSION));

  String url = storeUrl + "/api/check?channel=" + channel
             + "&current_version=" + FIRMWARE_VERSION;

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) {
    showScreen("Store", "Connection failed", url);
    waitAnyKey();
    return;
  }
  int code = http.GET();
  if (code != 200) {
    showScreen("Store", "HTTP " + String(code), "");
    waitAnyKey();
    http.end();
    return;
  }
  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) {
    showScreen("Store", "Bad JSON", "");
    waitAnyKey();
    return;
  }

  bool hasUpdate = doc["update"] | false;
  if (!hasUpdate) {
    String latest = doc["latest"].as<String>();
    showScreen("Store", "Up to date", latest.length() ? "v" + latest : "");
    waitAnyKey();
    return;
  }

  String newVer = doc["version"].as<String>();
  String title  = doc["title"].as<String>();
  String dlPath = doc["download_url"].as<String>();
  int size      = doc["size"] | 0;

  // Экран подтверждения
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x13_t_cyrillic);
  oled.drawStr(0, 12, "Update available");
  oled.drawHLine(0, 16, 128);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  oled.drawUTF8(4, 28, ("v" + newVer + " / " + channel).c_str());
  if (title.length()) {
    String t = title; if (t.length() > 20) t = t.substring(0, 20);
    oled.drawUTF8(4, 40, t.c_str());
  }
  oled.drawStr(4, 52, (String(size/1024) + " KB").c_str());
  oled.drawStr(0, 63, "OK: install   BACK: no");
  present();

  waitRelease();
  bool doIt = false;
  while (true) {
    pollButtons();
    if (evDown[BI_OK]) { doIt = true; break; }
    if (evTap[BI_BACK] || evLong[BI_BACK]) break;
    present();
    delay(15);
  }
  if (!doIt) return;

  // Скачивание + прошивка
  otaInProgress = true;

  String fullUrl = storeUrl + dlPath;
  WiFiClient dlClient;
  HTTPClient dlHttp;
  dlHttp.setTimeout(15000);
  if (!dlHttp.begin(dlClient, fullUrl)) {
    showScreen("Store", "DL failed", ""); waitAnyKey();
    otaInProgress = false;
    return;
  }
  int dlCode = dlHttp.GET();
  if (dlCode != 200) {
    showScreen("Store", "DL HTTP " + String(dlCode), "");
    waitAnyKey();
    dlHttp.end();
    otaInProgress = false;
    return;
  }

  int len = dlHttp.getSize();
  if (len <= 0) {
    showScreen("Store", "Size unknown", "Cannot OTA");
    waitAnyKey();
    dlHttp.end();
    otaInProgress = false;
    return;
  }

  if (!Update.begin(len)) {
    showScreen("Store", "OTA begin", Update.errorString());
    waitAnyKey();
    dlHttp.end();
    otaInProgress = false;
    return;
  }

  WiFiClient* stream = dlHttp.getStreamPtr();
  uint8_t buf[512];
  int written = 0;
  uint32_t lastDraw = 0;
  uint32_t lastData = millis();

  while (dlHttp.connected() && written < len) {
    size_t sz = stream->readBytes(buf, min((int)sizeof(buf), len - written));
    if (sz == 0) {
      if (millis() - lastData > 10000) break;
      delay(10);
      continue;
    }
    lastData = millis();
    if (Update.write(buf, sz) != sz) break;
    written += sz;

    if (millis() - lastDraw > 200) {
      lastDraw = millis();
      oled.clearBuffer();
      oled.setFont(u8g2_font_6x13_t_cyrillic);
      oled.drawStr(10, 15, "Installing...");
      oled.setFont(u8g2_font_5x8_t_cyrillic);
      oled.drawStr(10, 30,
        (String(written/1024) + " / " + String(len/1024) + " KB").c_str());
      oled.drawFrame(10, 42, 108, 6);
      int pct = (written * 100) / len;
      oled.drawBox(12, 44, pct * 104 / 100, 2);
      present();
    }
  }
  dlHttp.end();

  if (written == len && Update.end(true) && !Update.hasError()) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(20, 25, "SUCCESS!");
    oled.drawStr(15, 42, "Rebooting...");
    present();
    delay(2000);
    ESP.restart();
  } else {
    Update.abort();
    otaInProgress = false;
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(10, 20, "Update failed");
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(10, 40, Update.errorString());
    present();
    waitAnyKey();
  }
}
void runDynSnake(JsonDocument& doc) {
  waitRelease();
  int speedMs = doc["params"]["speed_ms"] | 150;
  bool wrap = doc["params"]["wrap"] | false;
  String mapStr = doc["params"]["map"] | "";
  mapStr.replace("\n", "");
  
  const int W = 16, H = 8;
  uint8_t map[H][W] = {};
  if (mapStr.length() >= W * H) {
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++)
      map[y][x] = (mapStr[y*W + x] == '#') ? 1 : 0;
  }
  
  int sx=4, sy=4, dir=0;
  int dx[4]={1,0,-1,0}, dy[4]={0,1,0,-1};
  int snX[128], snY[128], len=4;
  for (int i=0;i<len;i++) { snX[i]=sx-i; snY[i]=sy; }
  int foodX=10, foodY=4, score=0;
  for (int t=0;t<200;t++) { int fx=random(W),fy=random(H); if(!map[fy][fx]) { foodX=fx; foodY=fy; break; } }
  uint32_t last=millis();
  playMusicTheme(TECH_SNAKE,OLD_SNAKE,CHILL_SNAKE,DARK_SNAKE,TECH_SNAKE,120);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;
    if (evDown[BI_LEFT] && dir!=0) dir=2;
    if (evDown[BI_RIGHT] && dir!=2) dir=0;
    if (evDown[BI_UP] && dir!=1) dir=3;
    if (evDown[BI_DOWN] && dir!=3) dir=1;
    
    if (millis()-last >= (uint32_t)speedMs) {
      last=millis();
      int nx=snX[0]+dx[dir], ny=snY[0]+dy[dir];
      if (wrap) { nx=(nx+W)%W; ny=(ny+H)%H; }
      else if (nx<0||nx>=W||ny<0||ny>=H) { SFX(SF_DIE); overlayBox("WALL","Crash"); waitRelease(); return; }
      if (map[ny][nx]) { SFX(SF_DIE); overlayBox("WALL","Crash"); waitRelease(); return; }
      for (int i=0;i<len;i++) if (snX[i]==nx&&snY[i]==ny) { SFX(SF_DIE); overlayBox("CRASH","Self-bite"); waitRelease(); return; }
      
      for (int i=len;i>0;i--) { snX[i]=snX[i-1]; snY[i]=snY[i-1]; }
      snX[0]=nx; snY[0]=ny;
      
      if (nx==foodX && ny==foodY) {
        len++; score+=10; SFX(SF_EAT);
        for (int t=0;t<200;t++) {
          int fx=random(W),fy=random(H);
          if (map[fy][fx]) continue;
          bool on=false;
          for(int i=0;i<len;i++) if(snX[i]==fx&&snY[i]==fy){on=true;break;}
          if(!on){foodX=fx;foodY=fy;break;}
        }
      }
    }
    
    oled.clearBuffer();
    oled.drawFrame(0,0,128,56);
    int cs=6,cx=2,cy=2;
    for(int y=0;y<H;y++) for(int x=0;x<W;x++)
      if(map[y][x]) oled.drawBox(cx+x*cs,cy+y*cs,cs-1,cs-1);
    for(int i=0;i<len;i++) {
      int px=cx+snX[i]*cs, py=cy+snY[i]*cs;
      if(i==0) oled.drawRBox(px,py,cs-1,cs-1,2);
      else oled.drawBox(px,py,cs-1,cs-1);
    }
    oled.drawDisc(cx+foodX*cs+(cs/2),cy+foodY*cs+(cs/2),2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)).c_str());
    present(); frameWait(t0);
  }
}
void runDynRunner(JsonDocument& doc) {
  waitRelease();
  float speed = doc["params"]["speed"] | 2.2f;
  float gravity = doc["params"]["gravity"] | 0.42f;
  float jumpV = doc["params"]["jump"] | 5.6f;
  int maxObstacles = doc["params"]["obstacles"] | 3;
  if (maxObstacles < 1) maxObstacles = 1;
  if (maxObstacles > 3) maxObstacles = 3;

  const int DX = 14;
  float dy = 0, vy = 0;
  bool onGround = true;
  float score = 0, scroll = 0;
  bool obOn[3] = {false,false,false};
  float obX[3] = {0,0,0};
  uint8_t obT[3] = {0,0,0};
  float untilNext = 60;
  bool dead = false;
  uint32_t deadAt = 0;
  int frameN = 0;

  auto reset = [&]() {
    dy=0; vy=0; onGround=true; score=0;
    for (int i=0;i<3;i++) obOn[i]=false;
    untilNext=60; dead=false;
  };

  playMusicTheme(TECH_DINO,OLD_DINO,CHILL_DINO,DARK_DINO,TECH_DINO,110);

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    frameN++;

    if (!dead) {
      if (onGround && (evDown[BI_OK] || evDown[BI_UP])) { vy = jumpV; onGround = false; SFX(SF_JUMP); }
      if (!onGround) { vy -= gravity; dy += vy; if (dy<=0) { dy=0; vy=0; onGround=true; } }
      
      scroll += speed;
      for (int i=0;i<maxObstacles;i++) if (obOn[i]) { obX[i] -= speed; if (obX[i]<-12) obOn[i]=false; }
      
      untilNext -= speed;
      if (untilNext<=0) {
        for (int i=0;i<maxObstacles;i++) {
          if (!obOn[i]) {
            obOn[i]=true; obX[i]=130; obT[i]=random(3);
            break;
          }
        }
        untilNext = random(60,100) + speed*12;
      }
      
      int dL=DX+2, dR=DX+10, dB=GROUND_Y-(int)dy, dT=dB-12;
      for (int i=0;i<3;i++) {
        if (!obOn[i]) continue;
        int ox=(int)obX[i]; int w=obW(obT[i]), h=obH(obT[i]);
        if (dR>ox+1 && dL<ox+w-1 && dB>GROUND_Y-h+1 && dT<GROUND_Y) {
          dead=true; deadAt=millis(); SFX(SF_DIE);
          break;
        }
      }
      score += speed * 0.1f;
    } else if (evDown[BI_OK] && millis()-deadAt > 600) reset();

    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,7,("HI "+padNum(getBest(APP_DINO),5)).c_str());
    oled.drawStr(128-oled.getStrWidth(padNum((int)score,5).c_str()),7,padNum((int)score,5).c_str());
    oled.drawHLine(0,GROUND_Y+1,128);
    for (int i=0;i<10;i++) {
      int px = ((i*37-(int)scroll)%128+128)%128;
      oled.drawHLine(px,GROUND_Y+4,2+i%3);
    }
    for (int i=0;i<3;i++) if (obOn[i]) drawCactus((int)obX[i],obT[i]);
    drawDino(DX,GROUND_Y-(int)dy,onGround&&!dead,frameN);
    if (dead && millis()-deadAt>350) overlayBox("GAME OVER","OK: retry");
    present(); frameWait(t0);
  }
}
void runDynFlappy(JsonDocument& doc) {
  waitRelease();
  float gravity = doc["params"]["gravity"] | 0.22f;
  float flapV = doc["params"]["flap"] | -2.7f;
  int gap = doc["params"]["gap"] | 28;
  float speed = doc["params"]["speed"] | 1.2f;

  const int BX=30, PW=12, PSP=54;
  float by=28, vy=0, scroll=0;
  float px[3]; int gapTop[3]; bool passed[3];
  int score=0;
  bool dead=false;
  uint32_t deadAt=0;
  int frameN=0;

  auto reset = [&]() {
    by=28; vy=0; score=0; dead=false;
    for (int i=0;i<3;i++) { px[i]=130+i*PSP; gapTop[i]=random(7,max(8,GROUND_Y-gap-6)); passed[i]=false; }
  };
  reset();

  playMusicTheme(TECH_FLAPPY,OLD_FLAPPY,CHILL_FLAPPY,DARK_FLAPPY,TECH_FLAPPY,120);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;
    frameN++;

    if (!dead) {
      if (evDown[BI_OK] || evDown[BI_UP]) { vy=flapV; SFX(SF_JUMP); }
      vy += gravity;
      if (vy>3.2f) vy=3.2f;
      by += vy;
      
      scroll += speed*0.5f;
      for (int i=0;i<3;i++) {
        px[i] -= speed;
        if (px[i] < -PW-2) {
          px[i] += 3*PSP;
          gapTop[i] = random(7,max(8,GROUND_Y-gap-6));
          passed[i] = false;
        }
        if (!passed[i] && px[i]+PW < BX-4) { passed[i]=true; score++; SFX(SF_SCORE); }
        if (BX+4 > px[i] && BX-4 < px[i]+PW) {
          if (by-4 < gapTop[i] || by+4 > gapTop[i]+gap) dead=true;
        }
      }
      if (by-4 < 0 || by+4 > GROUND_Y) dead=true;
      if (dead) { deadAt=millis(); SFX(SF_DIE); }
    } else if (evDown[BI_OK] && millis()-deadAt>500) reset();

    oled.clearBuffer();
    for (int i=0;i<3;i++) drawPipe((int)px[i],gapTop[i],gap,PW);
    drawBird(BX,(int)by,(frameN/3)%2);
    oled.drawHLine(0,GROUND_Y,128);
    oled.setDrawColor(0); oled.drawBox(0,0,128,9); oled.setDrawColor(1);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,7,("BEST "+padNum(getBest(APP_FLAPPY),3)).c_str());
    oled.drawStr(126-oled.getStrWidth(padNum(score,3).c_str()),7,padNum(score,3).c_str());
    if (dead && millis()-deadAt>300) overlayBox("GAME OVER",("Score "+String(score)).c_str());
    present(); frameWait(t0);
  }
}
void runDynMaze(JsonDocument& doc) {
  waitRelease();
  int speedMs = doc["params"]["speed_ms"] | 150;
  int wallCount = doc["params"]["walls"] | 4;
  if (wallCount < 1) wallCount = 1;
  if (wallCount > 6) wallCount = 6;

  const int GW=20, GH=8, CS=6, MAXLEN=GW*GH;
  const int DX[4]={1,0,-1,0}, DY[4]={0,1,0,-1};
  uint8_t maze[GH][GW];
  int snX[MAXLEN], snY[MAXLEN];

  auto buildMaze = [&]() {
    for (int r=0;r<GH;r++) for (int c=0;c<GW;c++) {
      bool wall = (r==0 || r==GH-1 || c==0 || c==GW-1);
      if (c==4 && r!=2 && r!=5) wall=true;
      if (c==8 && r!=1 && r!=6) wall=true;
      if (wallCount >= 3 && c==12 && r!=3 && r!=6) wall=true;
      if (wallCount >= 4 && c==16 && r!=2 && r!=5) wall=true;
      maze[r][c] = wall ? 1 : 0;
    }
    for (int c=7;c<=13;c++) maze[4][c]=0;
    maze[3][8]=maze[2][8]=maze[1][8]=0;
    maze[5][12]=maze[6][12]=0;
  };

  int len=3, dir=0, pend=0, score=0;
  int foodX=15, foodY=3;
  bool dead=false;
  uint32_t lastStep=0;
  int best=getBest(APP_SNAKE_MAZE);

  auto placeFood = [&]() {
    for (int t=0;t<200;t++) {
      int x=random(1,GW-1), y=random(1,GH-1);
      if (maze[y][x]) continue;
      bool on=false;
      for(int i=0;i<len;i++) if(snX[i]==x&&snY[i]==y){on=true;break;}
      if(!on){foodX=x;foodY=y;return;}
    }
  };

  auto reset = [&]() {
    buildMaze(); len=3; dir=0; pend=0; score=0; dead=false;
    snX[0]=10;snY[0]=4; snX[1]=9;snY[1]=4; snX[2]=8;snY[2]=4;
    placeFood(); lastStep=millis();
  };
  reset();

  playMusicTheme(TECH_SNAKE,OLD_SNAKE,CHILL_SNAKE,DARK_SNAKE,TECH_SNAKE,110);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;

    if (!dead) {
      if (evDown[BI_RIGHT] && dir!=2) pend=0;
      if (evDown[BI_DOWN]  && dir!=3) pend=1;
      if (evDown[BI_LEFT]  && dir!=0) pend=2;
      if (evDown[BI_UP]    && dir!=1) pend=3;
      
      if ((int)(millis()-lastStep) >= speedMs) {
        lastStep=millis(); dir=pend;
        int hx=snX[0]+DX[dir], hy=snY[0]+DY[dir];
        bool eating=(hx==foodX && hy==foodY);
        bool wall=hx<0||hx>=GW||hy<0||hy>=GH||maze[hy][hx];
        bool self=false;
        for (int i=0;i<len-(eating?0:1);i++) if(snX[i]==hx&&snY[i]==hy){self=true;break;}
        if (wall||self) { dead=true; SFX(SF_DIE); if(score>best) best=score; setBest(APP_SNAKE_MAZE,score); }
        else {
          if (eating && len<MAXLEN) len++;
          for (int i=len-1;i>0;i--) { snX[i]=snX[i-1]; snY[i]=snY[i-1]; }
          snX[0]=hx; snY[0]=hy;
          if (eating) { score+=10; SFX(SF_EAT); placeFood(); }
        }
      }
    } else if (evDown[BI_OK]) reset();

    oled.clearBuffer();
    oled.drawFrame(0,0,120,48);
    for (int r=0;r<GH;r++) for (int c=0;c<GW;c++) if (maze[r][c]) {
      oled.drawBox(c*CS+1,r*CS+1,4,4);
    }
    oled.drawFrame(foodX*CS+1,foodY*CS+1,4,4);
    for (int i=len-1;i>0;i--) oled.drawRBox(snX[i]*CS+1,snY[i]*CS+1,4,4,1);
    oled.drawDisc(snX[0]*CS+3,snY[0]*CS+3,2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("FOOD "+String(score/10)+" HI "+String(best/10)).c_str());
    if (dead) overlayBox("CRASHED","OK: retry");
    present(); frameWait(t0);
  }
}
void runDynJumper(JsonDocument& doc) {
  waitRelease();
  float gravity = doc["params"]["gravity"] | 0.35f;
  float jumpV = doc["params"]["jump"] | -5.5f;
  int numPlat = doc["params"]["platforms"] | 6;
  if (numPlat < 4) numPlat = 4;
  if (numPlat > 8) numPlat = 8;

  float jx=60, jy=50, vy=0;
  int score=0;
  float platforms[8][2];
  bool dead=false;

  auto reset = [&]() {
    jx=60; jy=50; vy=jumpV; score=0; dead=false;
    for (int i=0;i<numPlat;i++) {
      platforms[i][0]=random(10,110);
      platforms[i][1]=60-i*10;
    }
  };
  reset();

  playMusicTheme(TECH_DINO,OLD_DINO,CHILL_DINO,DARK_DINO,TECH_DINO,130);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;

    if (!dead) {
      if (held(BI_LEFT)) jx -= 2.5f;
      if (held(BI_RIGHT)) jx += 2.5f;
      if (jx<5) jx=5; if (jx>118) jx=118;
      vy += gravity; jy += vy;
      
      for (int i=0;i<numPlat;i++) {
        if (vy>0 && jy+3>=platforms[i][1] && jy+3<=platforms[i][1]+4
            && jx+4>=platforms[i][0] && jx-4<=platforms[i][0]+14) {
          vy = jumpV; jy = platforms[i][1]-3; score++; SFX(SF_JUMP);
        }
      }
      
      if (jy < 25) {
        float diff = 25 - jy;
        jy = 25;
        for (int i=0;i<numPlat;i++) {
          platforms[i][1] += diff;
          if (platforms[i][1] > 64) {
            platforms[i][1] = -5;
            platforms[i][0] = random(5,115);
          }
        }
      }
      
      if (jy > 64) { dead=true; SFX(SF_DIE); }
    } else if (evDown[BI_OK]) reset();

    oled.clearBuffer();
    for (int i=0;i<numPlat;i++) {
      int px=(int)platforms[i][0], py=(int)platforms[i][1];
      if (py>=-3 && py<=64) oled.drawRBox(px,py,14,3,1);
    }
    oled.drawDisc((int)jx,(int)jy,3);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0,62,("JUMPS "+String(score)).c_str());
    if (dead) overlayBox("FELL!","OK: retry");
    present(); frameWait(t0);
  }
}
void runDynInvaders(JsonDocument& doc) {
  waitRelease();
  int enemyCount = doc["params"]["enemies"] | 24;
  float speed = doc["params"]["speed"] | 0.6f;
  if (enemyCount < 8) enemyCount = 8;
  if (enemyCount > 32) enemyCount = 32;

  float ex[32]; int ey[32]; bool alive[32];
  for (int i=0;i<enemyCount;i++) {
    ex[i] = 10 + (i%8)*13;
    ey[i] = 6 + (i/8)*8;
    alive[i] = true;
  }
  float shipX=64, shotX=-1, shotY=0;
  float dir=speed;
  int score=0;

  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,140);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;
    if (held(BI_LEFT)) shipX -= 3;
    if (held(BI_RIGHT)) shipX += 3;
    shipX = constrain(shipX,6,120);
    if (evDown[BI_OK] && shotX<0) { shotX=shipX; shotY=52; SFX(SF_SHOOT); }
    
    bool change=false;
    for (int i=0;i<enemyCount;i++) {
      ex[i] += dir;
      if (ex[i]<6 || ex[i]>120) change=true;
    }
    if (change) { dir=-dir; for (int i=0;i<enemyCount;i++) ey[i]+=6; }
    
    if (shotX>=0) { shotY -= 4; if (shotY<0) shotX=-1; }
    for (int i=0;i<enemyCount;i++) if (alive[i] && shotX>=0) {
      if (abs(shotX-ex[i])<6 && abs(shotY-ey[i])<4) { alive[i]=false; shotX=-1; score+=10; SFX(SF_HIT); }
    }
    
    for (int i=0;i<enemyCount;i++) if (alive[i] && ey[i]>46) {
      overlayBox("GAME OVER","Invaders won"); waitRelease(); return;
    }
    
    bool any=false;
    for (int i=0;i<enemyCount;i++) if (alive[i]) any=true;
    if (!any) { overlayBox("YOU WIN",("Score "+String(score)).c_str()); waitRelease(); return; }
    
    oled.clearBuffer();
    for (int i=0;i<enemyCount;i++) if (alive[i]) oled.drawBox((int)ex[i],ey[i],8,4);
    oled.drawRBox((int)shipX-4,54,8,4,2);
    if (shotX>=0) oled.drawBox((int)shotX,(int)shotY,2,4);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)).c_str());
    present(); frameWait(t0);
  }
}
void runDynPong(JsonDocument& doc) {
  waitRelease();
  float ballSpeed = doc["params"]["ball_speed"] | 2.0f;
  float aiSpeed = doc["params"]["ai_speed"] | 2.0f;
  int winScore = doc["params"]["win_score"] | 5;

  int paddleL=28, paddleR=28;
  const int PW=2, PH=10;
  float ballX=64, ballY=28, vX=ballSpeed, vY=ballSpeed*0.6f;
  int scoreL=0, scoreR=0;

  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,160);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;
    if (held(BI_UP)) paddleL -= 3;
    if (held(BI_DOWN)) paddleL += 3;
    paddleL = constrain(paddleL,2,46);
    
    if (ballY < paddleR+PH/2) paddleR -= (int)aiSpeed; else paddleR += (int)aiSpeed;
    paddleR = constrain(paddleR,2,46);
    
    ballX += vX; ballY += vY;
    if (ballY<2 || ballY>56-2) { vY=-vY; SFX(SF_HIT); }
    if (ballX<=6 && ballY>=paddleL && ballY<=paddleL+PH) { vX=-vX; ballX=8; vX*=1.03; SFX(SF_HIT); }
    if (ballX>=122 && ballY>=paddleR && ballY<=paddleR+PH) { vX=-vX; ballX=120; vX*=1.03; SFX(SF_HIT); }
    
    if (ballX<0) { scoreR++; ballX=64; ballY=28; vX=ballSpeed; vY=ballSpeed*0.6f; SFX(SF_DIE); }
    if (ballX>128) { scoreL++; ballX=64; ballY=28; vX=-ballSpeed; vY=ballSpeed*0.6f; SFX(SF_SCORE); }
    
    if (scoreL>=winScore || scoreR>=winScore) {
      overlayBox("MATCH END",("Score "+String(scoreL)+":"+String(scoreR)).c_str());
      waitRelease(); return;
    }
    
    oled.clearBuffer();
    for (int y=0;y<56;y+=4) oled.drawPixel(64,y+2);
    oled.drawBox(2,paddleL,PW,PH);
    oled.drawBox(124,paddleR,PW,PH);
    oled.drawDisc((int)ballX,(int)ballY,1);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,(String(scoreL)+" : "+String(scoreR)).c_str());
    present(); frameWait(t0);
  }
}
// ===== ДВИЖКИ ДЛЯ ДИНАМИЧЕСКИХ ИГР =====
void runDynSnake(JsonDocument& doc);
void runDynRunner(JsonDocument& doc);
void runDynFlappy(JsonDocument& doc);
void runDynBreakout(JsonDocument& doc);
void runDynMaze(JsonDocument& doc);
void runDynJumper(JsonDocument& doc);
void runDynInvaders(JsonDocument& doc);
void runDynPong(JsonDocument& doc);
void runDynBreakout(JsonDocument& doc) {
  int bricksHP = doc["params"]["bricks_hp"] | 2;
  float ballSpeed = doc["params"]["ball_speed"] | 2.2f;
  int paddleWidth = doc["params"]["paddle_width"] | 24;
  int rows = doc["params"]["rows"] | 4;
  if (rows < 2) rows = 2;
  if (rows > 6) rows = 6;

  int paddleX = 50;
  float bx = 64, by = 48, vx = ballSpeed, vy = -ballSpeed*0.9f;
  const int BR = 6, BC = 8, PH = 4;
  bool bricks[BR][BC];
  uint8_t hp[BR][BC];
  for (int r=0;r<rows;r++) for (int c=0;c<BC;c++) {
    bricks[r][c] = true;
    hp[r][c] = (r < rows/2) ? bricksHP : 1;
  }
  int score = 0;

  playMusicTheme(TECH_TETRIS,OLD_TETRIS,CHILL_TETRIS,DARK_TETRIS,TECH_TETRIS,140);

  while (true) {
    uint32_t t0=millis(); pollButtons();
    if (quitPressed()) break;
    if (held(BI_LEFT)) paddleX -= 3;
    if (held(BI_RIGHT)) paddleX += 3;
    paddleX = constrain(paddleX, 2, 128-paddleWidth-2);
    
    bx += vx; by += vy;
    if (bx<2 || bx>126) { vx=-vx; SFX(SF_HIT); }
    if (by<2) { vy=-vy; SFX(SF_HIT); }
    if (by>=48 && by<=52 && bx>=paddleX && bx<=paddleX+paddleWidth) {
      vy = -fabs(vy); vx += (bx-(paddleX+paddleWidth/2))*0.04f; SFX(SF_HIT);
    }
    
    int br = (int)((by-4)/6);
    int bc = (int)((bx-2)/15);
    if (br>=0 && br<rows && bc>=0 && bc<BC && bricks[br][bc]) {
      hp[br][bc]--;
      if (hp[br][bc]<=0) { bricks[br][bc]=false; score+=10; }
      else score+=5;
      vy = -vy; SFX(SF_SCORE);
    }
    
    if (by>64) { overlayBox("GAME OVER",("Score "+String(score)).c_str()); waitRelease(); return; }
    
    bool any=false;
    for (int r=0;r<rows;r++) for (int c=0;c<BC;c++) if (bricks[r][c]) any=true;
    if (!any) { overlayBox("YOU WIN",("Score "+String(score)).c_str()); waitRelease(); return; }
    
    oled.clearBuffer();
    for (int r=0;r<rows;r++) for (int c=0;c<BC;c++) {
      if (!bricks[r][c]) continue;
      int px = 2+c*15, py = 4+r*6;
      if (hp[r][c]>=2) {
        for (int xx=0;xx<13;xx+=2) oled.drawPixel(px+xx,py);
        for (int xx=0;xx<13;xx+=2) oled.drawPixel(px+xx,py+3);
      } else oled.drawBox(px,py,13,4);
    }
    oled.drawRBox(paddleX,54,paddleWidth,PH,2);
    oled.drawDisc((int)bx,(int)by,2);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(2,62,("S "+String(score)).c_str());
    present(); frameWait(t0);
  }
}
// Простой парсер: сначала *, /, потом +, -
float dynEval(const String& expr) {
  float nums[16]; char ops[16];
  int n=0, o=0, i=0;
  while (i < expr.length()) {
    int start = i;
    if (expr[i] == '-' && (i == 0 || String("+-*/").indexOf(expr[i-1]) >= 0)) i++;
    while (i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) i++;
    if (i > start) nums[n++] = expr.substring(start, i).toFloat();
    if (i < expr.length() && String("+-*/").indexOf(expr[i]) >= 0) {
      ops[o++] = expr[i]; i++;
    } else if (i < expr.length()) i++;
  }
  if (n == 0) return 0;
  for (int k=0; k<o; k++) {
    if (ops[k] == '*' || ops[k] == '/') {
      float r = (ops[k]=='*') ? nums[k]*nums[k+1] : ((nums[k+1]!=0) ? nums[k]/nums[k+1] : 0);
      nums[k] = r;
      for (int j=k+1; j<n-1; j++) nums[j] = nums[j+1];
      for (int j=k; j<o-1; j++) ops[j] = ops[j+1];
      n--; o--; k--;
    }
  }
  float res = nums[0];
  for (int k=0; k<o; k++) {
    if (ops[k] == '+') res += nums[k+1];
    else if (ops[k] == '-') res -= nums[k+1];
  }
  return res;
}

void runDynCalculator(JsonDocument& doc) {
  waitRelease();
  String opsAllowed = doc["params"]["ops"] | "+-*/";
  int precision = doc["params"]["precision"] | 2;

  // 4x4 сетка
  char buttons[4][4] = {
    {'7','8','9','/'},
    {'4','5','6','*'},
    {'1','2','3','-'},
    {'C','0','=','+'}
  };
  // Отключаем запрещённые операции
  for (int r=0;r<4;r++) for (int c=0;c<4;c++) {
    char ch = buttons[r][c];
    if ((ch=='+'||ch=='-'||ch=='*'||ch=='/') && opsAllowed.indexOf(ch) < 0)
      buttons[r][c] = ' ';
  }

  String expr = "";
  String result = "0";
  int curR = 1, curC = 0;

  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,160);

  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;

    if (nav(BI_UP))    curR = (curR + 3) % 4;
    if (nav(BI_DOWN))  curR = (curR + 1) % 4;
    if (nav(BI_LEFT))  curC = (curC + 3) % 4;
    if (nav(BI_RIGHT)) curC = (curC + 1) % 4;

    if (evDown[BI_OK]) {
      char ch = buttons[curR][curC];
      if (ch == 'C') { expr = ""; result = "0"; }
      else if (ch == '=') {
        float r = dynEval(expr);
        if (precision >= 0 && precision <= 4) {
          result = String(r, precision);
          while (result.endsWith("0") && result.indexOf('.') >= 0) result.remove(result.length()-1);
          if (result.endsWith(".")) result.remove(result.length()-1);
        } else result = String(r);
        if (result == "-0") result = "0";
      }
      else if (ch != ' ') expr += ch;
      SFX(SF_CLICK);
    }

    // Отрисовка
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    String dispE = expr;
    if (dispE.length() > 20) dispE = "..." + dispE.substring(dispE.length()-17);
    oled.drawStr(0, 8, dispE.c_str());
    
    oled.setFont(u8g2_font_6x13_tr);
    String dispR = result;
    if (dispR.length() > 16) dispR = dispR.substring(0, 16);
    oled.drawStr(0, 22, dispR.c_str());
    oled.drawHLine(0, 25, 128);

    // 4x4 кнопки
    int bw = 30, bh = 8;
    int bx0 = 2, by0 = 28;
    for (int r=0;r<4;r++) for (int c=0;c<4;c++) {
      int x = bx0 + c*(bw+1);
      int y = by0 + r*(bh+1);
      char ch = buttons[r][c];
      String lbl = (ch == ' ') ? "" : String(ch);
      if (r==curR && c==curC) {
        oled.drawRBox(x, y, bw, bh, 2);
        oled.setDrawColor(0);
        oled.drawStr(x + (bw - oled.getStrWidth(lbl.c_str()))/2, y+bh-2, lbl.c_str());
        oled.setDrawColor(1);
      } else {
        oled.drawFrame(x, y, bw, bh);
        oled.drawStr(x + (bw - oled.getStrWidth(lbl.c_str()))/2, y+bh-2, lbl.c_str());
      }
    }
    present();
    frameWait(t0);
  }
}
void runDynMiniApp(JsonDocument& doc) {
  waitRelease();
  
  struct MV { String name; int32_t value; };
  MV vars[8];
  int nv = 0;
  JsonObject vobj = doc["params"]["vars"];
  for (JsonPair kv : vobj) {
    if (nv >= 8) break;
    vars[nv].name = String(kv.key().c_str());
    vars[nv].value = kv.value().as<long>();
    nv++;
  }
  
  String displayBind = doc["params"]["display"] | "";
  String title = doc["params"]["title"] | "";
  
  struct MBtn {
    int8_t x, y, w, h;
    String label;
    uint8_t action;
    String target;
    String val;
  };
  MBtn btn[16];
  int nb = 0;
  JsonArray ja = doc["params"]["buttons"];
  for (JsonObject b : ja) {
    if (nb >= 16) break;
    btn[nb].x = b["x"] | 0;
    btn[nb].y = b["y"] | 40;
    btn[nb].w = b["w"] | 30;
    btn[nb].h = b["h"] | 20;
    
    // Label: может быть строкой или числом
    JsonVariant lv = b["label"];
    if (lv.isNull()) btn[nb].label = "?";
    else if (lv.is<const char*>()) btn[nb].label = String(lv.as<const char*>());
    else btn[nb].label = String(lv.as<long>());
    
    // Action: строка
    String act = String((const char*)(b["act"] | "set"));
    btn[nb].action = 0;
    if (act == "add") btn[nb].action = 1;
    else if (act == "mul") btn[nb].action = 2;
    else if (act == "expr") btn[nb].action = 3;
    else if (act == "rand") btn[nb].action = 4;
    else if (act == "exit") btn[nb].action = 5;
    
    btn[nb].target = String((const char*)(b["var"] | ""));
    
    // ★ ГЛАВНЫЙ ФИКС ★
    // val: может быть строкой ("5") или числом (5) — обрабатываем оба
    JsonVariant v = b["val"];
    if (v.isNull()) {
      btn[nb].val = "0";
    } else if (v.is<const char*>()) {
      btn[nb].val = String(v.as<const char*>());
    } else if (v.is<float>() && !v.is<long>()) {
      btn[nb].val = String(v.as<float>(), 2);
    } else {
      btn[nb].val = String(v.as<long>());
    }
    
    nb++;
  }
  
  String tickVar = doc["params"]["tick"]["var"] | "";
  int32_t tickBy = doc["params"]["tick"]["by"] | 0;
  String tickIf = doc["params"]["tick"]["if"] | "";
  int tickMs = doc["params"]["tick"]["ms"] | 1000;
  uint32_t lastTick = millis();
  
  auto findVar = [&](const String& name) -> int {
    for (int i = 0; i < nv; i++) if (vars[i].name == name) return i;
    return -1;
  };
  
  auto resolveVal = [&](const String& s) -> int32_t {
    String t = s; t.trim();
    if (!t.length()) return 0;
    bool isNum = (t[0] >= '0' && t[0] <= '9');
    if (!isNum && t[0] == '-' && t.length() > 1 && t[1] >= '0' && t[1] <= '9') isNum = true;
    if (isNum) return t.toInt();
    int vi = findVar(t);
    if (vi >= 0) return vars[vi].value;
    return 0;
  };
  
  int sel = 0;
  playMusicTheme(TECH_MENU, OLD_MENU, CHILL_MENU, DARK_MENU, TECH_MENU, 160);
  
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    
    if (tickVar.length() && millis() - lastTick >= (uint32_t)tickMs) {
      lastTick = millis();
      bool shouldTick = true;
      if (tickIf.length()) {
        int ti = findVar(tickIf);
        shouldTick = (ti >= 0 && vars[ti].value != 0);
      }
      if (shouldTick) {
        int vi = findVar(tickVar);
        if (vi >= 0) vars[vi].value += tickBy;
      }
    }
    
    if (nav(BI_LEFT)  && sel > 0)    sel--;
    if (nav(BI_RIGHT) && sel < nb - 1) sel++;
    
    if (evDown[BI_OK] && nb > 0) {
      MBtn& b = btn[sel];
      int vi = findVar(b.target);
      if (b.action == 0 && vi >= 0)      vars[vi].value = resolveVal(b.val);
      else if (b.action == 1 && vi >= 0) vars[vi].value += resolveVal(b.val);
      else if (b.action == 2 && vi >= 0) vars[vi].value *= resolveVal(b.val);
      else if (b.action == 3 && vi >= 0) {
        String e = b.val;
        for (int i = 0; i < nv; i++) e.replace(vars[i].name, String(vars[i].value));
        vars[vi].value = (int32_t)dynEval(e);
      }
      else if (b.action == 4 && vi >= 0) {
        int32_t mx = resolveVal(b.val);
        vars[vi].value = random(0, mx > 0 ? mx : 1);
      }
      else if (b.action == 5) break;
      SFX(SF_CLICK);
    }
    
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    if (title.length()) oled.drawUTF8(0, 8, title.c_str());
    oled.drawHLine(0, 11, 128);
    
    int vi = findVar(displayBind);
    String disp = (vi >= 0) ? String(vars[vi].value) : "--";
    oled.setFont(u8g2_font_9x15_tr);
    int w = oled.getStrWidth(disp.c_str());
    oled.drawStr(64 - w/2, 28, disp.c_str());
    
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i = 0; i < nb; i++) {
      MBtn& b = btn[i];
      if (i == sel) {
        oled.drawRBox(b.x, b.y, b.w, b.h, 2);
        oled.setDrawColor(0);
        int tw = oled.getUTF8Width(b.label.c_str());
        oled.drawUTF8(b.x + (b.w - tw)/2, b.y + b.h/2 + 3, b.label.c_str());
        oled.setDrawColor(1);
      } else {
        oled.drawFrame(b.x, b.y, b.w, b.h);
        int tw = oled.getUTF8Width(b.label.c_str());
        oled.drawUTF8(b.x + (b.w - tw)/2, b.y + b.h/2 + 3, b.label.c_str());
      }
    }
    present();
    frameWait(t0);
  }
}
void runDynText(JsonDocument& doc) {
  waitRelease();
  String file = doc["params"]["file"] | "";
  int scrollLine = 0;
  
  File f = SD.open(file);
  if (!f) { showScreen("Text", "Cannot open", file.c_str()); waitAnyKey(); return; }
  
  String lines[200];
  int n = 0;
  while (f.available() && n < 200) {
    String l = f.readStringUntil('\n');
    l.trim();
    lines[n++] = l;
  }
  f.close();
  
  if (n == 0) { showScreen("Text", "Empty", ""); waitAnyKey(); return; }
  
  playMusicTheme(TECH_MENU, OLD_MENU, CHILL_MENU, DARK_MENU, TECH_MENU, 160);
  
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    
    if (nav(BI_DOWN) && scrollLine < n - 1) scrollLine++;
    if (nav(BI_UP) && scrollLine > 0) scrollLine--;
    if (nav(BI_RIGHT) && scrollLine < n - 6) scrollLine += 6;
    if (nav(BI_LEFT) && scrollLine >= 6) scrollLine -= 6;
    
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    for (int i = 0; i < 7; i++) {
      int idx = scrollLine + i;
      if (idx >= n) break;
      String l = lines[idx];
      while (l.length() > 21) l.remove(l.length()-1);
      oled.drawUTF8(0, 8 + i*8, l.c_str());
    }
    if (n > 7) {
      int track = 56;
      int thumb = max(4, track * 7 / n);
      int ty = (track - thumb) * scrollLine / max(1, n-7);
      oled.drawVLine(126, 0, track);
      oled.drawBox(125, ty, 2, thumb);
    }
    present();
    frameWait(t0);
  }
}

void runDynQuiz(JsonDocument& doc) {
  waitRelease();
  JsonArray qs = doc["params"]["questions"];
  int total = qs.size();
  if (total == 0) { showScreen("Quiz", "No questions", ""); waitAnyKey(); return; }
  
  int score = 0;
  int correctIdx[32];
  for (int i = 0; i < total && i < 32; i++) correctIdx[i] = qs[i]["ans"] | 0;
  
  playMusicTheme(TECH_MENU, OLD_MENU, CHILL_MENU, DARK_MENU, TECH_MENU, 160);
  
  int qi = 0;
  int sel = 0;
  bool answered = false;
  int lastCorrect = -1;
  
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    
    if (qi >= total) {
      overlayBox("QUIZ DONE", ("Score: " + String(score) + "/" + String(total)).c_str());
      waitRelease();
      break;
    }
    
    if (!answered) {
      if (nav(BI_UP))   sel = (sel + 3) % 4;
      if (nav(BI_DOWN)) sel = (sel + 1) % 4;
      if (nav(BI_LEFT)) sel = (sel + 3) % 4;
      if (nav(BI_RIGHT))sel = (sel + 1) % 4;
      
      if (evDown[BI_OK]) {
        answered = true;
        lastCorrect = correctIdx[qi];
        if (sel == lastCorrect) { score++; SFX(SF_SCORE); }
        else SFX(SF_DIE);
      }
    } else {
      if (evDown[BI_OK] || evTap[BI_OK]) {
        qi++; sel = 0; answered = false; lastCorrect = -1;
      }
    }
    
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0, 8, (String(qi+1) + "/" + String(total) + "  S:" + String(score)).c_str());
    oled.drawHLine(0, 10, 128);
    
    String q = qs[qi]["q"] | "?";
    oled.drawUTF8(2, 22, q.c_str());
    
    JsonArray a = qs[qi]["a"];
    for (int i = 0; i < 4 && i < (int)a.size(); i++) {
      String opt = a[i] | "?";
      if (opt.length() > 20) opt = opt.substring(0, 20);
      int y = 32 + (i/2)*12;
      int x = (i%2) * 64;
      
      if (answered && i == lastCorrect) {
        oled.drawRBox(x+1, y-7, 62, 10, 2);
        oled.setDrawColor(0);
        oled.drawUTF8(x+3, y, opt.c_str());
        oled.setDrawColor(1);
      } else if (i == sel && !answered) {
        oled.drawRBox(x+1, y-7, 62, 10, 2);
        oled.setDrawColor(0);
        oled.drawUTF8(x+3, y, opt.c_str());
        oled.setDrawColor(1);
      } else {
        oled.drawFrame(x+1, y-7, 62, 10);
        oled.drawUTF8(x+3, y, opt.c_str());
      }
    }
    present();
    frameWait(t0);
  }
}

void runDynTodo(JsonDocument& doc) {
  waitRelease();
  String file = doc["params"]["file"] | "/todo.txt";
  
  auto loadTasks = [&](String* tasks, int& n) {
    n = 0;
    File f = SD.open(file);
    if (!f) return;
    while (f.available() && n < 50) {
      String l = f.readStringUntil('\n');
      l.trim();
      if (l.length()) tasks[n++] = l;
    }
    f.close();
  };
  
  auto saveTasks = [&](String* tasks, int n) {
    File f = SD.open(file, FILE_WRITE);
    if (!f) return;
    for (int i = 0; i < n; i++) f.println(tasks[i]);
    f.close();
  };
  
  String tasks[50];
  int n = 0;
  loadTasks(tasks, n);
  
  int sel = 0;
  playMusicTheme(TECH_MENU, OLD_MENU, CHILL_MENU, DARK_MENU, TECH_MENU, 160);
  
  while (true) {
    uint32_t t0 = millis();
    pollButtons();
    if (quitPressed()) break;
    
    if (n > 0) {
      if (nav(BI_UP) && sel > 0) sel--;
      if (nav(BI_DOWN) && sel < n - 1) sel++;
    }
    
    if (evDown[BI_OK] && n > 0) {
      String t = tasks[sel];
      if (t.startsWith("[x] ")) t = "[ ] " + t.substring(4);
      else if (t.startsWith("[ ] ")) t = "[x] " + t.substring(4);
      tasks[sel] = t;
      saveTasks(tasks, n);
      SFX(SF_CLICK);
    }
    
    if (evDown[BI_BACK] && n > 0) {
      if (confirmBox("Delete", tasks[sel].c_str(), "OK=yes BACK=no")) {
        for (int i = sel; i < n-1; i++) tasks[i] = tasks[i+1];
        n--;
        if (sel >= n) sel = max(0, n-1);
        saveTasks(tasks, n);
      }
    }
    
    if (held(BI_OK) && heldMs(BI_OK) >= 1500 && !bst[BI_OK].longDone) {
      bst[BI_OK].longDone = true;
      String newTask = "";
      if (runKeyboard("New task", newTask, false) && newTask.length()) {
        tasks[n] = "[ ] " + newTask;
        n++;
        saveTasks(tasks, n);
      }
      waitRelease();
    }
    
    oled.clearBuffer();
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(0, 8, "TODO");
    oled.drawHLine(0, 10, 128);
    
    if (n == 0) {
      oled.drawStr(10, 32, "Empty");
      oled.drawStr(10, 44, "hold OK to add");
    } else {
      int start = max(0, sel - 2);
      for (int i = 0; i < 5 && start + i < n; i++) {
        int idx = start + i;
        String t = tasks[idx];
        if (t.length() > 20) t = t.substring(0, 20);
        if (idx == sel) {
          oled.drawRBox(0, 14 + i*9, 128, 9, 2);
          oled.setDrawColor(0);
        }
        oled.drawUTF8(2, 21 + i*9, t.c_str());
        if (idx == sel) oled.setDrawColor(1);
      }
    }
    oled.drawStr(0, 63, "OK: toggle  hold: add");
    present();
    frameWait(t0);
  }
}
void runDynGemini(JsonDocument& doc) {
  waitRelease();
  String prompt = doc["params"]["prompt"] | "";
  bool freeMode = doc["params"]["free"] | false;
  bool shouldSave = doc["params"]["save_note"] | true;
  
  if (freeMode) {
    prompt = "";
    if (!runKeyboard("Ask Gemini", prompt, false)) return;
    if (!prompt.length()) return;
  } else if (!prompt.length()) {
    showScreen("Gemini", "No prompt configured", "");
    waitAnyKey();
    return;
  }
  
  showScreen("Gemini", "Thinking...");
  String answer = askAI(prompt);
  showAnswer(answer);
  
    if (shouldSave) {
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x13_t_cyrillic);
    oled.drawStr(0, 12, "Save answer?");
    oled.drawHLine(0, 16, 128);
    oled.setFont(u8g2_font_5x8_t_cyrillic);
    oled.drawStr(4, 32, "OK: save as note");
    oled.drawStr(4, 44, "BACK: discard");
    present();
    waitRelease();
    while (true) {
      pollButtons();
      if (evDown[BI_OK]) {
        String noteName = createNote();
        if (noteName.length()) {
          String noteText = "Q: " + prompt + "\n\nA: " + answer;
          saveNote(noteName, noteText);
          showScreen("Saved", noteName, "");
          waitAnyKey();
        }
        break;
      }
      if (evTap[BI_BACK] || evLong[BI_BACK]) break;
      present(); delay(20);
    }
  }
}
// ===== CHIP-8 EMULATOR =====
namespace chip8 {
  uint8_t mem[4096];
  uint8_t V[16];
  uint16_t I = 0;
  uint16_t PC = 0x200;
  uint16_t stack[16];
  uint8_t SP = 0;
  uint8_t DT = 0, ST = 0;
  uint8_t gfx[64*32];
  uint8_t keys[16];
  uint16_t opcode;
  bool waitingForKey = false;
  uint8_t waitReg = 0;
  uint32_t lastCycle = 0;
  uint32_t lastTimer = 0;
  
  const uint8_t FONTSET[80] = {
    0xF0,0x90,0x90,0x90,0xF0, 0x20,0x60,0x20,0x20,0x70,
    0xF0,0x10,0xF0,0x80,0xF0, 0xF0,0x10,0xF0,0x10,0xF0,
    0x90,0x90,0xF0,0x10,0x10, 0xF0,0x80,0xF0,0x10,0xF0,
    0xF0,0x80,0xF0,0x90,0xF0, 0xF0,0x10,0x20,0x40,0x40,
    0xF0,0x90,0xF0,0x90,0xF0, 0xF0,0x90,0xF0,0x10,0xF0,
    0xF0,0x90,0xF0,0x90,0x90, 0xE0,0x90,0xE0,0x90,0xE0,
    0xF0,0x80,0x80,0x80,0xF0, 0xE0,0x90,0x90,0x90,0xE0,
    0xF0,0x80,0xF0,0x80,0xF0, 0xF0,0x80,0xF0,0x80,0x80
  };
  
  void init() {
    memset(mem, 0, 4096);
    memset(V, 0, 16);
    memset(gfx, 0, 64*32);
    memset(stack, 0, sizeof(stack));
    I = 0; PC = 0x200; SP = 0; DT = 0; ST = 0;
    waitingForKey = false; waitReg = 0;
    for (int i = 0; i < 80; i++) mem[i] = FONTSET[i];
  }
  
  bool loadRom(const String& path) {
    File f = SD.open(path);
    if (!f || f.isDirectory()) { if (f) f.close(); return false; }
    size_t sz = f.size();
    if (sz > 3584) { f.close(); return false; }
    f.read(&mem[0x200], sz);
    f.close();
    return true;
  }
  
  void cycle() {
    opcode = (mem[PC] << 8) | mem[PC+1];
    PC += 2;
    uint8_t x = (opcode >> 8) & 0x0F;
    uint8_t y = (opcode >> 4) & 0x0F;
    uint8_t n = opcode & 0x0F;
    uint8_t nn = opcode & 0xFF;
    uint16_t nnn = opcode & 0xFFF;
    
    switch (opcode & 0xF000) {
      case 0x0000:
        if (opcode == 0x00E0) memset(gfx, 0, 64*32);
        else if (opcode == 0x00EE) PC = stack[--SP];
        break;
      case 0x1000: PC = nnn; break;
      case 0x2000: stack[SP++] = PC; PC = nnn; break;
      case 0x3000: if (V[x] == nn) PC += 2; break;
      case 0x4000: if (V[x] != nn) PC += 2; break;
      case 0x5000: if (V[x] == V[y]) PC += 2; break;
      case 0x6000: V[x] = nn; break;
      case 0x7000: V[x] += nn; break;
      case 0x8000:
        switch (n) {
          case 0x0: V[x] = V[y]; break;
          case 0x1: V[x] |= V[y]; break;
          case 0x2: V[x] &= V[y]; break;
          case 0x3: V[x] ^= V[y]; break;
          case 0x4: { uint16_t s = V[x] + V[y]; V[0xF] = (s > 255) ? 1 : 0; V[x] = s & 0xFF; } break;
          case 0x5: V[0xF] = (V[x] >= V[y]) ? 1 : 0; V[x] -= V[y]; break;
          case 0x6: V[0xF] = V[x] & 1; V[x] >>= 1; break;
          case 0x7: V[0xF] = (V[y] >= V[x]) ? 1 : 0; V[x] = V[y] - V[x]; break;
          case 0xE: V[0xF] = (V[x] >> 7) & 1; V[x] <<= 1; break;
        }
        break;
      case 0x9000: if (V[x] != V[y]) PC += 2; break;
      case 0xA000: I = nnn; break;
      case 0xB000: PC = nnn + V[0]; break;
      case 0xC000: V[x] = random(256) & nn; break;
      case 0xD000: {
        uint16_t vx = V[x] % 64, vy = V[y] % 32;
        V[0xF] = 0;
        for (int row = 0; row < n; row++) {
          uint8_t spr = mem[I + row];
          for (int col = 0; col < 8; col++) {
            if ((spr & (0x80 >> col)) != 0) {
              int px = (vx + col) % 64;
              int py = (vy + row) % 32;
              int idx = py * 64 + px;
              if (gfx[idx]) V[0xF] = 1;
              gfx[idx] ^= 1;
            }
          }
        }
      } break;
      case 0xE000:
        if (nn == 0x9E) { if (keys[V[x] & 0xF]) PC += 2; }
        else if (nn == 0xA1) { if (!keys[V[x] & 0xF]) PC += 2; }
        break;
      case 0xF000:
        switch (nn) {
          case 0x07: V[x] = DT; break;
          case 0x0A: waitingForKey = true; waitReg = x; break;
          case 0x15: DT = V[x]; break;
          case 0x18: ST = V[x]; break;
          case 0x1E: I += V[x]; break;
          case 0x29: I = (V[x] & 0xF) * 5; break;
          case 0x33: mem[I] = V[x]/100; mem[I+1] = (V[x]/10)%10; mem[I+2] = V[x]%10; break;
          case 0x55: for (int i=0; i<=x; i++) mem[I+i] = V[i]; break;
          case 0x65: for (int i=0; i<=x; i++) V[i] = mem[I+i]; break;
        }
        break;
    }
  }
  
  void render() {
    oled.clearBuffer();
    for (int y = 0; y < 32; y++)
      for (int x = 0; x < 64; x++)
        if (gfx[y*64 + x]) oled.drawBox(x*2, y*2, 2, 2);
    oled.sendBuffer();
  }
  
  void run(const String& romPath) {
    init();
    if (!loadRom(romPath)) {
      showScreen("CHIP-8", "ROM not found", romPath.c_str());
      waitAnyKey();
      return;
    }
    waitRelease();
    playMusicTheme(TECH_MENU, OLD_MENU, CHILL_MENU, DARK_MENU, TECH_MENU, 160);
    lastCycle = lastTimer = millis();
    
    while (true) {
      pollButtons();
      if (evLong[BI_BACK]) return;
      
      memset(keys, 0, 16);
      if (held(BI_UP))    keys[2] = 1;
      if (held(BI_DOWN))  keys[8] = 1;
      if (held(BI_LEFT))  keys[4] = 1;
      if (held(BI_RIGHT)) keys[6] = 1;
      if (held(BI_OK))    keys[5] = 1;
      
      if (waitingForKey) {
        for (int k = 0; k < 16; k++) {
          if (keys[k]) { V[waitReg] = k; waitingForKey = false; break; }
        }
        render();
        frameWait(millis());
        continue;
      }
      
      uint32_t now = millis();
      while (now - lastCycle >= 2) {
        cycle();
        lastCycle += 2;
      }
      if (now - lastTimer >= 16) {
        lastTimer = now;
        if (DT > 0) DT--;
        if (ST > 0) { ST--; SFX(SF_CLICK); }
      }
      render();
      frameWait(millis());
    }
  }
}

void runDynChip8(JsonDocument& doc) {
  String rom = doc["params"]["rom"] | "";
  if (!rom.length()) { showScreen("CHIP-8", "No rom in config", ""); waitAnyKey(); return; }
  chip8::run(rom);
}
void runDynamicApp(int idx) {
  waitRelease();
  if (idx < 0 || idx >= mbAppCount) return;
  String path = mbApps[idx].path;
  File f = SD.open(path, FILE_READ);
  if (!f) { showScreen("Error", "Cannot open file", path.c_str()); waitAnyKey(); return; }
  
  String body;
  while (f.available()) body += (char)f.read();
  f.close();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) { showScreen("Error", "Bad JSON", err.c_str()); waitAnyKey(); return; }
  
  String engine = doc["engine"] | "unknown";

  if (engine == "snake")       runDynSnake(doc);
  else if (engine == "runner") runDynRunner(doc);
  else if (engine == "flappy") runDynFlappy(doc);
  else if (engine == "breakout") runDynBreakout(doc);
  else if (engine == "maze")   runDynMaze(doc);
  else if (engine == "jumper") runDynJumper(doc);
  else if (engine == "invaders") runDynInvaders(doc);
  else if (engine == "pong")   runDynPong(doc);
  else if (engine == "calculator") runDynCalculator(doc);
  else if (engine == "app") runDynMiniApp(doc);
  else if (engine == "text") runDynText(doc);
  else if (engine == "quiz") runDynQuiz(doc);
  else if (engine == "todo") runDynTodo(doc);
  else if (engine == "gemini") runDynGemini(doc);
  else if (engine == "chip8") runDynChip8(doc);
  else {
    showScreen("Unknown engine", engine.c_str(), "Supported:", "snake/runner/flappy/breakout/maze/jumper/invaders/pong");
    waitAnyKey();
  }
}
void launchApp(int id) {
  if (id == APP_FILE_MANAGER) { runLocalFileManager(); return; }
  appSettingsRequested = false;
  activeAppId = id;
  audioStop();
  if (id >= DYNAMIC_APP_ID_BASE) { runDynamicApp(id - DYNAMIC_APP_ID_BASE); return; }
  if (id != APP_SETTINGS) bumpPlays(id);
  // Legacy games read cfg.difficulty; temporarily feed them this app's own value.
  uint8_t savedGlobalDifficulty = cfg.difficulty;
  if (id != APP_SETTINGS && id >= 0 && id < APP_COUNT) cfg.difficulty = appDifficulty[id];
  switch (id) {
    case APP_DINO:        runDino(); break;
    case APP_FLAPPY:      runFlappy(); break;
    case APP_TETRIS:      runTetris(); break;
    case APP_SNAKE:       runSnake(); break;
    case APP_PONG:        runPong(); break;
    case APP_BRICK:       runBreak(); break;
    case APP_INVADE:      runInvade(); break;
    case APP_ASTER:       runAster(); break;
    case APP_MINES:       runMines(); break;
    case APP_2048:        run2048(); break;
    case APP_SIMON:       runSimon(); break;
    case APP_FROGGER:     runFrogger(); break;
    case APP_TTT:         runTTT(); break;
    case APP_GEMINI:      runGemini(); break;
    case APP_SNAKE_MAZE:  runSnakeMaze(); break;
    case APP_MEMORY:      runMemory(); break;
    case APP_SKATE:       runSkater(); break;
    case APP_TANK:        runTank(); break;
    case APP_STARS:       runStars(); break;
    case APP_REACTION:    runReaction(); break;
    case APP_LABYRINTH:   runLabyrinth(); break;
    case APP_JUMPER:      runJumper(); break;
    case APP_SETTINGS:    runSettings(); break;
    case APP_PIXEL_QUEST: runPixelQuest(); break;
    case APP_NOTES:       runNotes(); break;
  }
  audioStop();
  cfg.difficulty = savedGlobalDifficulty;
  if (appSettingsRequested && id != APP_GEMINI) {
    appSettingsRequested = false;
    runAppOptions(id);
  }
  activeAppId = -1;
  waitRelease();
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,150);
}

void runMenu() {
  menuSel = constrain(menuSel, 0, menuAppCount - 1);
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,150);

  bool okWasDown = false;
  uint32_t okDownAt = 0;
  bool okLongFired = false;

  while (true) {
    uint32_t t0 = millis();
    serviceFileWeb();
    pollButtons();

    // Short press on physical button 6 cycles language; hold exits menu.
    if (evTap[BI_BACK]) {
      cfg.lang = (cfg.lang + 1) % 3;
      saveCfg();
      if (cfg.sfx) SFX(SF_CLICK);
    }
if (evLong[BI_BACK]) { 
  if (menuSel==APP_SETTINGS) runSettings(); 
  else if (menuSel==APP_FILE_MANAGER) runFmSettings(); 
  else runAppOptions(menuSel); 
  waitRelease(); continue; 
}

    bool prevPressed = evDown[BI_LEFT] || evRep[BI_LEFT] || evDown[BI_UP] || evRep[BI_UP];
    bool nextPressed = evDown[BI_RIGHT] || evRep[BI_RIGHT] || evDown[BI_DOWN] || evRep[BI_DOWN];
    if (prevPressed) { menuSel = (menuSel + menuAppCount - 1) % menuAppCount; SFX(SF_MOVE); }
    if (nextPressed) { menuSel = (menuSel + 1) % menuAppCount; SFX(SF_MOVE); }

    // The joystick SW button is a normal short OK press; only the physical OK key
    // can invoke the long-press help page, preventing a joystick click from opening docs.
bool okDown = bst[BI_OK].stable; // Теперь учитывается и физическая, и веб-кнопка
    if (joystickEnabled && joySwitchStable && !okWasDown) { /* joystick press is handled on its debounced release below */ }
    if (okDown && !okWasDown) {
      okDownAt = millis();
      okLongFired = false;
    }
    if (okDown && !okLongFired && (millis() - okDownAt > 2500)) {
      okLongFired = true;
      bst[BI_OK].longDone = true; // подавить evTap
      if (menuSel == APP_FILE_MANAGER) { showScreen("File Manager", "OK: open", "Phone: MINI-BOY-FILES", "192.168.4.1"); waitAnyKey(); } else showGameDocs(menuSel);
    }
    if (joyOkTapEvent || (!okDown && okWasDown && !okLongFired)) {
      launchApp(menuSel);
      waitRelease();
      // После игры пользователь может всё ещё держать OK — избегаем повторного запуска
      okWasDown = false;
      continue;
    }
    okWasDown = okDown;

    drawMenu((float)menuSel, (int)(millis()/FRAME_MS));
    frameWait(t0);
  }
}

// =====================================================
//  BOOT
// =====================================================
void showBoot() {
  suppressSystemOverlay = true;
  // Optional 128x64 monochrome XBM bytes stored as exactly 1024 raw bytes at /bootlogo.raw.
  // XBM bit order is LSB-first, 16 bytes per row. Invalid/missing files fall back to the built-in animation.
  if (bootLogoFromSD && sdReady) {
    File logo = SD.open("/bootlogo.raw", FILE_READ);
    if (logo && !logo.isDirectory() && logo.size() == 1024) {
      static uint8_t bitmap[1024];
      size_t got = logo.read(bitmap, sizeof(bitmap));
      logo.close();
      if (got == sizeof(bitmap)) {
        oled.clearBuffer();
        oled.drawXBMP(0, 0, 128, 64, bitmap);
        present();
        delay(1200);
        suppressSystemOverlay = false;
        return;
      }
    } else if (logo) logo.close();
  }
  SFX(SF_WIN);
  for (int f=0;f<40;f++) {
    uint32_t t0 = millis();
    oled.clearBuffer();
    float k = min(1.0f, f/20.0f);
    drawSparkle(64, 26, 3+(int)(k*10));
    if (f>20) {
      oled.setFont(u8g2_font_6x13_t_cyrillic);
      oled.drawStr(64-oled.getStrWidth("MINI BOY v5.1")/2, 56, "MINI BOY v5.1");
    }
    present();
    frameWait(t0);
  }
  suppressSystemOverlay = false;
}

void calibrateJoystickWizard(bool force) {
  if (!joystickEnabled) { oled.clearBuffer(); oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(8,24,"Enable joystick first"); present(); delay(900); return; }
  bool done = prefs.getBool("joycal_v2", false);
  if (done && !force) return;

  int oldCx=joyCenterX, oldCy=joyCenterY;
  int oldMinX=joyMinX, oldMaxX=joyMaxX, oldMinY=joyMinY, oldMaxY=joyMaxY;
  
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x13_t_cyrillic);
  oled.drawStr(8,13,"KALIBRUVANNIA");
  oled.drawHLine(0,17,128);
  oled.setFont(u8g2_font_5x8_t_cyrillic);
  oled.drawStr(5,29,"1. Ne chypai rucku");
  oled.drawStr(5,39,"Zalysh po centru 3 sek");
  
  for (int f=0; f<36; ++f) {
    uint32_t t0=millis();
    oled.setDrawColor(0); oled.drawBox(8,47,112,10); oled.setDrawColor(1);
    oled.drawFrame(8,48,112,8); oled.drawBox(10,50,(f*108)/35,4);
    oled.drawStr(19,63,"Ne rukhai dzhoystyk"); present(); frameWait(t0);
  }
  
  long sx=0, sy=0; int minCenterX=4095, maxCenterX=0, minCenterY=4095, maxCenterY=0;
  for(int i=0;i<160;i++){
    int x=analogRead(JOY_X_PIN), y=analogRead(JOY_Y_PIN);
    sx+=x; sy+=y; minCenterX=min(minCenterX,x); maxCenterX=max(maxCenterX,x);
    minCenterY=min(minCenterY,y); maxCenterY=max(maxCenterY,y); delay(5);
  }
  int cx=sx/160, cy=sy/160;
  bool centerStable=(maxCenterX-minCenterX)<260 && (maxCenterY-minCenterY)<260;
  
  if(!(cx>250 && cx<3845 && cy>250 && cy<3845 && centerStable)) {
    oled.clearBuffer(); oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(8,22,"CENTER ERROR");
    oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(5,36,"Stick moved or signal noisy"); oled.drawStr(5,48,"Check wires, then reboot"); present(); delay(1800); return;
  }

  int newMinX=4095, newMaxX=0, newMinY=4095, newMaxY=0;
  oled.clearBuffer(); oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(8,13,"RUHAY DZHОYSTYK"); oled.drawHLine(0,17,128);
  oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(5,29,"2. Povilno do vsikh kraiv"); oled.drawStr(5,39,"Livo, pravo, vhoru, vnyz");
  oled.drawCircle(64,51,9); oled.drawCircle(64,51,15); oled.drawDisc(64,51,2); present();
  
  uint32_t start=millis(), lastDraw=0;
  while(millis()-start<7000){
    int x=analogRead(JOY_X_PIN), y=analogRead(JOY_Y_PIN);
    newMinX=min(newMinX,x); newMaxX=max(newMaxX,x); newMinY=min(newMinY,y); newMaxY=max(newMaxY,y);
    if(millis()-lastDraw>100){ 
      lastDraw=millis(); oled.clearBuffer(); oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(8,13,"RUHAY DZHОYSTYK"); oled.drawHLine(0,17,128); 
      oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(5,28,"Circle the outer limits"); oled.drawFrame(10,34,108,8); 
      int pct=(millis()-start)*104/7000; oled.drawBox(12,36,pct,4); 
      oled.drawStr(5,53,(String("X ")+newMinX+".."+newMaxX).c_str()); oled.drawStr(5,63,(String("Y ")+newMinY+".."+newMaxY).c_str()); present(); 
    }
    delay(8);
  }
  
  bool validX=(newMaxX-newMinX)>1200, validY=(newMaxY-newMinY)>1200;
  if(!validX || !validY) {
    oled.clearBuffer(); oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(8,22,"NE ZBERIHALOS");
    oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(5,36,"Move stick to all 4 edges"); oled.drawStr(5,48,"Calibration NOT saved"); present(); delay(1800);
    joyCenterX=oldCx; joyCenterY=oldCy; joyMinX=oldMinX; joyMaxX=oldMaxX; joyMinY=oldMinY; joyMaxY=oldMaxY;
    joyFilteredX=joyCenterX; joyFilteredY=joyCenterY; return;
  }
  
  joyMinX=newMinX; joyMaxX=newMaxX; joyMinY=newMinY; joyMaxY=newMaxY;
  joyCenterX=constrain(cx, joyMinX+150, joyMaxX-150); 
  joyCenterY=constrain(cy, joyMinY+150, joyMaxY-150);
  joyFilteredX=joyCenterX; joyFilteredY=joyCenterY;
  
  prefs.putInt("jcx",joyCenterX); prefs.putInt("jcy",joyCenterY); 
  prefs.putInt("jminx",joyMinX); prefs.putInt("jmaxx",joyMaxX); 
  prefs.putInt("jminy",joyMinY); prefs.putInt("jmaxy",joyMaxY); 
  prefs.putBool("joycal",true); prefs.putBool("joycal_v2",true);
  
  oled.clearBuffer(); oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(25,19,"ZBEREZHENO!"); 
  oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(6,32,"Kalibrovku zberezheno"); 
  oled.drawStr(6,43,(String("Center: ")+joyCenterX+","+joyCenterY).c_str()); 
  oled.drawStr(6,54,"Natysny fizychnu knopku"); oled.drawStr(6,63,"shchob prodovzhyty"); present();
  delay(1800);
}

  
void showFileManagerSplash() {
  oled.clearBuffer();
  // Tiny monochrome folder + SD card mark, drawn natively for the 128x64 OLED.
  oled.drawRFrame(6, 4, 116, 56, 4);
  oled.drawBox(16, 16, 27, 6);
  oled.drawRBox(13, 20, 40, 25, 3);
  oled.setDrawColor(0); oled.drawBox(17, 25, 32, 15); oled.setDrawColor(1);
  oled.drawFrame(26, 26, 13, 13); oled.drawVLine(30, 28, 9); oled.drawVLine(34, 28, 9);
  oled.drawFrame(72, 14, 22, 31); oled.drawHLine(77, 19, 12); oled.drawHLine(77, 23, 12);
  oled.drawHLine(77, 27, 8); oled.drawBox(79, 35, 8, 5);
  oled.setFont(u8g2_font_6x13_t_cyrillic); oled.drawStr(47, 17, "MINI-BOY");
  oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(47, 29, "FILE MANAGER");
  oled.drawStr(47, 40, "SD SERVICE");
  oled.drawHLine(14, 49, 100);
  oled.setFont(u8g2_font_5x8_t_cyrillic); oled.drawStr(14, 57, "192.168.4.1");
  present();
}
void scanMbApps() {
  mbAppCount = 0;
  if (!sdReady) return;
  if (!SD.exists(MB_APPS_DIR)) SD.mkdir(MB_APPS_DIR);
  File dir = SD.open(MB_APPS_DIR);
  if (!dir) return;
  File f = dir.openNextFile();
  while (f && mbAppCount < 16) {
    String nm = String(f.name());
    String low = nm; low.toLowerCase();
    if (!f.isDirectory() && low.endsWith(".mbg")) {
      String full = nm.startsWith("/") ? nm : String(MB_APPS_DIR) + "/" + nm;
      String body;
      while (f.available() && body.length() < 4000) body += (char)f.read();
      
      String name = "?";
      String engine = "?";
      uint8_t iop = 0;
      
      JsonDocument d;
      if (!deserializeJson(d, body)) {
        name = String((const char*)(d["name"] | "?"));
        engine = String((const char*)(d["engine"] | "?"));
        
        JsonArray ic = d["icon"];
        for (JsonArray p : ic) {
          if (iop >= 12) break;
          const char* op = p[0] | "box";
          int a = p[1] | 0;
          int b = p[2] | 0;
          int c = p[3] | 4;
          int e = p[4] | 4;
          uint8_t opc = 0;
          if (!strcmp(op, "frame"))      opc = 1;
          else if (!strcmp(op, "line"))  opc = 2;
          else if (!strcmp(op, "circle"))opc = 3;
          else if (!strcmp(op, "disc"))  opc = 4;
          else if (!strcmp(op, "pixel")) opc = 5;
          mbApps[mbAppCount].icon[iop].op = opc;
          mbApps[mbAppCount].icon[iop].p[0] = (int8_t)a;
          mbApps[mbAppCount].icon[iop].p[1] = (int8_t)b;
          mbApps[mbAppCount].icon[iop].p[2] = (int8_t)c;
          mbApps[mbAppCount].icon[iop].p[3] = (int8_t)e;
          iop++;
        }
      }
      mbApps[mbAppCount].slug = nm;
      mbApps[mbAppCount].name = name;
      mbApps[mbAppCount].engine = engine;
      mbApps[mbAppCount].path = full;
      mbApps[mbAppCount].ok = true;
      mbApps[mbAppCount].iconOpCount = iop;
      mbAppCount++;
    }
    f.close();
    f = dir.openNextFile();
  }
  if (f) f.close();
  dir.close();
  menuAppCount = APP_COUNT + 1 + mbAppCount;
}

void setup() {
#if ENABLE_SERIAL
  Serial.begin(115200);
#endif
  for (int i=0;i<BI_COUNT;i++) pinMode(BTN_PIN[i], INPUT_PULLUP);
  analogReadResolution(12);
#if CHARGE_STATUS_PIN >= 0
  pinMode(CHARGE_STATUS_PIN, CHARGE_ACTIVE_LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
#endif
  ledcAttach(BUZ_PIN, 2000, 8);
  ledcWrite(BUZ_PIN, 0);
  Wire.begin(OLED_SDA, OLED_SCL);
  oled.begin();
  oled.setBusClock(400000);
  randomSeed(micros());
  prefs.begin("miniboy", false);
  setenv("TZ","CET-1CEST,M3.5.0/2,M10.5.0/3",1); tzset();
  loadCfg();
  loadButtonGpio();
  bootLogoFromSD = prefs.getBool("bootlogo_sd", false);
  // Load persisted joystick center/extrema from NVS; first-run wizard calibrates if enabled.
  joyFilteredX = joyCenterX; joyFilteredY = joyCenterY;
  Wire.beginTransmission(RTC_ADDR);
  rtcPresent = (Wire.endTransmission() == 0);
  if (rtcPresent && time(nullptr) < 1700000000) rtcSetSystemClock();
  int64_t savedClock = prefs.getLong64("manualtime", 0);
  if (savedClock > 1700000000LL && time(nullptr) < 1700000000) {
    struct timeval savedTv;
    savedTv.tv_sec = (time_t)savedClock;
    savedTv.tv_usec = 0;
    settimeofday(&savedTv, nullptr);
  }
  applyDisplay();
  ttW = prefs.getInt("ttW", 0);
  ttL = prefs.getInt("ttL", 0);
  ttD = prefs.getInt("ttD", 0);
  WiFi.mode(WIFI_STA);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (cfg.autoConn && ssid.length()) WiFi.begin(ssid.c_str(), pass.c_str());
  setupFileWeb();
 scanMbApps(); 
  showBoot();
  // Joystick removed; do not run joystick calibration wizard.
  playMusicTheme(TECH_MENU,OLD_MENU,CHILL_MENU,DARK_MENU,TECH_MENU,150);
}

void loop() {
  serviceFileWeb();
  runMenu();
  serviceFileWeb();
}


/*
 * MINI BOY - controls and game tuning notes
 * -------------------------------------------------------------
 * PHYSICAL INPUT
 *   Buttons (active LOW): LEFT=GPIO3, RIGHT=GPIO1, UP=GPIO5, DOWN=GPIO7, OK=GPIO2, BACK=GPIO6.
 *   SD SPI candidate mapping: CS=GPIO4, SCK=GPIO0, MOSI=GPIO20, MISO=GPIO21.
 *   Verify GPIO0 boot behavior and exact board pinout before wiring.
 *   OLED SDA/SCL are GPIO8/GPIO9 after the user's pin reassignment.
 *   DS3231 shares the OLED I2C bus on GPIO8/GPIO9.
 *
 * CONTROL MODEL
 *   The joystick does not replace buttons. Its directions are merged into
 *   the same debounced input events as the buttons, so old games continue to
 *   use one input interface. Analog values are sampled once per frame and
 *   filtered by the dead zone to reduce drift. If a joystick direction is
 *   reversed on a particular module, swap the axis wiring or reverse that
 *   comparison in pollButtons(); no game loop should need to be changed.
 *
 * PER-APP SETTINGS
 *   Each app stores its own difficulty under gd00..gdNN in Preferences.
 *   The long press of the sixth physical button (2.5 seconds) opens the
 *   selected app's options from the menu. During a running app, the same hold
 *   requests options after the app exits its loop. Gemini is deliberately
 *   excluded: the long press exits Gemini instead of opening its options.
 *   Dino-specific values are stored as dpt (pterodactyl enable) and dns
 *   (night transition score). Flappy's pipe gap is stored as fgap.
 *
 * FAIRNESS RULES
 *   The Dino pterodactyl is not spawned before the selected night threshold.
 *   After a flying obstacle, the next spawn uses a longer distance interval.
 *   Snake Maze reserves a clean initial corridor, uses a food target, allows
 *   entry into the departing tail cell when not growing, and does not start
 *   by pointing into a generated wall. Jumper's EASY mode lowers gravity and
 *   increases the bounce impulse.
 *
 * DISPLAY RULES
 *   System status is drawn only when no app is active, so the clock cannot
 *   cover the score strip in Flappy, Dino, or other games. Game HUDs should
 *   reserve their own rows and avoid drawing system text over gameplay.
 *
 * TROUBLESHOOTING
 *   - If the OLED is blank after moving I2C, verify SDA=8 and SCL=9 in code.
 *   - If the RTC is absent, check its address (normally 0x68), VCC/GND and
 *     shared SDA/SCL wiring. The RTC's square-wave pin is not an I2C signal.
 *   - If the joystick moves without touching it, recalibrate the center or
 *     increase JOY_DEADZONE. Typical ADC readings are near 2048 at center.
 *   - If SW conflicts with USB serial on a board variant, leave SW disconnected;
 *     directions still work. Confirm GPIO20 is exposed on the exact board.
 *   - Do not power analog joystick outputs from 5 V on ESP32-C3 inputs.
 *
 * Future game-specific settings should use the same pattern: define a bounded
 * default, load it from Preferences, expose it in runAppOptions(), persist it,
 * and read it inside the relevant game. Av oid adding unused menu labels.
 */