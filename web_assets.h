#pragma once

#include <Arduino.h>

/*
  Web Assets
  ----------
  HTML / CSS / JS blobs.
*/


const char INDEX_HTML[] PROGMEM = R"HTML(
    <!doctype html>
    <html>
    <head>
      <meta charset="utf-8"/>
      <meta name="viewport" content="width=device-width, initial-scale=1"/>
      <title>Incubator</title>
      <link rel="stylesheet" href="/style.css">
    </head>
    <body>
    <div class="card">
      <h1>
        <span>🥚 Incubator</span>
        <span class="header-links">
          <a href="/wifi">Settings</a>
          <a href="#" onclick="resetDevice(); return false;">Reboot</a>
        </span>
      </h1>
    
      <!-- System -->
      <div class="row"><span class="label">WiFi</span><span id="wifi" class="value">—</span></div>
      <div class="row"><span class="label">AP SSID</span><span id="ap_ssid" class="value">—</span></div>
      <div class="row"><span class="label">WebSocket</span><span id="ws" class="value">—</span></div>
      <div class="row"><span class="label">Mode</span><span id="mode" class="value">—</span></div>
      <div class="row"><span class="label">Wifi IP</span><span id="ip_sta" class="value">—</span></div>
      <div class="row"><span class="label">AP IP</span><span id="ip_ap" class="value">—</span></div>
      <div class="row"><span class="label">MAC</span><span id="mac" class="value">—</span></div>
    
      <div class="row">
        <span class="label">Profile</span>
        <select id="profileSelect" style="background:#0d1117;border:1px solid var(--border);border-radius:4px;padding:4px 8px;color:var(--text);font-size:13px;font-weight:600">
          <option value="0">Chicken</option>
          <option value="1">Cockatiel</option>
          <option value="2">Cormorant</option>
          <option value="3">Crane</option>
          <option value="4">Duck</option>
          <option value="5">Duck Muscovy</option>
          <option value="6">Eagle</option>
          <option value="7">Emu</option>
          <option value="8">Falcon</option>
          <option value="9">Flamingo</option>
          <option value="10">Goose</option>
          <option value="11">Grouse</option>
          <option value="12">Guinea Fowl</option>
          <option value="13">Hawk</option>
          <option value="14">Heron</option>
          <option value="15">Hummingbird</option>
          <option value="16">Large Parrots</option>
          <option value="17">Lovebird</option>
          <option value="18">Ostrich</option>
          <option value="19">Owl</option>
          <option value="20">Parakeet</option>
          <option value="21">Parrots</option>
          <option value="22">Partridge</option>
          <option value="23">Peacock</option>
          <option value="24">Pelican</option>
          <option value="25">Penguin</option>
          <option value="26">Pheasant</option>
          <option value="27">Pigeon</option>
          <option value="28">Quail</option>
          <option value="29">Rail</option>
          <option value="30">Rhea</option>
          <option value="31">Seabirds</option>
          <option value="32">Songbirds</option>
          <option value="33">Stork</option>
          <option value="34">Swan</option>
          <option value="35">Toucan</option>
          <option value="36">Turkey</option>
          <option value="37">Vulture</option>
          <option value="38">Custom</option>
        </select>
      </div>
    
      <div id="rangeDisplay" class="row">
        <span class="label">Target Range</span>
        <span class="value">
          <span id="range_min">—</span> – <span id="range_max">—</span> °F
        </span>
      </div>
    
      <div id="customRange" style="display:none">
        <div class="row" style="padding:4px 0">
          <span class="label">Min °F</span>
          <input id="tmin" placeholder="Min °F" style="background:#0d1117;border:1px solid var(--border);border-radius:4px;padding:4px 8px;color:var(--text);font-size:13px;width:120px"/>
        </div>
        <div class="row" style="padding:4px 0">
          <span class="label">Max °F</span>
          <input id="tmax" placeholder="Max °F" style="background:#0d1117;border:1px solid var(--border);border-radius:4px;padding:4px 8px;color:var(--text);font-size:13px;width:120px"/>
        </div>
      </div>
    
      <!-- Metrics -->
      <div class="lamp-row">
        <div class="metric wide" style="flex:1">
          <div id="lampIcon" class="lampIcon"></div>
          <div class="lampText">Lamp <span id="lamp">—</span></div>
        </div>
      </div>
      <div class="timer-row">
        <div id="timerContainer" class="timer-container">
          <div id="timerRing" class="timer-ring">
            <div id="timerValue" class="timer-value">2.0</div>
          </div>
        </div>
      </div>
      <div class="grid">
        <div class="metric"><span class="label">Temp (°F)</span><span id="temp_f" class="value">—</span></div>
        <div class="metric"><span class="label">Temp (°C)</span><span id="temp_c" class="value">—</span></div>
        <div class="metric"><span class="label">Humidity (%)</span><span id="rh" class="value">—</span></div>
        <div class="metric"><span class="label">Abs Hum (g/m³)</span><span id="ah" class="value">—</span></div>
        <div class="metric"><span class="label">Dew (°F)</span><span id="dew_f" class="value">—</span></div>
        <div class="metric"><span class="label">HeatIdx (°F)</span><span id="heat_f" class="value">—</span></div>
      </div>
    
      <!-- Peaks -->
      <div class="mini">
        <div class="miniTitle">
          <span class="t">Temperature Peaks</span>
          <span id="peakTop" class="value">—</span>
        </div>
        <div id="peaks" class="peakList"></div>
      </div>
    </div>
    
    <div id="toast" class="toast">Saved</div>
    
    <script src="/app.js"></script>
    </body>
    </html>
    )HTML";
    
    const char WIFI_HTML[] PROGMEM = R"HTML(
    <!doctype html>
    <html>
    <head>
      <meta charset="utf-8"/>
      <meta name="viewport" content="width=device-width, initial-scale=1"/>
      <title>WiFi Settings</title>
      <style>
        :root {
          --bg:#0f1216;
          --panel:#161b22;
          --text:#e6edf3;
          --muted:#9da7b1;
          --accent:#4cc9f0;
          --border:#2a2f36;
          --ok:#3fb950;
        }
        *{box-sizing:border-box}
        body{
          margin:0;padding:16px;
          background:linear-gradient(180deg,#0f1216,#0b0e12);
          color:var(--text);
          font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
        }
        .card{
          max-width:520px;margin:0 auto;
          background:var(--panel);
          border:1px solid var(--border);
          border-radius:12px;
          padding:16px;
          box-shadow:0 10px 30px rgba(0,0,0,.35);
        }
        h1{margin:0 0 12px;font-size:20px}
        .row{margin-bottom:10px}
        label{font-size:13px;color:var(--muted);display:block;margin-bottom:4px}
        input,select{
          width:100%;
          padding:8px;
          border-radius:8px;
          border:1px solid var(--border);
          background:#0d1117;
          color:var(--text)
        }
        button{
          width:100%;
          padding:10px;
          border-radius:10px;
          border:none;
          font-weight:700;
          background:var(--accent);
          color:#000;
          cursor:pointer;
        }
        .net{
          padding:8px;
          border:1px solid var(--border);
          border-radius:8px;
          margin-bottom:6px;
          cursor:pointer;
          background:#11161c;
          font-size:13px;
        }
        .net small{color:var(--muted)}
        a{color:var(--accent);font-size:13px;text-decoration:none}
      </style>
    </head>
    <body>
    <div class="card">
      <h1>📡 WiFi Settings</h1>
    
      <div class="row">
        <label>SSID</label>
        <input id="ssid"/>
      </div>
    
      <div class="row">
        <label>Password</label>
        <input id="pass" type="password"/>
      </div>
    
      <div class="row">
        <label>
          <input type="checkbox" id="keep_ap"/> Keep Access Point enabled
        </label>
      </div>
    
      <button onclick="save()">Save & Apply</button>
    
      <div class="row" style="margin-top:14px">
        <label>Available Networks</label>
        <div id="nets"></div>
      </div>
    
      <div style="text-align:right;margin-top:8px">
        <a href="/">← Back to Status</a>
      </div>
    </div>
    
    <script>
    const el=id=>document.getElementById(id);
    
    function load(){
      fetch('/api/wifi').then(r=>r.json()).then(j=>{
        el('ssid').value=j.ssid||'';
        el('keep_ap').checked=!!j.keep_ap;
      });
    
      fetch('/api/wifi/scan').then(r=>r.json()).then(j=>{
        const host=el('nets');
        host.innerHTML='';
        j.networks.forEach(n=>{
          const d=document.createElement('div');
          d.className='net';
          d.innerHTML=`<b>${n.ssid||'(hidden)'}</b><br><small>RSSI ${n.rssi} ${n.enc?'🔒':''}</small>`;
          d.onclick=()=>el('ssid').value=n.ssid;
          host.appendChild(d);
        });
      });
    }
    
    function save(){
      fetch('/api/wifi',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({
          ssid:el('ssid').value,
          pass:el('pass').value,
          keep_ap:el('keep_ap').checked
        })
      }).then(()=>alert('Saved. Reconnecting…'));
    }
    
    load();
    </script>
    </body>
    </html>
    )HTML";

