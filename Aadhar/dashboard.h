// Web dashboard, served at http://aadhar.local/
// Polls /api/status every second; /api/history seeds the chart on load.

#pragma once

const char DASHBOARD_HTML[] = R"rawliteral(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Aadhar Larvae Guard</title>
<style>
:root{--bg:#f4f6f8;--card:#fff;--text:#1a2330;--muted:#6b7684;--line:#e3e7ec;--accent:#1f6feb;--ok:#1a7f37;--bad:#cf222e;--s1:#1f6feb;--s2:#8250df;--s3:#bf8700}
@media (prefers-color-scheme:dark){:root{--bg:#0f1419;--card:#182029;--text:#e6edf3;--muted:#8b98a5;--line:#2a3541;--accent:#4c8dff;--ok:#3fb950;--bad:#f85149;--s1:#4c8dff;--s2:#a371f7;--s3:#e3b341}}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--text)}
main{max-width:980px;margin:0 auto;padding:16px}
header{display:flex;justify-content:space-between;align-items:center;gap:12px;margin-bottom:16px}
h1{font-size:20px;margin:0}
.sub{font-size:13px;color:var(--muted);margin-top:2px}
/* status banner */
.hero{display:flex;gap:18px;align-items:center;flex-wrap:wrap;border-left:4px solid var(--hc,var(--line))}
.hero .info{flex:1;min-width:220px}
.ripples{position:relative;width:64px;height:64px;flex:none}
.ripples i{position:absolute;inset:0;border:2px solid var(--hc,var(--muted));border-radius:50%;opacity:.35}
.ripples i:nth-child(1){transform:scale(.3)}
.ripples i:nth-child(2){transform:scale(.62)}
.hero.active .ripples i{animation:ripple 2.4s ease-out infinite}
.hero.active .ripples i:nth-child(2){animation-delay:.8s}
.hero.active .ripples i:nth-child(3){animation-delay:1.6s}
@keyframes ripple{from{transform:scale(.15);opacity:1}to{transform:scale(1);opacity:0}}
.stats{display:grid;grid-template-columns:repeat(3,auto);gap:4px 24px}
.stats div{display:grid;gap:3px}
.stats b{font-size:15px;font-variant-numeric:tabular-nums}
@media (max-width:600px){.stats{width:100%;grid-template-columns:repeat(3,1fr);gap:4px 12px}}
.pill{font-size:13px;color:var(--muted);display:flex;align-items:center;gap:6px;white-space:nowrap}
.dot{width:9px;height:9px;border-radius:50%;background:var(--bad)}
.dot.on{background:var(--ok)}
.grid{display:grid;gap:12px;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));margin-bottom:12px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px}
.section{margin-bottom:12px}
.head{display:flex;justify-content:space-between;align-items:center;gap:8px;flex-wrap:wrap}
.label{font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:.04em}
.big{font-size:28px;font-weight:600;font-variant-numeric:tabular-nums;margin:4px 0}
.tag{font-size:13px;color:var(--muted);margin-top:6px}
/* water level tank: probe 1 top, probe 3 bottom */
.level{display:flex;gap:24px;flex-wrap:wrap}
.tank{position:relative;width:110px;height:230px;flex:none;margin:0 auto;border:2px solid var(--line);border-radius:14px;overflow:hidden;background:var(--bg)}
.water{position:absolute;left:0;right:0;bottom:0;height:0;background:var(--accent);opacity:.75;transition:height .8s ease}
.mark{position:absolute;left:0;right:0;border-top:2px dashed var(--line);z-index:1}
.mark span{position:absolute;right:6px;top:-10px;font-size:11px;font-weight:600;padding:1px 6px;border-radius:6px;background:var(--card);color:var(--muted);border:1px solid var(--line)}
.mark.wet{border-top-style:solid;border-color:var(--c)}
.mark.wet span{background:var(--c);border-color:var(--c);color:#fff}
.levelinfo{flex:1;min-width:240px}
.prow{display:grid;grid-template-columns:auto 1fr auto;gap:10px;align-items:center;padding:9px 0;border-top:1px solid var(--line);font-size:14px}
.prow:first-of-type{margin-top:10px}
.prow i{width:10px;height:10px;border-radius:50%}
.prow small{color:var(--muted)}
.prow b{font-variant-numeric:tabular-nums;text-align:right}
.warn{margin-top:10px;padding:8px 10px;border-left:3px solid var(--bad);color:var(--bad);font-size:13px;background:var(--bg);border-radius:0 6px 6px 0}
.water::before{content:"";position:absolute;left:0;top:-6px;width:200%;height:7px;background:inherit;
  -webkit-mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 52 7'%3E%3Cpath d='M0 3.5Q13 0 26 3.5T52 3.5V7H0z'/%3E%3C/svg%3E") repeat-x 0 0/52px 7px;
  mask:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 52 7'%3E%3Cpath d='M0 3.5Q13 0 26 3.5T52 3.5V7H0z'/%3E%3C/svg%3E") repeat-x 0 0/52px 7px;
  animation:wave 2.4s linear infinite}
.water.dry::before{display:none}
@keyframes wave{to{transform:translateX(-52px)}}
/* controls */
.prog{height:6px;background:var(--line);border-radius:3px;overflow:hidden;margin-top:10px}
.prog i{display:block;height:100%;width:0;background:var(--accent);transition:width .8s linear}
.row{display:flex;gap:8px;margin-top:12px;flex-wrap:wrap}
button{padding:9px 14px;border-radius:8px;border:1px solid var(--line);background:var(--bg);color:var(--text);font-size:14px;cursor:pointer}
button:hover:not(:disabled){border-color:var(--muted)}
button.primary{background:var(--accent);border-color:var(--accent);color:#fff}
button:disabled{opacity:.45;cursor:default}
.grow{flex:1}
.chips{display:flex;gap:6px;flex-wrap:wrap}
.chip{padding:5px 11px;border-radius:999px;font-size:13px}
.chip[aria-pressed=true]{background:var(--text);border-color:var(--text);color:var(--card)}
.pulse{display:inline-block;width:12px;height:12px;border-radius:50%;background:var(--ok);margin-right:8px;vertical-align:middle;animation:pulse 1.2s ease-in-out infinite}
@keyframes pulse{50%{opacity:.3;transform:scale(.7)}}
/* chart */
.legend{display:flex;gap:6px;flex-wrap:wrap;margin:10px 0 4px}
.legend button{display:flex;align-items:center;gap:6px;font-size:13px;padding:4px 10px;border-radius:999px}
.legend button[aria-pressed=false]{opacity:.4;text-decoration:line-through}
.legend b{width:12px;height:3px;border-radius:2px}
.chart{position:relative}
canvas{width:100%;height:240px;display:block;touch-action:pan-y}
.tip{position:absolute;top:8px;pointer-events:none;background:var(--card);border:1px solid var(--line);border-radius:8px;padding:6px 9px;font-size:12px;line-height:1.6;box-shadow:0 4px 14px rgba(0,0,0,.12);display:none;white-space:nowrap}
.tip i{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
/* log */
ul{list-style:none;margin:8px 0 0;padding:0;max-height:300px;overflow:auto}
li{display:flex;gap:12px;padding:8px 0;border-top:1px solid var(--line);font-size:14px}
li:first-child{border-top:0}
li time{color:var(--muted);font-variant-numeric:tabular-nums;white-space:nowrap}
li.empty{color:var(--muted)}
/* settings */
details summary{cursor:pointer;list-style:none;display:flex;justify-content:space-between;align-items:center}
details summary::after{content:"+";font-size:20px;color:var(--muted)}
details[open] summary::after{content:"\2212"}
form{display:grid;gap:10px;grid-template-columns:repeat(auto-fit,minmax(170px,1fr));margin-top:12px}
form label{font-size:13px;color:var(--muted);display:grid;gap:4px}
input{width:100%;padding:9px 10px;border-radius:8px;border:1px solid var(--line);background:var(--bg);color:var(--text);font-size:14px}
form .row{grid-column:1/-1;margin-top:0}
h3{font-size:14px;margin:18px 0 0}
/* toasts */
#toasts{position:fixed;left:50%;bottom:16px;transform:translateX(-50%);display:grid;gap:8px;z-index:9;width:min(420px,calc(100% - 32px))}
.toast{background:var(--text);color:var(--card);padding:10px 14px;border-radius:10px;font-size:14px;box-shadow:0 6px 20px rgba(0,0,0,.2);animation:in .25s ease}
.toast.bad{background:var(--bad);color:#fff}
@keyframes in{from{opacity:0;transform:translateY(8px)}}
@media (prefers-reduced-motion:reduce){*{animation:none!important;transition:none!important}}
</style>
</head>
<body>
<main>
  <header>
    <div>
      <h1>Aadhar Larvae Guard</h1>
      <div class="sub">Ripples standing water so mosquito larvae can't develop</div>
    </div>
    <div class="pill"><span class="dot" id="dot"></span><span id="conn">Connecting...</span></div>
  </header>

  <div class="card section hero" id="hero">
    <div class="ripples" aria-hidden="true"><i></i><i></i><i></i></div>
    <div class="info">
      <div class="label">Status</div>
      <div class="big" id="status">-</div>
      <div class="tag" id="statusNote">&nbsp;</div>
    </div>
    <div class="stats">
      <div><span class="label">Last ripple</span><b id="lastRipple">-</b></div>
      <div><span class="label">Ripples</span><b id="rippleCount">-</b></div>
      <div><span class="label">Last check</span><b id="lastCheck">-</b></div>
    </div>
  </div>

  <div class="card section level">
    <div class="tank"><div class="water dry" id="water"></div><div id="marks"></div></div>
    <div class="levelinfo">
      <div class="label">Water depth at the device</div>
      <div class="big" id="levelName">-</div>
      <div class="tag" id="levelNote">&nbsp;</div>
      <div class="warn" id="warn" hidden>Probe readings are out of order: a higher probe is wet while a lower one is dry. Check the probe wiring.</div>
      <div id="probeRows"></div>
    </div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="label">Ripple pump</div>
      <div class="big" id="pump">-</div>
      <div class="tag" id="pumpLeft">&nbsp;</div>
      <div class="prog"><i id="pumpProg"></i></div>
      <div class="row">
        <div class="chips" id="durations"></div>
      </div>
      <div class="row">
        <button class="primary grow" id="on">Ripple now</button>
        <button class="grow" id="off">Stop</button>
      </div>
    </div>
    <div class="card">
      <div class="label">Next still-water check</div>
      <div class="big" id="next">-</div>
      <div class="tag" id="nextNote">&nbsp;</div>
      <div class="prog"><i id="checkProg"></i></div>
      <div class="row">
        <button class="grow" id="check">Check now</button>
      </div>
    </div>
  </div>

  <div class="card section">
    <div class="head">
      <div class="label">Probe readings</div>
      <div class="chips" id="ranges"></div>
    </div>
    <div class="legend" id="legend"></div>
    <div class="chart"><canvas id="chart"></canvas><div class="tip" id="tip"></div></div>
  </div>

  <div class="card section">
    <div class="head">
      <div class="label">Event log</div>
      <div class="chips" id="filters"></div>
    </div>
    <ul id="log"></ul>
  </div>

  <details class="card">
    <summary><span class="label">Settings</span></summary>
    <form id="cfgForm">
      <label>Probe dry at or below<input type="number" name="noWater" min="0" max="4095" required></label>
      <label>Probe submerged at or above<input type="number" name="full" min="1" max="4095" required></label>
      <label>Water is still if change within<input type="number" name="stable" min="1" max="4095" required></label>
      <label>Check for still water every (min)<input type="number" name="checkMin" min="1" max="1440" required></label>
      <label>Ripple for (min)<input type="number" name="pumpMin" min="1" max="120" required></label>
      <label>Device submerged: ripple every (h)<input type="number" name="fullHours" min="1" max="72" required></label>
      <div class="row"><button class="primary" type="submit">Save settings</button></div>
    </form>

    <h3>WiFi</h3>
    <div class="tag" id="wifiNow">&nbsp;</div>
    <form id="wifiForm">
      <label>New WiFi name<input name="ssid" autocomplete="off" required></label>
      <label>Password<input name="pass" type="password" autocomplete="new-password"></label>
      <div class="row"><button class="primary" type="submit">Save &amp; restart</button></div>
    </form>
    <div class="tag">The ESP32's own hotspot is always on. Join it and open 192.168.4.1 to see this dashboard without any router.</div>
  </details>
</main>
<div id="toasts"></div>

<script>
const $ = id => document.getElementById(id);
const css = v => getComputedStyle(document.documentElement).getPropertyValue(v).trim();
const COLORS = ['--s1', '--s2', '--s3'];
// Probes sit at different heights in one device: 1 top, 2 middle, 3 bottom.
const NAMES = ['Top', 'Middle', 'Bottom'];
const MARK_AT = [80, 50, 20];   // marker height in the tank, % from bottom
// Level by highest wet probe: none, bottom, middle, top
const LEVELS = [
  ['Dry', 'No water at the bottom probe', 0],
  ['Shallow', 'Water between the bottom and middle probes', 35],
  ['Medium', 'Water between the middle and top probes', 65],
  ['Deep', 'Water at or above the top probe', 94],
];
const CHECK_RESULTS = { none: 'Not yet', dry: 'No water', moving: 'Water changing', still: 'Still water', full: 'Submerged' };
const RANGES = [['1m', 60e3], ['5m', 300e3], ['15m', 900e3], ['1h', 3600e3]];
const FILTERS = [['All', /./], ['Pump', /PUMP/], ['Checks', /check|Initial/i], ['WiFi', /WiFi|hotspot/i]];
const DURATIONS = [1, 5, 15];

let d = null, offset = 0, pts = [], hidden = [false, false, false];
let range = 300e3, filter = FILTERS[0][1], pumpMin = 15, hoverX = null;
let built = false, fails = 0, lastEventT = -1, logKey = '';

// ---------- helpers ----------
function fmt(ms) {
  let s = Math.ceil(ms / 1000), h = Math.floor(s / 3600), m = Math.floor(s % 3600 / 60);
  s %= 60;
  return (h ? h + 'h ' : '') + (h || m ? m + 'm ' : '') + s + 's';
}
const clock = t => new Date(t).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
const esc = s => s.replace(/&/g, '&amp;').replace(/</g, '&lt;');

// Wet/dry with a margin, so a reading hovering at the threshold doesn't flip back and forth.
let wetState = [false, false, false];
function updateWet() {
  const t = d.cfg.noWater, m = Math.max(20, t * 0.25);
  wetState = d.values.map((v, i) => v > t + m ? true : v < t - m ? false : wetState[i]);
  return wetState;
}
const probeState = (v, isWet) => v >= d.cfg.full ? 'Wet (max)' : isWet ? 'Wet' : 'Dry';

function toast(msg, bad) {
  const t = document.createElement('div');
  t.className = 'toast' + (bad ? ' bad' : '');
  t.textContent = msg;
  $('toasts').append(t);
  setTimeout(() => t.remove(), 4000);
}

async function post(url, body) {
  try {
    const r = await fetch(url, { method: 'POST', body: body && new URLSearchParams(body) });
    const j = await r.json().catch(() => ({}));
    if (!r.ok) toast(j.error || 'Request failed.', true);
    return r.ok;
  } catch (e) {
    toast('Could not reach the ESP32.', true);
    return false;
  }
}

function chips(el, items, isOn, onPick) {
  el.innerHTML = '';
  items.forEach(it => {
    const b = document.createElement('button');
    b.className = 'chip';
    b.textContent = it[0];
    b.setAttribute('aria-pressed', isOn(it));
    b.onclick = () => {
      onPick(it);
      [...el.children].forEach((c, i) => c.setAttribute('aria-pressed', isOn(items[i])));
    };
    el.append(b);
  });
}

// ---------- build once ----------
function build() {
  $('marks').innerHTML = d.pins.map((p, i) =>
    `<div class="mark" id="m${i}" style="bottom:${MARK_AT[i]}%;--c:var(${COLORS[i]})"><span>P${i + 1}</span></div>`).join('');
  $('probeRows').innerHTML = d.pins.map((p, i) =>
    `<div class="prow"><i style="background:var(${COLORS[i]})"></i>` +
    `<span>Probe ${i + 1} &middot; ${NAMES[i]} <small>GPIO ${p}</small></span>` +
    `<b><span id="v${i}">-</span> <small id="t${i}"></small></b></div>`).join('');

  $('legend').innerHTML = '';
  d.pins.forEach((p, i) => {
    const b = document.createElement('button');
    b.innerHTML = `<b style="background:${css(COLORS[i])}"></b>P${i + 1} ${NAMES[i]}`;
    b.setAttribute('aria-pressed', 'true');
    b.onclick = () => { hidden[i] = !hidden[i]; b.setAttribute('aria-pressed', !hidden[i]); draw(); };
    $('legend').append(b);
  });

  pumpMin = d.cfg.pumpMin;
  const durs = [...new Set([...DURATIONS, pumpMin])].sort((a, b) => a - b).map(m => [m + ' min', m]);
  chips($('durations'), durs, it => it[1] === pumpMin, it => pumpMin = it[1]);
  chips($('ranges'), RANGES, it => it[1] === range, it => { range = it[1]; draw(); });
  chips($('filters'), FILTERS, it => it[1] === filter, it => { filter = it[1]; logKey = ''; renderLog(); });

  fillSettings();
  built = true;
}

function fillSettings() {
  const f = $('cfgForm');
  for (const k in d.cfg) if (f[k]) f[k].value = d.cfg[k];
}

// ---------- render ----------
function render() {
  if (!built) build();

  $('dot').className = 'dot on';
  $('conn').textContent = d.connected ? 'Live · WiFi ' + d.rssi + ' dBm' : 'Live · hotspot';
  $('wifiNow').textContent = (d.connected
    ? 'Connected to "' + d.ssid + '" · IP ' + d.ip
    : 'Not connected to "' + d.ssid + '"') + ' · Hotspot "' + d.apSsid + '" · IP ' + d.apIp;

  const w = updateWet();
  d.values.forEach((v, i) => {
    $('m' + i).classList.toggle('wet', w[i]);
    $('v' + i).textContent = v;
    $('t' + i).textContent = probeState(v, w[i]);
  });
  const top = w.indexOf(true);                 // highest wet probe (0 = top), -1 = none
  const [name, note, height] = LEVELS[top < 0 ? 0 : 3 - top];
  $('levelName').textContent = name;
  $('levelNote').textContent = note;
  $('water').style.height = height + '%';
  $('water').classList.toggle('dry', top < 0);
  $('warn').hidden = !((w[0] && !w[1]) || (w[1] && !w[2]) || (w[0] && !w[2]));

  // status banner
  const ago = d.pumpRuns ? fmt(d.uptime - d.lastPump) + ' ago' : 'Not yet';
  const next = 'next check in ' + fmt(d.nextCheck);
  let status, info, color;
  if (d.pump) {
    [status, info, color] = ['Rippling the water', 'Pump stops in ' + fmt(d.pumpLeft), '--ok'];
  } else if (top < 0) {
    [status, info, color] = ['No standing water', 'Nothing to treat · ' + next, '--muted'];
  } else if (d.pumpRuns) {
    [status, info, color] = ['Standing water · treated', 'Last rippled ' + ago + ' · ' + next, '--accent'];
  } else {
    [status, info, color] = ['Standing water · watching',
      'If it stays still, the pump ripples it at the ' + next, '--s3'];
  }
  $('hero').style.setProperty('--hc', 'var(' + color + ')');
  $('hero').classList.toggle('active', d.pump);
  $('status').textContent = status;
  $('statusNote').textContent = info;
  $('lastRipple').textContent = d.pump ? 'Now' : ago;
  $('rippleCount').textContent = d.pumpRuns + (d.pumpRuns === 1 ? ' run' : ' runs');
  $('lastCheck').textContent = CHECK_RESULTS[d.checkResult] || d.checkResult;

  $('pump').innerHTML = d.pump ? '<span class="pulse"></span>Rippling' : 'Idle';
  $('pumpLeft').textContent = d.pump ? 'Stops in ' + fmt(d.pumpLeft) : 'Pick a run time to ripple manually';
  $('pumpProg').style.width = d.pump ? (100 - d.pumpLeft / d.pumpTotal * 100) + '%' : '0';
  $('on').disabled = d.pump;
  $('off').disabled = !d.pump;
  $('check').disabled = d.pump;

  $('next').textContent = d.pump ? 'Paused' : fmt(d.nextCheck);
  $('nextNote').textContent = d.pump ? 'Resumes after the pump stops' : 'Checks every ' + fmt(d.checkEvery);
  $('checkProg').style.width = d.pump ? '0' : (100 - d.nextCheck / d.checkEvery * 100) + '%';

  renderLog();
  draw();
}

function renderLog() {
  const ev = d.events;
  const key = ev.length + ':' + (ev.length ? ev[ev.length - 1].t : 0);
  if (key === logKey) return;
  logKey = key;

  // pop up anything new since the last poll
  if (lastEventT >= 0) ev.filter(e => e.t > lastEventT).forEach(e => toast(e.m));
  lastEventT = ev.length ? ev[ev.length - 1].t : 0;

  const rows = ev.filter(e => filter.test(e.m)).reverse();
  $('log').innerHTML = rows.length
    ? rows.map(e => `<li><time>${clock(e.t + offset)}</time><span>${esc(e.m)}</span></li>`).join('')
    : '<li class="empty">Nothing here yet.</li>';
}

// ---------- chart ----------
function draw() {
  if (!d) return;
  const c = $('chart'), dpr = window.devicePixelRatio || 1;
  const w = c.clientWidth, h = c.clientHeight;
  if (c.width !== w * dpr || c.height !== h * dpr) { c.width = w * dpr; c.height = h * dpr; }
  const x = c.getContext('2d');
  x.setTransform(dpr, 0, 0, dpr, 0, 0);
  x.clearRect(0, 0, w, h);

  const L = 38, R = 8, T = 8, B = 20, pw = w - L - R, ph = h - T - B;
  const end = Date.now(), start = end - range;
  const px = t => L + (t - start) / range * pw;
  const py = v => T + ph - v / 4095 * ph;

  x.font = '11px system-ui, sans-serif';
  x.fillStyle = css('--muted');
  x.strokeStyle = css('--line');
  x.lineWidth = 1;
  [0, 1024, 2048, 3072, 4095].forEach(v => {
    x.beginPath(); x.moveTo(L, py(v)); x.lineTo(w - R, py(v)); x.stroke();
    x.fillText(v, 2, py(v) + 4);
  });
  x.fillText(clock(start), L, h - 4);
  x.fillText('now', w - R - 22, h - 4);

  x.setLineDash([4, 4]);
  x.strokeStyle = css('--muted');
  [[d.cfg.noWater, 'dry'], [d.cfg.full, 'full']].forEach(([v, n]) => {
    x.beginPath(); x.moveTo(L, py(v)); x.lineTo(w - R, py(v)); x.stroke();
    x.fillText(n, w - R - 26, py(v) - 4);
  });
  x.setLineDash([]);

  const shown = pts.filter(p => p.t >= start - 20e3);
  [0, 1, 2].forEach(i => {
    if (hidden[i]) return;
    x.strokeStyle = css(COLORS[i]);
    x.lineWidth = 2;
    x.beginPath();
    shown.forEach((p, j) => {
      const gap = j && p.t - shown[j - 1].t > 25e3;   // break the line across gaps
      (j && !gap) ? x.lineTo(px(p.t), py(p.v[i])) : x.moveTo(px(p.t), py(p.v[i]));
    });
    x.stroke();
  });

  // hover crosshair + tooltip
  const tip = $('tip');
  if (hoverX === null || hoverX < L || !shown.length) { tip.style.display = 'none'; return; }
  const t = start + (hoverX - L) / pw * range;
  const near = shown.reduce((a, b) => Math.abs(b.t - t) < Math.abs(a.t - t) ? b : a);
  const nx = px(near.t);
  x.strokeStyle = css('--muted');
  x.beginPath(); x.moveTo(nx, T); x.lineTo(nx, T + ph); x.stroke();
  [0, 1, 2].forEach(i => {
    if (hidden[i]) return;
    x.fillStyle = css(COLORS[i]);
    x.beginPath(); x.arc(nx, py(near.v[i]), 4, 0, 7); x.fill();
  });
  tip.innerHTML = '<b>' + clock(near.t) + '</b><br>' + [0, 1, 2].filter(i => !hidden[i])
    .map(i => `<i style="background:${css(COLORS[i])}"></i>P${i + 1} ${NAMES[i]}: ${near.v[i]}`).join('<br>');
  tip.style.display = 'block';
  tip.style.left = Math.min(nx + 12, w - tip.offsetWidth - 4) + 'px';
}

const chart = $('chart');
chart.addEventListener('pointermove', e => { hoverX = e.clientX - chart.getBoundingClientRect().left; draw(); });
chart.addEventListener('pointerleave', () => { hoverX = null; draw(); });
window.addEventListener('resize', draw);

// ---------- data ----------
async function loadHistory() {
  try {
    const h = await (await fetch('/api/history', { cache: 'no-store' })).json();
    const n = h.data.length;
    pts = h.data.map((v, i) => ({ t: h.last - (n - 1 - i) * h.every + offset, v }));
  } catch (e) {}
}

async function poll() {
  try {
    const r = await fetch('/api/status', { cache: 'no-store' });
    d = await r.json();
    offset = Date.now() - d.uptime;
    if (!built) await loadHistory();
    pts.push({ t: Date.now(), v: d.values });
    while (pts.length && pts[0].t < Date.now() - 3600e3) pts.shift();
    render();
    fails = 0;
  } catch (e) {
    if (++fails > 2) { $('dot').className = 'dot'; $('conn').textContent = 'Disconnected'; }
  }
  setTimeout(poll, 1000);
}

// ---------- actions ----------
$('on').onclick = () => {
  if (confirm('Ripple the water for ' + pumpMin + ' minutes?')) {
    $('on').disabled = true;
    post('/api/pump', { state: 'on', min: pumpMin });
  }
};
$('off').onclick = () => { $('off').disabled = true; post('/api/pump', { state: 'off' }); };

$('check').onclick = () => {
  if (confirm('Check for still water now?\n\nIf the water hasn\'t moved since the last check, the pump ripples it. ' +
              'The check timer restarts from this check.')) {
    $('check').disabled = true;
    post('/api/check');
  }
};

$('cfgForm').onsubmit = async e => {
  e.preventDefault();
  if (await post('/api/settings', new FormData(e.target))) toast('Settings saved.');
};

$('wifiForm').onsubmit = async e => {
  e.preventDefault();
  const f = new FormData(e.target), ssid = f.get('ssid').trim();
  if (!confirm('Switch to "' + ssid + '"?\n\nThe ESP32 will restart, and a running pump will stop. ' +
               'Its own hotspot stays on either way.')) return;
  if (await post('/api/wifi', f)) toast('Saved. Restarting — open http://aadhar.local on "' + ssid + '", or 192.168.4.1 on the hotspot');
};

poll();
</script>
</body>
</html>
)rawliteral";
