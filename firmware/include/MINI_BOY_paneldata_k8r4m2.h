#pragma once
const char FILE_PANEL[] PROGMEM = R"MINIHTML(<!doctype html>
<html lang="ru">
  <head>
    <meta charset="utf-8" />
    <meta
      name="viewport"
      content="width=device-width,initial-scale=1,viewport-fit=cover"
    />
    <meta name="theme-color" content="#101827" />
    <title>MINI-BOY Control</title>
    <style>
      :root {
        color-scheme: dark;
        --bg: #0b1220;
        --panel: #141f31;
        --line: #2a3b54;
        --text: #edf4ff;
        --muted: #9cb0cc;
        --mint: #6de5c1;
        --blue: #7ba7ff;
        --red: #ff8192;
      }
      * {
        box-sizing: border-box;
      }
      body {
        margin: 0;
        background: radial-gradient(ellipse at top, #1b2a43 0, #0b1220 58%);
        color: var(--text);
        font:
          15px/1.45 system-ui,
          -apple-system,
          Segoe UI,
          sans-serif;
        min-height: 100vh;
      }
      .wrap {
        max-width: 1000px;
        margin: auto;
        padding: 18px 12px 32px;
      }
      .head {
        display: flex;
        align-items: center;
        gap: 12px;
        margin: 3px 0 16px;
      }
      .brand {
        width: 48px;
        height: 48px;
        border-radius: 15px;
        background: linear-gradient(145deg, var(--mint), var(--blue));
        display: grid;
        place-items: center;
        color: #091321;
        font-size: 25px;
        font-weight: 800;
      }
      .head h1 {
        font-size: 22px;
        margin: 0;
      }
      .sub {
        font-size: 12px;
        color: var(--muted);
      }
      .status {
        border: 1px solid var(--line);
        background: #121d2e;
        border-radius: 12px;
        padding: 10px 12px;
        margin-bottom: 12px;
        color: var(--muted);
        overflow-wrap: anywhere;
      }
      .tabs {
        display: flex;
        gap: 7px;
        flex-wrap: wrap;
        margin: 0 0 14px;
      }
      .tab,
      button {
        font: inherit;
        border: 1px solid var(--line);
        border-radius: 10px;
        background: #1b2940;
        color: var(--text);
        padding: 9px 12px;
        min-height: 40px;
        cursor: pointer;
      }
      .tab.active,
      .primary {
        background: var(--mint);
        border-color: var(--mint);
        color: #082018;
        font-weight: 700;
      }
      .pane {
        display: none;
      }
      .pane.active {
        display: block;
      }
      .card {
        background: linear-gradient(145deg, #162338, #111c2d);
        border: 1px solid var(--line);
        border-radius: 15px;
        padding: 14px;
        margin-bottom: 12px;
      }
      .card h2 {
        font-size: 16px;
        margin: 0 0 10px;
      }
      .muted {
        color: var(--muted);
        font-size: 12px;
      }
      .dash {
        display: grid;
        grid-template-columns: minmax(0, 1fr) minmax(230px, 0.75fr);
        gap: 12px;
      }
      .screenbox {
        background: #05090f;
        border: 1px solid #354963;
        border-radius: 12px;
        padding: 10px;
        display: grid;
        place-items: center;
        min-height: 170px;
      }
      .screenbox canvas {
        width: min(100%, 512px);
        height: auto;
        image-rendering: pixelated;
        background: #000;
        border: 1px solid #26364b;
      }
      .pad {
        display: grid;
        grid-template-columns: repeat(3, minmax(52px, 1fr));
        gap: 7px;
        max-width: 270px;
        margin: auto;
      }
      .pad button {
        font-size: 20px;
        font-weight: 650;
        min-height: 48px;
      }
      .pad .ok {
        background: var(--mint);
        color: #092018;
        border-color: var(--mint);
      }
      .pad .back {
        color: var(--red);
      }
      .toolbar {
        display: flex;
        gap: 7px;
        flex-wrap: wrap;
        margin-bottom: 10px;
      }
      .crumb {
        padding: 9px 11px;
        background: #0e1929;
        border: 1px solid var(--line);
        border-radius: 10px;
        margin-bottom: 10px;
        overflow-wrap: anywhere;
        color: var(--mint);
      }
      .upload {
        display: flex;
        gap: 8px;
        flex-wrap: wrap;
        align-items: center;
        background: #111c2d;
        border: 1px solid var(--line);
        padding: 11px;
        border-radius: 12px;
        margin-bottom: 11px;
      }
      .upload input,
      .field input {
        font: inherit;
        background: #0c1727;
        color: var(--text);
        border: 1px solid var(--line);
        border-radius: 9px;
        padding: 9px;
        min-width: 0;
        max-width: 100%;
      }
      .upload input[type="file"] {
        flex: 1;
        min-width: 170px;
      }
      .files {
        display: grid;
        gap: 8px;
      }
      .item {
        display: flex;
        align-items: center;
        gap: 10px;
        padding: 10px;
        background: #121e30;
        border: 1px solid var(--line);
        border-radius: 12px;
        min-width: 0;
      }
      .glyph {
        width: 39px;
        height: 39px;
        flex: 0 0 39px;
        display: grid;
        place-items: center;
        background: #24344d;
        border-radius: 11px;
        font-size: 20px;
      }
      .meta {
        flex: 1;
        min-width: 0;
      }
      .name {
        font-weight: 650;
        overflow-wrap: anywhere;
      }
      .size {
        font-size: 12px;
        color: var(--muted);
        margin-top: 2px;
      }
      .actions {
        display: flex;
        gap: 5px;
        flex-wrap: wrap;
        justify-content: flex-end;
      }
      .actions button {
        padding: 7px 9px;
        min-height: 34px;
        font-size: 13px;
      }
      .fields {
        display: grid;
        grid-template-columns: repeat(2, minmax(0, 1fr));
        gap: 10px;
      }
      .field {
        display: grid;
        gap: 5px;
        color: var(--muted);
        font-size: 13px;
      }
      .field input {
        width: 100%;
      }
      .empty {
        text-align: center;
        padding: 28px 10px;
        color: var(--muted);
        border: 1px dashed var(--line);
        border-radius: 12px;
      }
      .danger {
        color: var(--red);
      }
      .preview {
        display: none;
      }
      .preview.open {
        display: block;
      }
      .preview canvas {
        width: min(100%, 512px);
        height: auto;
        image-rendering: pixelated;
        background: #000;
        border: 1px solid var(--line);
      }
      .foot {
        text-align: center;
        color: var(--muted);
        font-size: 12px;
        margin-top: 18px;
      }
      @media (max-width: 650px) {
        .wrap {
          padding: 12px 9px 24px;
        }
        .head h1 {
          font-size: 19px;
        }
        .dash {
          grid-template-columns: 1fr;
        }
        .fields {
          grid-template-columns: 1fr;
        }
        .item {
          align-items: flex-start;
        }
        .actions {
          max-width: 118px;
        }
        .actions button {
          padding: 6px 7px;
        }
        .card {
          padding: 11px;
        }
      }
    </style>
  </head>
  <body>
    <main class="wrap">
      <header class="head">
        <div class="brand">M</div>
        <div>
          <h1>MINI-BOY <span style="color: var(--mint)">Control</span></h1>
          <div class="sub">Локальная панель устройства · интернет не нужен</div>
        </div>
      </header>
      <div id="status" class="status">Подключение к MINI-BOY…</div>
      <nav class="tabs">
        <button class="tab active" data-tab="dashboard">
          ▣ Экран и управление</button
        ><button class="tab" data-tab="files">▤ Файлы SD</button
        ><button class="tab" data-tab="settings">⚙ Устройство</button>
      </nav>
      <section id="dashboard" class="pane active">
        <div class="dash">
          <article class="card">
            <h2>Что сейчас на дисплее</h2>
            <div class="screenbox">
              <canvas id="screen" width="128" height="64"></canvas>
            </div>
            <div class="toolbar" style="margin-top: 10px">
              <button onclick="refreshScreen()">↻ Обновить экран</button
              ><label class="muted" style="align-self: center"
                ><input id="live" type="checkbox" checked /> Live · 0,5 с</label
              >
            </div>
            <div class="muted">Отображается текущий буфер OLED 128×64.</div>
          </article>
          <article class="card">
            <h2>Удалённые кнопки</h2>
<div class="pad">
  <button onclick="cycleLang()" style="font-size:14px; color:var(--mint)">🌐</button>
  <button data-key="up">↑</button>
  <span></span>
  <button data-key="left">←</button>
  <button data-key="ok" class="ok">OK</button>
  <button data-key="right">→</button>
  <span></span>
  <button data-key="down">↓</button>
  <button data-key="back" class="back">BACK</button>
</div>
            <p class="muted">
              Нажатия передаются в меню и приложения так же, как физические
              кнопки. Удержание кнопки браузером не эмулируется.
            </p>
            <div class="toolbar">
              <button onclick="refreshDevice()">Проверить устройство</button>
            </div>
          </article>
        </div>
      </section>
      <section id="files" class="pane">
        <article class="card">
          <h2>SD-карта</h2>
          <div id="crumb" class="crumb">/</div>
          <div id="fileCount" class="muted" style="margin: 0 0 8px">
            Объектов: —
          </div>
          <div class="toolbar">
            <button onclick="up()">↰ Вверх</button
            ><button onclick="refreshFiles()">↻ Обновить</button>
          </div>
          <div class="upload">
            <input id="upload" type="file" /><button
              class="primary"
              onclick="sendFile()"
            >
              Загрузить
            </button>
          </div>
          <div class="upload">
            <input
              id="newFolderName"
              maxlength="80"
              placeholder="Имя новой папки"
            /><button onclick="mk()">＋ Создать папку</button>
          </div>
          <section id="actionsPanel" class="card" style="display: none">
            <h2 id="selectedTitle">Выбранный объект</h2>
            <div class="fields">
              <label class="field"
                >Новое имя<input
                  id="renameTo"
                  maxlength="80"
                  placeholder="Новое имя" /></label
              ><label class="field"
                >Путь для копирования / перемещения<input
                  id="targetPath"
                  placeholder="/folder/name.copy"
              /></label>
            </div>
            <div class="toolbar" style="margin-top: 10px">
              <button onclick="renameSelected()">Переименовать</button
              ><button onclick="copySelected()">Копировать</button
              ><button onclick="moveSelected()">Переместить</button
              ><button onclick="previewSelected()">Просмотр</button
              ><button onclick="downloadSelected()">Скачать</button
              ><button
                id="setLogoBtn"
                style="display: none"
                class="primary"
                onclick="setBootLogo()"
              >
                Использовать как заставку</button
              ><button onclick="deleteSelected()" class="danger">
                Удалить
              </button>
            </div>
            <div id="logoHint" class="muted"></div>
          </section>
          <section id="filesList" class="files"></section>
          <section id="preview" class="card preview">
            <div class="toolbar">
              <strong id="previewTitle">Просмотр</strong
              ><button onclick="closePreview()">Закрыть</button>
            </div>
            <canvas id="rawPreview" width="128" height="64"></canvas>
            <pre
              id="textPreview"
              style="
                white-space: pre-wrap;
                overflow-wrap: anywhere;
                max-height: 50vh;
                overflow: auto;
              "
            ></pre>
            <img
              id="imagePreview"
              style="display: none; max-width: 100%; height: auto"
              alt="Предпросмотр изображения"
            />
          </section>
        </article>
      </section>
      <section id="settings" class="pane">
        <article class="card">
          <h2>Все настройки MINI-BOY</h2>
          <p class="muted">
            Здесь доступны переключатели из меню Settings. Изменения сохраняются
            в памяти устройства.
          </p>
          <div class="fields">
            <label class="field"
              >Яркость OLED
              <select id="brightness" onchange="saveSettings()">
                <option value="0">1 · низкая</option>
                <option value="1">2 · средняя</option>
                <option value="2">3 · высокая</option>
                <option value="3">4 · максимум</option>
              </select></label
            >
            <label class="field"
              >Инверсия дисплея
              <select id="invert" onchange="saveSettings()">
                <option value="0">Выключена</option>
                <option value="1">Включена</option>
              </select></label
            >
            <label class="field"
              >Звуковые эффекты
              <select id="sound" onchange="saveSettings()">
                <option value="0">Выключены</option>
                <option value="1">Включены</option>
              </select></label
            >
            <label class="field"
              >Показывать пароль Wi-Fi
              <select id="showPass" onchange="saveSettings()">
                <option value="0">Нет</option>
                <option value="1">Да</option>
              </select></label
            >
            <label class="field"
              >Раскладка
              <select id="layout" onchange="saveSettings()">
                <option value="0">EN</option>
                <option value="1">RU</option>
                <option value="2">UA</option>
              </select></label
            >
            <label class="field"
              >Скорость кнопок
              <select id="rep" onchange="saveSettings()">
                <option value="0">Быстро</option>
                <option value="1">Нормально</option>
                <option value="2">Медленно</option>
              </select></label
            >
            <label class="field"
              >Музыкальная тема
              <select id="theme" onchange="saveSettings()">
                <option value="0">OFF</option>
                <option value="1">TECH</option>
                <option value="2">CHILL</option>
                <option value="3">DARK</option>
                <option value="4">OLDSCHOOL</option>
              </select></label
            >
            <label class="field"
              >Громкость
              <input
                id="volume"
                type="range"
                min="0"
                max="100"
                step="5"
                onchange="saveSettings()"
            /></label>
            <label class="field"
              >Показывать FPS
              <select id="fps" onchange="saveSettings()">
                <option value="0">Нет</option>
                <option value="1">Да</option>
              </select></label
            >
            <label class="field"
              >Автоподключение Wi-Fi
              <select id="autoConn" onchange="saveSettings()">
                <option value="0">Нет</option>
                <option value="1">Да</option>
              </select></label
            >
            <label class="field"
              >Сложность игр
              <select id="difficulty" onchange="saveSettings()">
                <option value="0">Лёгкая</option>
                <option value="1">Обычная</option>
                <option value="2">Сложная</option>
              </select></label
            >
            <label class="field"
              >Скорость анимации
              <select id="animSpeed" onchange="saveSettings()">
                <option value="0">Медленно</option>
                <option value="1">Нормально</option>
                <option value="2">Быстро</option>
              </select></label
            >
            <label class="field"
              >Показывать часы
              <select id="showClock" onchange="saveSettings()">
                <option value="0">Нет</option>
                <option value="1">Да</option>
              </select></label
            >
            <label class="field"
              >Формат часов
              <select id="clock24h" onchange="saveSettings()">
                <option value="0">12 часов</option>
                <option value="1">24 часа</option>
              </select></label
            >
            <label class="field"
              >Автосинхронизация времени
              <select id="autoTime" onchange="saveSettings()">
                <option value="0">Нет</option>
                <option value="1">Да</option>
              </select></label
            >
            <label class="field">Язык интерфейса <select id="lang" onchange="saveSettings()"><option value="0">EN</option><option value="1">RU</option><option value="2">UA</option></select></label>
<label class="field">Показывать расширения файлов <select id="showFileTypes" onchange="saveSettings()"><option value="0">Нет</option><option value="1">Да</option></select></label>
<label class="field">Циклическая навигация меню <select id="settingsWrap" onchange="saveSettings()"><option value="0">Нет</option><option value="1">Да</option></select></label>
          </div>
        </article>
        <article class="card">
          <h2>Действия из меню Settings</h2>
          <p class="muted">
            Они выполняются на самом устройстве, поэтому экран MINI-BOY может
            переключиться на выбранный раздел.
          </p>
          <div class="toolbar">
            <button onclick="settingsAction('scan')">Сканировать Wi-Fi</button
            ><button onclick="settingsAction('reconnect')">
              Переподключиться</button
            ><button onclick="settingsAction('forget')">Забыть сеть</button
            ><button onclick="settingsAction('setTime')">
              Установить дату/время</button
            ><button onclick="settingsAction('syncTime')">
              Синхронизировать время</button
            ><button onclick="settingsAction('records')">Рекорды</button
            ><button onclick="settingsAction('resetScores')">
              Сбросить рекорды</button
            ><button onclick="settingsAction('systemInfo')">
              Информация о системе</button
            ><button onclick="settingsAction('about')">О MINI-BOY</button
            ><button onclick="settingsAction('sdStatus')">Статус SD</button
            ><button onclick="settingsAction('bootLogo')">
              Переключить заставку
            </button>
          </div>
        </article>
        <article class="card">
          <h2>Загрузочная заставка</h2>
          <p class="muted">
            Монохромный RAW 128×64 — ровно 1024 байта. PNG/JPG можно
            просматривать, но прошивка не декодирует их в заставку
            автоматически.
          </p>
          <div class="toolbar">
            <button onclick="chooseDefaultLogo()">Стандартная заставка</button
            ><button onclick="chooseExistingLogo()">
              Использовать /bootlogo.raw
            </button>
          </div>
          <div id="logoStatus" class="muted">Загрузка статуса…</div>
        </article>
        <article class="card">
  <h2>Обновление прошивки (OTA)</h2>
  <p class="muted danger">ВНИМАНИЕ! Не выключайте устройство во время обновления. Убедитесь, что скачали BIN-файл для ESP32-C3.</p>
  <div class="upload">
    <input id="otaFile" type="file" accept=".bin">
    <button class="primary danger" onclick="uploadOta()">⚠ Прошить</button>
  </div>
  <div id="otaStatus" class="muted"></div>
</article>
        <article class="card">
          <h2>Состояние</h2>
          <div id="deviceInfo" class="muted">Проверка…</div>
          <div class="toolbar" style="margin-top: 10px">
            <button onclick="refreshDevice()">Обновить статус</button>
          </div>
        </article>
      </section>
      <div class="foot">
        MINI-BOY · локальный Wi-Fi: MINI-BOY-FILES · адрес 192.168.4.1
      </div>
    </main>
    <script>
      let path = "/",
        selectedFile = null,
        busy = false,
        liveTimer = null;
      const $ = (id) => document.getElementById(id);
      function esc(s) {
        return String(s).replace(
          /[&<>"']/g,
          (c) =>
            ({
              "&": "&amp;",
              "<": "&lt;",
              ">": "&gt;",
              '"': "&quot;",
              "'": "&#39;",
            })[c],
        );
      }
      function bytes(n) {
        if (n < 1024) return n + " Б";
        if (n < 1048576) return (n / 1024).toFixed(1) + " КБ";
        return (n / 1048576).toFixed(1) + " МБ";
      }
      async function uploadOta() {
  let f = $('otaFile').files[0];
  if(!f || !f.name.endsWith('.bin')) { $('otaStatus').textContent = 'Выберите .bin файл'; return; }
  if(!confirm('ВНИМАНИЕ! Устройство перезагрузится. Не выключайте питание!')) return;
  
  let fd = new FormData();
  fd.append('update', f, f.name);
  $('otaStatus').textContent = 'Прошивка OTA... Пожалуйста, подождите.';
  
  let xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/ota');
  xhr.onload = () => $('otaStatus').textContent = xhr.responseText;
  xhr.onerror = () => $('otaStatus').textContent = 'Ошибка сети при OTA';
  xhr.upload.onprogress = (e) => { 
    if(e.lengthComputable) $('otaStatus').textContent = 'OTA: ' + Math.round(e.loaded*100/e.total) + '%'; 
  };
  xhr.send(fd);
}
      async function api(url, opts) {
        let r = await fetch(url, opts);
        let t = await r.text();
        if (!r.ok) throw Error(t || "HTTP " + r.status);
        return t;
      }
      function status(s) {
        $("status").textContent = s;
      }
      document.querySelectorAll("[data-tab]").forEach(
        (b) =>
          (b.onclick = () => {
            document
              .querySelectorAll(".tab")
              .forEach((x) => x.classList.toggle("active", x === b));
            document
              .querySelectorAll(".pane")
              .forEach((x) =>
                x.classList.toggle("active", x.id === b.dataset.tab),
              );
            if (b.dataset.tab === "files") refreshFiles();
            if (b.dataset.tab === "settings") refreshDevice();
          }),
      );
async function refreshScreen(){
  try{
    let r=await fetch('/api/display?ts='+Date.now());
    if(!r.ok)throw Error('HTTP '+r.status);
    let d=new Uint8Array(await r.arrayBuffer());
    if(d.length<1024)throw Error('Короткий буфер OLED');
    let c=$('screen').getContext('2d');
    let imgData=c.createImageData(128,64);
    let data=imgData.data;
    for(let i=0;i<1024;i++){
      let b=d[i];
      let x=i%128;
      let yPage=Math.floor(i/128);
      for(let bit=0;bit<8;bit++){
        let y=yPage*8+bit;
        let on=(b>>bit)&1;
        let idx=(y*128+x)*4;
        if(on){
          data[idx]=233; data[idx+1]=255; data[idx+2]=247; data[idx+3]=255;
        } else {
          data[idx]=0; data[idx+1]=0; data[idx+2]=0; data[idx+3]=255;
        }
      }
    }
    c.putImageData(imgData,0,0);
  }catch(e){status('Экран недоступен: '+e.message)}
}
      function liveLoop() {
        if (liveTimer) clearInterval(liveTimer);
        liveTimer = $("live").checked ? setInterval(refreshScreen, 166) : null;
      }
      $("live").onchange = liveLoop;
      liveLoop();
      async function keyState(k, state) {
        try {
          await api("/api/control?key=" + k + "&state=" + state);
          if (state === "up") setTimeout(refreshScreen, 100);
        } catch (e) {
          status("Ошибка управления: " + e.message);
        }
      }
document.querySelectorAll('.pad button').forEach(b => {
  let k = b.dataset.key;
  b.addEventListener('pointerdown', e => {
    e.preventDefault();
    try { b.setPointerCapture(e.pointerId); } catch(_) {}
    keyState(k, 'down'); // Отправляем "нажатие" (сработает задержка)
  });
  for(let ev of ['pointerup','pointercancel','lostpointercapture']) {
    b.addEventListener(ev, () => keyState(k, 'up')); // Отправляем "отпускание"
  }
  b.addEventListener('contextmenu', e => e.preventDefault());
});

// Функция для быстрой смены языка
async function cycleLang() {
  try {
    let s = JSON.parse(await api('/api/settings'));
    let next = (s.lang + 1) % 3;
    await api('/api/settings?lang=' + next);
    let langs = ['English', 'Русский', 'Українська'];
    status('Язык изменен на: ' + langs[next]);
    await refreshDevice();
  } catch (e) {
    status('Ошибка смены языка: ' + e.message);
  }
}
      async function refreshFiles() {
        if (busy) return;
        busy = true;
        $("crumb").textContent = path;
        try {
          let a = JSON.parse(
            await api("/api/list?path=" + encodeURIComponent(path)),
          );
          let root = $("filesList");
          root.innerHTML = "";
          status("SD-карта: доступна · объектов: " + a.length);
          if ($("fileCount"))
            $("fileCount").textContent = "Объектов в папке: " + a.length;
          if (!a.length) {
            root.innerHTML = '<div class="empty">Папка пуста</div>';
            return;
          }
          a.sort(
            (x, y) =>
              Number(y.dir) - Number(x.dir) || x.name.localeCompare(y.name),
          );
          for (let f of a) {
            let el = document.createElement("article");
            el.className = "item";
            let ext = f.name.split(".").pop().toLowerCase();
            let glyph = f.dir
              ? "▰"
              : ["png", "jpg", "jpeg", "bmp", "gif", "raw"].includes(ext)
                ? "▧"
                : ["ino", "h", "cpp", "txt", "json", "cfg"].includes(ext)
                  ? "≡"
                  : "▤";
            el.innerHTML =
              '<div class="glyph">' +
              glyph +
              '</div><div class="meta"><div class="name">' +
              esc(f.name) +
              '</div><div class="size">' +
              (f.dir ? "Папка" : bytes(f.size)) +
              '</div></div><div class="actions"><button data-open>Открыть</button><button data-select>Выбрать</button></div>';
            el.querySelector("[data-open]").onclick = () => {
              if (f.dir) {
                path = f.path;
                selectedFile = null;
                $("actionsPanel").style.display = "none";
                refreshFiles();
              } else {
                selectedFile = f;
                showActions(f);
                previewSelected();
              }
            };
            el.querySelector("[data-select]").onclick = () => {
              selectedFile = f;
              showActions(f);
            };
            root.appendChild(el);
          }
        } catch (e) {
          if ($("fileCount"))
            $("fileCount").textContent = "Количество недоступно";
          status("Ошибка чтения SD: " + e.message);
          $("filesList").innerHTML =
            '<div class="empty">' + esc(e.message) + "</div>";
        } finally {
          busy = false;
        }
      }
      function showActions(f) {
        $("actionsPanel").style.display = "block";
        $("selectedTitle").textContent =
          (f.dir ? "Папка: " : "Файл: ") + f.name;
        $("renameTo").value = f.name;
        $("targetPath").value =
          (path === "/" ? "" : path) + "/" + f.name + ".copy";
        let raw = f.name.toLowerCase().endsWith(".raw") && f.size === 1024;
        $("setLogoBtn").style.display = raw ? "inline-block" : "none";
        $("logoHint").textContent = raw
          ? "Подходит для заставки 128×64 RAW."
          : "Заставка доступна только для .raw размером 1024 байта; PNG/JPG доступны для просмотра.";
        status("Выбрано: " + f.path);
      }
      async function act(op, p, to = "") {
        try {
          status("Выполняется: " + op + "…");
          let t = await api(
            "/api/op?op=" +
              encodeURIComponent(op) +
              "&path=" +
              encodeURIComponent(p) +
              "&to=" +
              encodeURIComponent(to),
          );
          status(t);
          await refreshFiles();
        } catch (e) {
          status("Ошибка: " + e.message);
        }
      }
      function up() {
        if (path != "/") {
          path = path.slice(0, path.lastIndexOf("/")) || "/";
          selectedFile = null;
          $("actionsPanel").style.display = "none";
          refreshFiles();
        }
      }
      async function mk() {
        let n = $("newFolderName").value.trim();
        if (!n) {
          status("Введи имя папки");
          $("newFolderName").focus();
          return;
        }
        if (/[\\/]/.test(n) || n === "." || n === "..") {
          status("Недопустимое имя папки");
          return;
        }
        await act("mkdir", (path === "/" ? "" : path) + "/" + n);
        $("newFolderName").value = "";
      }
      async function renameSelected() {
        if (!selectedFile) return;
        let n = $("renameTo").value.trim();
        if (!n || /[\\/]/.test(n) || n === "." || n === "..") {
          status("Укажи допустимое имя");
          return;
        }
        await act(
          "rename",
          selectedFile.path,
          (path === "/" ? "" : path) + "/" + n,
        );
        selectedFile = null;
        $("actionsPanel").style.display = "none";
      }
      async function copySelected() {
        if (!selectedFile) return;
        let to = $("targetPath").value.trim();
        if (!to.startsWith("/")) {
          status("Путь должен начинаться с /");
          return;
        }
        await act("copy", selectedFile.path, to);
      }
      async function moveSelected() {
        if (!selectedFile) return;
        let to = $("targetPath").value.trim();
        if (!to.startsWith("/")) {
          status("Путь должен начинаться с /");
          return;
        }
        await act("move", selectedFile.path, to);
        selectedFile = null;
        $("actionsPanel").style.display = "none";
      }
      async function deleteSelected() {
        if (!selectedFile || !confirm("Удалить " + selectedFile.name + "?"))
          return;
        await act("delete", selectedFile.path);
        selectedFile = null;
        $("actionsPanel").style.display = "none";
      }
      function downloadSelected() {
        if (!selectedFile || selectedFile.dir) {
          status("Выбери файл");
          return;
        }
        location.href =
          "/api/download?path=" + encodeURIComponent(selectedFile.path);
      }
      async function previewSelected() {
        if (!selectedFile || selectedFile.dir) return;
        let f = selectedFile,
          low = f.name.toLowerCase();
        $("preview").classList.add("open");
        $("previewTitle").textContent = f.name;
        $("rawPreview").style.display = "none";
        $("imagePreview").style.display = "none";
        $("textPreview").style.display = "none";
        try {
          if (low.endsWith(".raw") && f.size === 1024) {
            let r = await fetch(
              "/api/download?path=" + encodeURIComponent(f.path),
            );
            let d = new Uint8Array(await r.arrayBuffer());
            if (d.length !== 1024) throw Error("RAW должен быть 1024 байта");
            let c = $("rawPreview");
            c.style.display = "block";
            let ctx = c.getContext("2d");
            ctx.fillStyle = "#000";
            ctx.fillRect(0, 0, 128, 64);
            ctx.fillStyle = "#fff";
            for (let y = 0; y < 64; y++)
              for (let x = 0; x < 128; x++)
                if (d[(y >> 3) * 128 + x] & (1 << (y & 7)))
                  ctx.fillRect(x, y, 1, 1);
          } else if (/\.(png|jpe?g|gif|bmp|svg)$/.test(low)) {
            let im = $("imagePreview");
            im.src = "/api/view?path=" + encodeURIComponent(f.path);
            im.style.display = "block";
          } else if (/\.(txt|ino|h|cpp|json|cfg)$/.test(low)) {
            let r = await fetch(
              "/api/preview?path=" + encodeURIComponent(f.path),
            );
            if (!r.ok) throw Error(await r.text());
            $("textPreview").textContent = await r.text();
            $("textPreview").style.display = "block";
          } else {
            status("Для этого типа файла доступно скачивание.");
          }
        } catch (e) {
          status("Предпросмотр: " + e.message);
        }
      }
      function closePreview() {
        $("preview").classList.remove("open");
      }
async function sendFile() {
  let input = $("upload");
  let f = input.files[0];

  if (!f) {
    status("❌ Сначала выбери файл");
    return;
  }

  if (!f.size || f.size === 0) {
    status("❌ Файл пустой или недоступен. Скопируй его в локальную папку телефона (не в облако)");
    return;
  }

  if (f.size > 3 * 1024 * 1024) {
    status("❌ Файл больше 3 МБ. ESP32 не может принять такой объём");
    return;
  }

  status("📖 Чтение файла " + f.name + " (" + bytes(f.size) + ")...");

  // Читаем файл в память — это решает проблему с облачными файлами
  let buffer;
  try {
    buffer = await f.arrayBuffer();
  } catch(e) {
    status("❌ Ошибка чтения: " + e.message + ". Скопируй файл в локальную папку");
    return;
  }

  if (buffer.byteLength === 0) {
    status("❌ Файл пустой");
    return;
  }

  let dest = (path === "/" ? "" : path) + "/" + f.name;
  let blob = new Blob([buffer], { type: 'application/octet-stream' });
  let fd = new FormData();
  fd.append("file", blob, f.name);

  status("⬆ Загрузка " + f.name + " (" + bytes(buffer.byteLength) + ")...");

  try {
    let res = await fetch("/api/upload?path=" + encodeURIComponent(dest), {
      method: "POST",
      body: fd
    });

    if (res.ok) {
      let txt = await res.text();
      status("✅ " + (txt || "Загрузка завершена"));
      input.value = "";
      await refreshFiles();
    } else {
      status("❌ HTTP " + res.status + ": " + await res.text());
    }
  } catch(e) {
    status("❌ Ошибка сети: " + e.message + ". Проверь, что телефон в сети MINI-BOY-FILES");
  }
}
      async function setBootLogo() {
        if (!selectedFile) return;
        try {
          status(
            await api(
              "/api/setbootlogo?path=" + encodeURIComponent(selectedFile.path),
            ),
          );
          await refreshDevice();
        } catch (e) {
          status("Заставка: " + e.message);
        }
      }
      async function chooseDefaultLogo() {
        try {
          status(await api("/api/bootlogo?mode=default"));
          await refreshDevice();
        } catch (e) {
          status(e.message);
        }
      }
      async function chooseExistingLogo() {
        try {
          status(await api("/api/bootlogo?mode=sd"));
          await refreshDevice();
        } catch (e) {
          status(e.message);
        }
      }
      async function saveSettings() {
        let ids=['brightness','invert','sound','showPass','layout','rep','theme','volume','fps','autoConn','difficulty','animSpeed','showClock','clock24h','autoTime','lang','showFileTypes','settingsWrap'];
        let q =
          "/api/settings?" +
          ids.map((id) => id + "=" + encodeURIComponent($(id).value)).join("&");
        try {
          let s = JSON.parse(await api(q));
          status("Настройки сохранены");
          $("logoStatus").textContent = "Заставка: " + s.bootlogo.toUpperCase();
        } catch (e) {
          status("Настройки: " + e.message);
        }
      }
      async function settingsAction(a) {
        try {
          status(
            await api("/api/settings/action?action=" + encodeURIComponent(a)),
          );
          setTimeout(refreshScreen, 80);
          await refreshDevice();
        } catch (e) {
          status("Действие: " + e.message);
        }
      }
      async function refreshDevice() {
        try {
          let s = JSON.parse(await api("/api/settings"));
let vals={
  brightness:s.brightness, invert:s.invert?1:0, sound:s.sound?1:0, showPass:s.showPass?1:0,
  layout:s.layout, rep:s.rep, theme:s.theme, volume:s.volume, fps:s.fps?1:0,
  autoConn:s.autoConn?1:0, difficulty:s.difficulty, animSpeed:s.animSpeed,
  showClock:s.showClock?1:0, clock24h:s.clock24h?1:0, autoTime:s.autoTime?1:0,
  lang:s.lang, showFileTypes:s.showFileTypes?1:0, settingsWrap:s.settingsWrap?1:0
};
          for (let [id, v] of Object.entries(vals))
            if ($(id)) $(id).value = String(v);
          $("logoStatus").textContent =
            "Текущий режим: " +
            s.bootlogo.toUpperCase() +
            " · перезапуск нужен для применения";
          $("deviceInfo").textContent =
            "SD: " +
            (s.sd ? "готова" : "не обнаружена") +
            " · яркость " +
            (s.brightness + 1) +
            "/4 · FPS " +
            (s.fps ? "on" : "off") +
            " · часы " +
            (s.showClock ? "on" : "off") +
            " · звук " +
            (s.sound ? "on" : "off");
          status("MINI-BOY отвечает");
        } catch (e) {
          $("deviceInfo").textContent = e.message;
          status("Устройство недоступно: " + e.message);
        }
      }
      refreshScreen();
      refreshDevice();
    </script>
  </body>
</html>
)MINIHTML";