const char STYLES[] PROGMEM = R"CSS(
    :root {
      --bg:#0f1216;
      --panel:#161b22;
      --text:#e6edf3;
      --muted:#9da7b1;
      --accent:#4cc9f0;
      --ok:#3fb950;
      --bad:#f85149;
      --border:#2a2f36;
    }
    *{box-sizing:border-box}
    body{
      margin:0;padding:16px;
      background:linear-gradient(180deg,#0f1216,#0b0e12);
      color:var(--text);
      font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
    }
    .card{
      max-width:520px;margin:0 auto;
      background:var(--panel);
      border:1px solid var(--border);
      border-radius:12px;
      padding:16px;
      box-shadow:0 10px 30px rgba(0,0,0,.35);
    }
    h1{
      margin:0 0 12px;
      font-size:20px;
      display:flex;
      justify-content:space-between;
      align-items:center;
    }
    .header-links{
      display:flex;
      gap:8px;
      align-items:center;
      font-size:13px;
    }
    .row{
      display:flex;justify-content:space-between;align-items:center;
      padding:6px 0;border-bottom:1px solid var(--border)
    }
    .row:last-child{border-bottom:none}
    .label{color:var(--muted);font-size:13px}
    .value{font-weight:600}
    .grid{
      display:grid;grid-template-columns:1fr 1fr;
      gap:10px;margin-top:10px
    }
    .metric{
      background:#11161c;border:1px solid var(--border);
      border-radius:10px;padding:12px;text-align:center
    }
    .metric .label{display:block;margin-bottom:6px}
    .metric .value{font-size:20px;font-weight:800}
    .metric.wide{
      grid-column:1 / -1;
      display:flex;
      justify-content:center;
      align-items:center;
      gap:10px;
      text-align:center;
      padding:12px;
    }
    .lampIcon{
      width:18px;height:18px;border-radius:50%;
      border:1px solid var(--border);
      box-shadow:0 0 18px rgba(76,201,240,.15);
      background:#0d1117;
    }
    .lampIcon.on{
      background: radial-gradient(circle at 30% 30%, #fff, #4cc9f0 40%, #0d1117 75%);
    }
    .lampText{font-weight:800}
    select,input{
      width:85%;padding:8px;border-radius:8px;
      border:1px solid var(--border);
      background:#0d1117;color:var(--text)
    }
    .toast{
      position:fixed;bottom:20px;left:50%;
      transform:translateX(-50%);
      background:#0d1117;border:1px solid var(--border);
      padding:8px 14px;border-radius:20px;
      opacity:0;transition:.2s
    }
    .toast.show{opacity:1}
    a{color:var(--accent);text-decoration:none;font-size:13px}
    .mini{
      background:#0d1117;
      border:1px solid var(--border);
      border-radius:10px;
      padding:10px;
      margin-top:10px;
    }
    .miniTitle{
      display:flex;
      justify-content:space-between;
      align-items:center;
      margin-bottom:8px;
    }
    .miniTitle .t{
      font-size:13px;
      color:var(--muted);
      font-weight:600;
    }
    .peakList{
      display:flex;
      flex-direction:column;
      gap:6px;
      font-size:13px;
    }
    .peakRow{
      display:flex;
      justify-content:space-between;
      align-items:center;
      gap:10px;
      padding:6px 8px;
      border:1px solid var(--border);
      border-radius:10px;
      background:#11161c;
    }
    .peakTemp{font-weight:800}
    .peakWhen{
      color:var(--muted);
      font-variant-numeric:tabular-nums;
      white-space:nowrap;
    }
    .timer-container{
      position:relative;
      width:80px;
      height:80px;
      display:inline-block;
      vertical-align:middle;
    }
    .timer-ring{
      width:80px;
      height:80px;
      position:relative;
    }
    .timer-ring svg{
      position:absolute;
      top:0;
      left:0;
    }
    .timer-ring svg circle#progressCircle{
      will-change:stroke-dashoffset;
    }
    .timer-value{
      position:absolute;
      top:27%;
      left:24%;
      font-size:14px;
      font-weight:800;
      font-variant-numeric:tabular-nums;
      color:var(--text);
      z-index:1;
      text-align:center;
      line-height:14px;
      white-space:nowrap;
      margin:0;
      padding:0;
      display:inline-block;
      vertical-align:middle;
    }
    .lamp-row{
      display:flex;
      align-items:center;
      margin-bottom:16px;
      margin-top:16px;
    }
    .timer-row{
      display:flex;
      justify-content:flex-start;
      align-items:center;
      margin-bottom:10px;
    }
    )CSS";
    
    const char JAVASCRIPTS[] PROGMEM = R"JS(
    const el = id => document.getElementById(id);
    
    const uiState = {
      editingMin: false,
      editingMax: false,
      tmin: null,
      tmax: null,
    };
    
    let toast, tminEl, tmaxEl;
    let ws = null;
    let timer = 2.0;
    let timerAnimationFrameId = null;
    let timerLastUpdateTime = null;
    let timerRing = null;
    let timerValue = null;
    
    function showToast(){
      toast.classList.add('show');
      setTimeout(()=>toast.classList.remove('show'),1000);
    }
    
    function fmt(v,d=2){
      if(v==null || Number.isNaN(v)) return '—';
      return Number(v).toFixed(d);
    }
    
    function pad2(n){return String(n).padStart(2,'0');}
    
    function fmtWhen(ts){
      if(!ts) return '—';
      const d = new Date(ts*1000);
      let hours = d.getHours();
      const minutes = d.getMinutes();
      const ampm = hours >= 12 ? 'PM' : 'AM';
      hours = hours % 12;
      hours = hours ? hours : 12;
      return `${hours}:${pad2(minutes)} ${ampm}`;
    }
    
    function renderPeaks(list){
      const host = el('peaks');
      host.innerHTML = '';
      el('peakTop').textContent = '—';
    
      if(!Array.isArray(list) || !list.length){
        host.innerHTML =
          '<div class="peakRow"><span class="peakTemp">—</span><span class="peakWhen">No data</span></div>';
        return;
      }
    
      // Find highest overall peak for the top display
      let highestPeak = null;
      for(const p of list){
        if(p.temp_f != null && (!highestPeak || p.temp_f > highestPeak.temp_f)){
          highestPeak = p;
        }
      }
      if(highestPeak && highestPeak.temp_f != null){
        el('peakTop').textContent = `${fmt(highestPeak.temp_f,1)}°F`;
      }
    
      // Display each time window peak
      for(const p of list){
        const r = document.createElement('div');
        r.className = 'peakRow';
        const label = p.label || '—';
        const temp = p.temp_f != null ? `${fmt(p.temp_f,1)}°F` : '—';
        const when = p.ts ? fmtWhen(p.ts) : '—';
        r.innerHTML =
          `<span class="peakTemp">${label}: ${temp}</span>
           <span class="peakWhen">${when}</span>`;
        host.appendChild(r);
      }
    }
    
    function applyStatus(s){
      if(!s) return;
      
      const wsEl = el('ws');
      if(wsEl){
        const wsConnected = s.ws === 'CONNECTED';
        wsEl.textContent = wsConnected ? 'Connected' : 'Disconnected';
        wsEl.style.color = wsConnected ? 'var(--ok)' : 'var(--bad)';
      }
      const wifiEl = el('wifi');
      if(wifiEl){
        const wifiConnected = s.wifi_connected === true;
        const wifiSsid = s.wifi_ssid || '';
        if(wifiSsid){
          wifiEl.innerHTML = wifiSsid + ' ' + (wifiConnected ? '<span style="color:var(--ok)">✓</span>' : '<span style="color:var(--bad)">✗</span>');
        }else{
          wifiEl.innerHTML = '— <span style="color:var(--bad)">✗</span>';
        }
      }
      const modeEl = el('mode');
      if(modeEl) modeEl.textContent = s.mode || '—';
      const ipApEl = el('ip_ap');
      if(ipApEl) ipApEl.textContent = s.ip_ap || '—';
      const apSsidEl = el('ap_ssid');
      if(apSsidEl) apSsidEl.textContent = s.ap_ssid || '—';
      const ipStaEl = el('ip_sta');
      if(ipStaEl) ipStaEl.textContent = s.ip_sta || '—';
      const macEl = el('mac');
      if(macEl) macEl.textContent = s.mac || '—';
    
      // Temperature color coding
      const tempFEl = el('temp_f');
      if(tempFEl){
        tempFEl.textContent = fmt(s.temp_f);
        if(s.temp_f == null || s.temp_f === undefined || Number.isNaN(s.temp_f)){
          tempFEl.style.color = 'var(--text)'; // Default color for invalid
        }else if(s.temp_alarm === true){
          tempFEl.style.color = 'var(--bad)'; // Red - out of range by >0.5°F
        }else if(s.temp_f < s.tmin || s.temp_f > s.tmax){
          tempFEl.style.color = '#ffaa00'; // Yellow - out of target range but within alarm threshold
        }else{
          tempFEl.style.color = 'var(--ok)'; // Green - in range
        }
      }
      const tempCEl = el('temp_c');
      if(tempCEl){
        tempCEl.textContent = fmt(s.temp_c);
        if(s.temp_c == null || s.temp_c === undefined || Number.isNaN(s.temp_c)){
          tempCEl.style.color = 'var(--text)'; // Default color for invalid
        }else if(s.temp_alarm === true){
          tempCEl.style.color = 'var(--bad)'; // Red - out of range by >0.5°F
        }else{
          const tminC = (s.tmin - 32) * 5 / 9;
          const tmaxC = (s.tmax - 32) * 5 / 9;
          if(s.temp_c < tminC || s.temp_c > tmaxC){
            tempCEl.style.color = '#ffaa00'; // Yellow - out of target range but within alarm threshold
          }else{
            tempCEl.style.color = 'var(--ok)'; // Green - in range
          }
        }
      }
      
      // Humidity color coding
      const rhEl = el('rh');
      if(rhEl){
        rhEl.textContent = fmt(s.rh,1);
        if(s.rh == null || s.rh === undefined || Number.isNaN(s.rh)){
          rhEl.style.color = 'var(--text)'; // Default color for invalid
        }else if(s.humidity_alarm === true){
          rhEl.style.color = 'var(--bad)'; // Red - out of range by >5%
        }else if(s.rh < s.hmin || s.rh > s.hmax){
          rhEl.style.color = '#ffaa00'; // Yellow - out of target range but within alarm threshold
        }else{
          rhEl.style.color = 'var(--ok)'; // Green - in range
        }
      }
      const ahEl = el('ah');
      if(ahEl) ahEl.textContent = fmt(s.ah);
      // Dew point color coding - RED when reached (temp equals or below dew point = condensation)
      const dewFEl = el('dew_f');
      if(dewFEl){
        dewFEl.textContent = fmt(s.dew_f);
        if(s.dew_f != null && s.temp_f != null && !Number.isNaN(s.dew_f) && !Number.isNaN(s.temp_f)){
          // Check if current temp is at or below dew point (within 0.1°F tolerance for floating point)
          if(s.temp_f <= (s.dew_f + 0.1)){
            dewFEl.style.color = 'var(--bad)'; // Red when dew point reached (condensation occurring)
          }else{
            dewFEl.style.color = 'var(--text)'; // Normal color
          }
        }else{
          dewFEl.style.color = 'var(--text)'; // Default color for invalid
        }
      }
      const heatFEl = el('heat_f');
      if(heatFEl) heatFEl.textContent = fmt(s.heat_f);
    
      const lampOn = !!s.lamp;
      const lampEl = el('lamp');
      if(lampEl) lampEl.textContent = lampOn ? 'ON' : 'OFF';
      const lampIconEl = el('lampIcon');
      if(lampIconEl) lampIconEl.classList.toggle('on', lampOn);
    
      const isCustom = (s.profile_id === 38);
    
      const profileSelectEl = el('profileSelect');
      // Only update profile dropdown if user hasn't manually selected Custom
      // This prevents WebSocket updates from overwriting user's Custom selection
      if(profileSelectEl && profileSelectEl.dataset.userSelected !== 'true' && profileSelectEl.value != String(s.profile_id)) {
        profileSelectEl.value = String(s.profile_id);
      }
      // Reset the flag if the backend profile matches what user selected
      if(profileSelectEl && profileSelectEl.dataset.userSelected === 'true' && String(s.profile_id) === profileSelectEl.value){
        delete profileSelectEl.dataset.userSelected;
      }
    
      const rangeMinEl = el('range_min');
      if(rangeMinEl) rangeMinEl.textContent = fmt(s.tmin,1);
      const rangeMaxEl = el('range_max');
      if(rangeMaxEl) rangeMaxEl.textContent = fmt(s.tmax,1);
    
      const customRangeEl = el('customRange');
      if(customRangeEl) customRangeEl.style.display = isCustom ? 'block' : 'none';
      const rangeDisplayEl = el('rangeDisplay');
      if(rangeDisplayEl){
        rangeDisplayEl.style.display = isCustom ? 'none' : 'flex';
      }
    
      uiState.tmin = s.tmin;
      uiState.tmax = s.tmax;
    
      if(isCustom && tminEl && tmaxEl){
        if(!uiState.editingMin) tminEl.value = fmt(s.tmin,1);
        if(!uiState.editingMax) tmaxEl.value = fmt(s.tmax,1);
      }
    
      renderPeaks(s.temp_peaks || []);
      
      // Reset timer on data arrival
      resetTimer();
    }
    
    function saveCustomRange(){
      if(uiState.tmin == null || uiState.tmax == null) return;
      fetch('/api/profile',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify({
          profile_id:6,
          tmin:uiState.tmin,
          tmax:uiState.tmax
        })
      }).then(showToast);
    }
    
    function updateTimerVisual(progress){
      if(!timerRing) return;
      let svg = timerRing.querySelector('svg');
      if(!svg){
        svg = document.createElementNS('http://www.w3.org/2000/svg','svg');
        svg.setAttribute('width','60');
        svg.setAttribute('height','60');
        svg.setAttribute('viewBox','0 0 60 60');
        const circleBg = document.createElementNS('http://www.w3.org/2000/svg','circle');
        circleBg.setAttribute('cx','30');
        circleBg.setAttribute('cy','30');
        circleBg.setAttribute('r','27');
        circleBg.setAttribute('fill','none');
        circleBg.setAttribute('stroke','#2a2f36');
        circleBg.setAttribute('stroke-width','3');
        const circle = document.createElementNS('http://www.w3.org/2000/svg','circle');
        circle.setAttribute('cx','30');
        circle.setAttribute('cy','30');
        circle.setAttribute('r','27');
        circle.setAttribute('fill','none');
        circle.setAttribute('stroke','#4cc9f0');
        circle.setAttribute('stroke-width','3');
        circle.setAttribute('stroke-linecap','round');
        circle.setAttribute('stroke-dasharray','169.65');
        circle.setAttribute('stroke-dashoffset','169.65');
        circle.setAttribute('transform','rotate(-90 30 30)');
        circle.id = 'progressCircle';
        svg.appendChild(circleBg);
        svg.appendChild(circle);
        timerRing.appendChild(svg);
      }
      const circle = document.getElementById('progressCircle');
      if(!circle) return;
      const circumference = 2 * Math.PI * 27;
      const offset = circumference - (progress * circumference);
      circle.setAttribute('stroke-dashoffset',offset.toFixed(2));
    }
    
    function updateTimer(currentTime){
      if(!timerLastUpdateTime){
        timerLastUpdateTime = currentTime;
        timerAnimationFrameId = requestAnimationFrame(updateTimer);
        return;
      }
      const deltaTime = (currentTime - timerLastUpdateTime) / 1000;
      timerLastUpdateTime = currentTime;
      timer -= deltaTime;
      if(timer < 0) timer = 0;
      if(timerValue){
        const newText = timer.toFixed(1);
        if(timerValue.textContent !== newText){
          timerValue.textContent = newText;
        }
      }
      const progress = Math.max(0,Math.min(1,(2.0 - timer) / 2.0));
      updateTimerVisual(progress);
      if(timer <= 0){
        timer = 0;
        if(timerAnimationFrameId){
          cancelAnimationFrame(timerAnimationFrameId);
          timerAnimationFrameId = null;
        }
        timerLastUpdateTime = null;
      }else{
        timerAnimationFrameId = requestAnimationFrame(updateTimer);
      }
    }
    
    function resetTimer(){
      timer = 2.0;
      timerLastUpdateTime = null;
      if(timerValue) timerValue.textContent = '2.0';
      updateTimerVisual(0);
      if(timerAnimationFrameId){
        cancelAnimationFrame(timerAnimationFrameId);
        timerAnimationFrameId = null;
      }
      timerAnimationFrameId = requestAnimationFrame(updateTimer);
    }
    
    function resetDevice(){
      if(confirm('This will reboot the device')){
        fetch('/api/reset',{method:'POST'});
      }
    }
    
    function connectWebSocket(){
      if(ws && ws.readyState === WebSocket.OPEN) return;
      
      try {
        ws = new WebSocket(`ws://${location.hostname}:81/`);
        
        ws.onopen = () => {
          console.log('WebSocket connected');
          if(el('ws')) el('ws').textContent = 'CONNECTED';
        };
        
        ws.onclose = () => {
          console.log('WebSocket disconnected');
          if(el('ws')) el('ws').textContent = 'DISCONNECTED';
          setTimeout(connectWebSocket, 2000);
        };
        
        ws.onerror = (err) => {
          console.error('WebSocket error:', err);
          if(el('ws')) el('ws').textContent = 'ERROR';
        };
        
        ws.onmessage = (e) => {
          try {
            const data = JSON.parse(e.data);
            applyStatus(data);
          } catch(err) {
            console.error('Failed to parse WebSocket message:', err, e.data);
          }
        };
      } catch(err) {
        console.error('Failed to create WebSocket:', err);
        if(el('ws')) el('ws').textContent = 'ERROR';
        setTimeout(connectWebSocket, 2000);
      }
    }
    
    function saveProfile(){
      const profileSelectEl = el('profileSelect');
      if(!profileSelectEl) return;
      
      const profileId = parseInt(profileSelectEl.value);
      const isCustom = (profileId === 38);
      
      // Update UI immediately when Custom is selected
      const customRangeEl = el('customRange');
      const rangeDisplayEl = el('rangeDisplay');
      if(customRangeEl) customRangeEl.style.display = isCustom ? 'block' : 'none';
      if(rangeDisplayEl) rangeDisplayEl.style.display = isCustom ? 'none' : 'flex';
      
      // Immediately save to backend to prevent WebSocket from overwriting
      let body = { profile_id: profileId };
      
      if(isCustom && tminEl && tmaxEl){
        const tmin = parseFloat(tminEl.value);
        const tmax = parseFloat(tmaxEl.value);
        if(!isNaN(tmin) && !isNaN(tmax)){
          body.tmin = tmin;
          body.tmax = tmax;
        }else{
          // If Custom selected but no values yet, use current values from status
          body.tmin = uiState.tmin || 98.0;
          body.tmax = uiState.tmax || 100.5;
        }
      }
      
      fetch('/api/profile',{
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body:JSON.stringify(body)
      }).then(() => {
        showToast();
        // Mark that we've saved, so WebSocket updates don't overwrite
        profileSelectEl.dataset.userSelected = 'true';
      });
    }
    
    document.addEventListener('DOMContentLoaded', () => {
      toast = el('toast');
      tminEl = el('tmin');
      tmaxEl = el('tmax');
      timerRing = el('timerRing');
      timerValue = el('timerValue');
      
      const profileSelectEl = el('profileSelect');
      if(profileSelectEl) profileSelectEl.onchange = saveProfile;
      
      if(tminEl) tminEl.onfocus = () => uiState.editingMin = true;
      if(tmaxEl) tmaxEl.onfocus = () => uiState.editingMax = true;
      
      if(tminEl) tminEl.onblur = () => {
        uiState.editingMin = false;
        uiState.tmin = parseFloat(tminEl.value);
        saveCustomRange();
      };
      
      if(tmaxEl) tmaxEl.onblur = () => {
        uiState.editingMax = false;
        uiState.tmax = parseFloat(tmaxEl.value);
        saveCustomRange();
      };
      
      // Initialize timer
      resetTimer();
      
      connectWebSocket();
    });
    )JS";