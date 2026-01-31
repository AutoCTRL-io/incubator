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
        if(data.type === 'info'){
          window._lastInfo = data;
          applyStatus(Object.assign({}, window._lastStatus || {}, data));
        } else {
          window._lastStatus = data;
          applyStatus(Object.assign({}, data, window._lastInfo || {}));
        }
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
