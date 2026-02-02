const el = id => document.getElementById(id);

const uiState = {
  tmin: null,
  tmax: null,
  manualEditingTemp: false,
  manualEditingHum: false,
};

let toast;
let ws = null;
let timer = 2.0;
let timerAnimationFrameId = null;
let timerLastUpdateTime = null;
let timerRing = null;
let timerValue = null;

/* When true, next status will prefill manual temp/humidity/turn-every (e.g. after profile change). */
let pendingProfilePrefill = false;
/* 1s debounce timers for Manual fields: save after 1s of no input. */
let manualFieldsDebounceTimer = null;
let turnIntervalDebounceTimer = null;

function showToast(){
  if(toast) { toast.classList.add('show'); setTimeout(()=>toast.classList.remove('show'),1000); }
}

/* Send a command over WebSocket. All settings use WS, not HTTP. */
function sendWsCommand(obj){
  if(!ws || ws.readyState !== WebSocket.OPEN){
    if(toast) { toast.textContent = 'Not connected'; showToast(); }
    return;
  }
  ws.send(JSON.stringify(obj));
  showToast();
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
  const wifiRssiEl = el('wifi_rssi');
  if(wifiRssiEl){
    const rssi = s.wifi_rssi;
    if(s.wifi_connected === true && rssi != null && rssi > -128) wifiRssiEl.textContent = rssi + ' dBm';
    else wifiRssiEl.textContent = '—';
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

  const humidifierEl = el('humidifier');
  if(humidifierEl) humidifierEl.textContent = (s.humidifier === true) ? 'ON' : 'OFF';

  const profileSelectEl = el('profileSelect');
  if(profileSelectEl && profileSelectEl.dataset.userSelected !== 'true' && s.profile_id <= 37 && profileSelectEl.value != String(s.profile_id)) {
    profileSelectEl.value = String(s.profile_id);
  }
  if(profileSelectEl && profileSelectEl.dataset.userSelected === 'true' && String(s.profile_id) === profileSelectEl.value){
    delete profileSelectEl.dataset.userSelected;
  }

  const rangeMinEl = el('range_min');
  if(rangeMinEl) rangeMinEl.textContent = fmt(s.tmin,1);
  const rangeMaxEl = el('range_max');
  if(rangeMaxEl) rangeMaxEl.textContent = fmt(s.tmax,1);

  uiState.tmin = s.tmin;
  uiState.tmax = s.tmax;

  // Mode (shown in Actions dropdown only; not repeated in Overview)
  const effectiveMode = (!s.active || s.process_type === 0) ? 0 : s.process_type;
  const modeSelectEl = el('modeSelect');
  if(modeSelectEl && modeSelectEl.dataset.userSelected !== 'true') modeSelectEl.value = String(effectiveMode);
  if(modeSelectEl && modeSelectEl.dataset.userSelected === 'true' && String(effectiveMode) === modeSelectEl.value) delete modeSelectEl.dataset.userSelected;

  // System On/Off: when Off, sensors still read but no lamp/output
  const systemEnableSelectEl = el('systemEnableSelect');
  if(systemEnableSelectEl && systemEnableSelectEl.dataset.userSelected !== 'true') systemEnableSelectEl.value = (s.system_enabled === false ? '0' : '1');
  if(systemEnableSelectEl && systemEnableSelectEl.dataset.userSelected === 'true' && String(s.system_enabled === false ? 0 : 1) === systemEnableSelectEl.value) delete systemEnableSelectEl.dataset.userSelected;

  // Temp target & Humidity target: editable when Mode is Manual, read-only when preset profile
  const isManualMode = (effectiveMode === 0);
  const profileTempDisplayEl = el('profile_temp_display');
  const manualTempInputsEl = el('manual_temp_inputs');
  const profileHumidityDisplayEl = el('profile_humidity_display');
  const manualHumidityInputsEl = el('manual_humidity_inputs');
  if(profileTempDisplayEl) profileTempDisplayEl.style.display = isManualMode ? 'none' : 'inline';
  if(manualTempInputsEl) manualTempInputsEl.style.display = isManualMode ? 'inline' : 'none';
  if(profileHumidityDisplayEl) profileHumidityDisplayEl.style.display = isManualMode ? 'none' : 'inline';
  if(manualHumidityInputsEl) manualHumidityInputsEl.style.display = isManualMode ? 'inline' : 'none';
  if(isManualMode){
    /* In Manual, only update these fields when user just selected a profile; never overwrite from periodic status. */
    if(pendingProfilePrefill){
      const manualTminEl = el('manual_tmin');
      const manualTmaxEl = el('manual_tmax');
      const manualHminEl = el('manual_hmin');
      const manualHmaxEl = el('manual_hmax');
      if(manualTminEl && s.tmin != null) manualTminEl.value = fmt(s.tmin,1);
      if(manualTmaxEl && s.tmax != null) manualTmaxEl.value = fmt(s.tmax,1);
      if(manualHminEl && s.hmin != null) manualHminEl.value = fmt(s.hmin,0);
      if(manualHmaxEl && s.hmax != null) manualHmaxEl.value = fmt(s.hmax,0);
      const turnEl = el('turn_interval_hours_input');
      if(turnEl){
        const hrs = s.turn_interval_hours != null && s.turn_interval_hours > 0
          ? s.turn_interval_hours
          : (s.motor_turns_per_day != null && s.motor_turns_per_day > 0 ? 24 / s.motor_turns_per_day : null);
        turnEl.value = hrs != null ? (Number(hrs) === Math.round(hrs) ? String(Math.round(hrs)) : Number(hrs).toFixed(2)) : '';
      }
      pendingProfilePrefill = false;
    } else {
      /* When Manual: fill empty fields from profile default (status) so first load shows selected profile's Turn every (hrs), etc. */
      const manualTminEl = el('manual_tmin');
      const manualTmaxEl = el('manual_tmax');
      const manualHminEl = el('manual_hmin');
      const manualHmaxEl = el('manual_hmax');
      const turnEl = el('turn_interval_hours_input');
      if(manualTminEl && manualTminEl.value === '' && s.tmin != null) manualTminEl.value = fmt(s.tmin,1);
      if(manualTmaxEl && manualTmaxEl.value === '' && s.tmax != null) manualTmaxEl.value = fmt(s.tmax,1);
      if(manualHminEl && manualHminEl.value === '' && s.hmin != null) manualHminEl.value = fmt(s.hmin,0);
      if(manualHmaxEl && manualHmaxEl.value === '' && s.hmax != null) manualHmaxEl.value = fmt(s.hmax,0);
      if(turnEl && turnEl.value === ''){
        const hrs = s.turn_interval_hours != null && s.turn_interval_hours > 0
          ? s.turn_interval_hours
          : (s.motor_turns_per_day != null && s.motor_turns_per_day > 0 ? 24 / s.motor_turns_per_day : null);
        if(hrs != null) turnEl.value = Number(hrs) === Math.round(hrs) ? String(Math.round(hrs)) : Number(hrs).toFixed(2);
      }
    }
  } else {
    if(profileTempDisplayEl) profileTempDisplayEl.textContent = (s.tmin != null && s.tmax != null) ? `${fmt(s.tmin,1)} – ${fmt(s.tmax,1)} °F` : '—';
    if(profileHumidityDisplayEl) profileHumidityDisplayEl.textContent = (s.hmin != null && s.hmax != null) ? `${fmt(s.hmin,0)} – ${fmt(s.hmax,0)} %` : '—';
  }

  const processDayEl = el('process_day');
  if(processDayEl) processDayEl.textContent = s.day != null && s.day !== undefined ? String(s.day) : '—';

  // Egg Tilting: Turn every (hrs), next tilt in, last tilt
  // Turn every (hrs): input when Mode is Manual; otherwise read-only from backend
  const turnIntervalHoursInputEl = el('turn_interval_hours_input');
  const turnIntervalHoursDisplayEl = el('turn_interval_hours_display');
  const showTurnIntervalInput = (effectiveMode === 0);
  if(turnIntervalHoursInputEl) turnIntervalHoursInputEl.style.display = showTurnIntervalInput ? '' : 'none';
  if(turnIntervalHoursDisplayEl) turnIntervalHoursDisplayEl.style.display = showTurnIntervalInput ? 'none' : '';
  if(!showTurnIntervalInput && turnIntervalHoursDisplayEl){
    const hrs = s.turn_interval_hours != null && s.turn_interval_hours > 0
      ? s.turn_interval_hours
      : (s.motor_turns_per_day != null && s.motor_turns_per_day > 0 ? 24 / s.motor_turns_per_day : null);
    turnIntervalHoursDisplayEl.textContent = hrs != null ? (Number(hrs) === Math.round(hrs) ? String(Math.round(hrs)) : Number(hrs).toFixed(2)) : '—';
  }
  // In Manual when tilting is on, always show the backend interval in the Turn every (hrs) box
  if(showTurnIntervalInput && turnIntervalHoursInputEl && s.rotation_enabled){
    const hrs = s.turn_interval_hours != null && s.turn_interval_hours > 0
      ? s.turn_interval_hours
      : (s.motor_turns_per_day != null && s.motor_turns_per_day > 0 ? 24 / s.motor_turns_per_day : null);
    if(hrs != null) turnIntervalHoursInputEl.value = Number(hrs) === Math.round(hrs) ? String(Math.round(hrs)) : Number(hrs).toFixed(2);
  }
  // Egg Tilting On/Off toggle: visible when Manual (use dropdown value so it shows as soon as user selects Manual)
  const eggTiltingToggleWrap = el('egg_tilting_toggle_wrap');
  const eggTiltingSwitch = el('egg_tilting_switch');
  const isManualFromDropdown = modeSelectEl && modeSelectEl.value === '0';
  if(eggTiltingToggleWrap) eggTiltingToggleWrap.style.display = isManualFromDropdown ? 'inline-flex' : 'none';
  if(eggTiltingSwitch && isManualFromDropdown) eggTiltingSwitch.checked = !!s.rotation_enabled;

  const motorNextEl = el('motor_seconds_until_next');
  if(motorNextEl){
    const sec = s.motor_seconds_until_next;
    if(sec == null || sec === undefined) motorNextEl.textContent = '—';
    else if(sec < 60) motorNextEl.textContent = sec + ' s';
    else motorNextEl.textContent = Math.round(sec / 60) + ' min';
  }
  const motorLastEl = el('motor_last_turn');
  if(motorLastEl) motorLastEl.textContent = s.motor_last_turn ? fmtWhen(s.motor_last_turn) : '—';

  renderPeaks(s.temp_peaks || []);

  // Reset timer on data arrival
  resetTimer();
}

function saveManualTargets(){
  const manualTminEl = el('manual_tmin');
  const manualTmaxEl = el('manual_tmax');
  const manualHminEl = el('manual_hmin');
  const manualHmaxEl = el('manual_hmax');
  if(!manualTminEl || !manualTmaxEl || !manualHminEl || !manualHmaxEl) return;
  const tmin = parseFloat(manualTminEl.value);
  const tmax = parseFloat(manualTmaxEl.value);
  const hmin = parseFloat(manualHminEl.value);
  const hmax = parseFloat(manualHmaxEl.value);
  if(Number.isNaN(tmin) || Number.isNaN(tmax) || Number.isNaN(hmin) || Number.isNaN(hmax)) return;
  sendWsCommand({ type: 'set_manual_targets', tmin, tmax, hmin, hmax });
}

// Prefill temp, humidity, and Turn every (hrs) from status (e.g. when switching to Manual or when profile is selected).
function prefillManualFieldsFromStatus(s){
  if(!s) return;
  const manualTminEl = el('manual_tmin');
  const manualTmaxEl = el('manual_tmax');
  const manualHminEl = el('manual_hmin');
  const manualHmaxEl = el('manual_hmax');
  const turnIntervalHoursInputEl = el('turn_interval_hours_input');
  if(manualTminEl && s.tmin != null) manualTminEl.value = fmt(s.tmin,1);
  if(manualTmaxEl && s.tmax != null) manualTmaxEl.value = fmt(s.tmax,1);
  if(manualHminEl && s.hmin != null) manualHminEl.value = fmt(s.hmin,0);
  if(manualHmaxEl && s.hmax != null) manualHmaxEl.value = fmt(s.hmax,0);
  if(turnIntervalHoursInputEl){
    const hrs = s.turn_interval_hours != null && s.turn_interval_hours > 0
      ? s.turn_interval_hours
      : (s.motor_turns_per_day != null && s.motor_turns_per_day > 0 ? 24 / s.motor_turns_per_day : null);
    turnIntervalHoursInputEl.value = hrs != null ? (Number(hrs) === Math.round(hrs) ? String(Math.round(hrs)) : Number(hrs).toFixed(2)) : '';
  }
}

// Saves egg turning schedule: UI is "turn every X hours"; backend expects turns_per_day (24/hours).
function saveTurningSchedule(){
  const turnIntervalHoursInputEl = el('turn_interval_hours_input');
  if(!turnIntervalHoursInputEl) return;
  const hours = parseFloat(turnIntervalHoursInputEl.value);
  if(Number.isNaN(hours) || hours <= 0 || hours > 24) return;
  const turnsPerDay = Math.round(24 / hours);
  const clamped = Math.max(1, Math.min(24, turnsPerDay)); // backend caps at 24
  sendWsCommand({ type: 'set_turning', turns_per_day: clamped });
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
    sendWsCommand({ type: 'reset' });
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

  const profileId = parseInt(profileSelectEl.value, 10);
  if(profileId < 0 || profileId > 37) return;

  sendWsCommand({ type: 'set_profile', profile_id: profileId });
  profileSelectEl.dataset.userSelected = 'true';
  /* Next status will prefill manual temp/humidity/turn every (when Manual); applyStatus uses pendingProfilePrefill. */
  pendingProfilePrefill = true;
}

document.addEventListener('DOMContentLoaded', () => {
  toast = el('toast');
  timerRing = el('timerRing');
  timerValue = el('timerValue');

  const profileSelectEl = el('profileSelect');
  if(profileSelectEl) profileSelectEl.onchange = saveProfile;

  // Initialize timer
  resetTimer();

  // System On/Off: when Off, no lamp/output (sensors still read)
  const systemEnableSelectEl = el('systemEnableSelect');
  if(systemEnableSelectEl) systemEnableSelectEl.onchange = () => {
    const v = systemEnableSelectEl.value;
    systemEnableSelectEl.dataset.userSelected = 'true';
    sendWsCommand({ type: 'set_system', enabled: v === '1' });
  };

  // Mode dropdown (Manual / Egg Holding / Incubation): process state after Profile is chosen
  const modeSelectEl = el('modeSelect');
  if(modeSelectEl) modeSelectEl.onchange = () => {
    modeSelectEl.dataset.userSelected = 'true';
    const mode = parseInt(modeSelectEl.value, 10);
    sendWsCommand({ type: 'set_mode', mode });
    // When switching to Manual, prefill temp/humidity/turn every from current (selected profile) status so user can tweak
    if(mode === 0) prefillManualFieldsFromStatus(window._lastStatus);
    // Show/hide Egg Tilting toggle immediately when Mode changes (Manual = show)
    const eggTiltingToggleWrap = el('egg_tilting_toggle_wrap');
    if(eggTiltingToggleWrap) eggTiltingToggleWrap.style.display = (mode === 0) ? 'inline-flex' : 'none';
  };

  // Temp/Humidity target visibility: editable when Mode is Manual, read-only when preset profile
  const profileTempDisplayEl = el('profile_temp_display');
  const manualTempInputsEl = el('manual_temp_inputs');
  const profileHumidityDisplayEl = el('profile_humidity_display');
  const manualHumidityInputsEl = el('manual_humidity_inputs');
  function updateTargetVisibility(){
    const modeSelectEl = el('modeSelect');
    const isManual = modeSelectEl && modeSelectEl.value === '0';
    if(profileTempDisplayEl) profileTempDisplayEl.style.display = isManual ? 'none' : 'inline';
    if(manualTempInputsEl) manualTempInputsEl.style.display = isManual ? 'inline' : 'none';
    if(profileHumidityDisplayEl) profileHumidityDisplayEl.style.display = isManual ? 'none' : 'inline';
    if(manualHumidityInputsEl) manualHumidityInputsEl.style.display = isManual ? 'inline' : 'none';
    const eggTiltingToggleWrap = el('egg_tilting_toggle_wrap');
    if(eggTiltingToggleWrap) eggTiltingToggleWrap.style.display = isManual ? 'inline-flex' : 'none';
  }
  updateTargetVisibility();
  if(profileSelectEl) profileSelectEl.addEventListener('change', updateTargetVisibility);
  const modeSelectElForVisibility = el('modeSelect');
  if(modeSelectElForVisibility) modeSelectElForVisibility.addEventListener('change', updateTargetVisibility);

  // Egg Tilting toggle: sync visibility from Mode dropdown on load (so Manual default shows before first status)
  const eggTiltingToggleWrapInit = el('egg_tilting_toggle_wrap');
  const modeSelectInit = el('modeSelect');
  if(eggTiltingToggleWrapInit && modeSelectInit) eggTiltingToggleWrapInit.style.display = modeSelectInit.value === '0' ? 'inline-flex' : 'none';

  // Tilt now: link after "Next tilt in" – triggers one tilt; schedule continues every X hrs from that point
  const tiltNowLink = el('tilt_now_link');
  if(tiltNowLink) tiltNowLink.addEventListener('click', (e) => { e.preventDefault(); sendWsCommand({ type: 'tilt_now' }); });

  // Manual temp/humidity: 1s debounced save on input; save immediately on blur (cancel debounce)
  const manualTminEl = el('manual_tmin');
  const manualTmaxEl = el('manual_tmax');
  const manualHminEl = el('manual_hmin');
  const manualHmaxEl = el('manual_hmax');
  function scheduleManualTargetsSave(){
    if(manualFieldsDebounceTimer) clearTimeout(manualFieldsDebounceTimer);
    manualFieldsDebounceTimer = setTimeout(() => { saveManualTargets(); manualFieldsDebounceTimer = null; }, 1000);
  }
  function flushManualTargetsSave(){
    if(manualFieldsDebounceTimer){ clearTimeout(manualFieldsDebounceTimer); manualFieldsDebounceTimer = null; }
    saveManualTargets();
  }
  if(manualTminEl){ manualTminEl.addEventListener('input', scheduleManualTargetsSave); manualTminEl.onblur = flushManualTargetsSave; }
  if(manualTmaxEl){ manualTmaxEl.addEventListener('input', scheduleManualTargetsSave); manualTmaxEl.onblur = flushManualTargetsSave; }
  if(manualHminEl){ manualHminEl.addEventListener('input', scheduleManualTargetsSave); manualHminEl.onblur = flushManualTargetsSave; }
  if(manualHmaxEl){ manualHmaxEl.addEventListener('input', scheduleManualTargetsSave); manualHmaxEl.onblur = flushManualTargetsSave; }

  // Turn every (hrs): 1s debounced save on input; save immediately on blur (cancel debounce)
  const turnIntervalHoursInputEl = el('turn_interval_hours_input');
  function scheduleTurnIntervalSave(){
    if(turnIntervalDebounceTimer) clearTimeout(turnIntervalDebounceTimer);
    turnIntervalDebounceTimer = setTimeout(() => { saveTurningSchedule(); turnIntervalDebounceTimer = null; }, 1000);
  }
  function flushTurnIntervalSave(){
    if(turnIntervalDebounceTimer){ clearTimeout(turnIntervalDebounceTimer); turnIntervalDebounceTimer = null; }
    saveTurningSchedule();
  }
  if(turnIntervalHoursInputEl){ turnIntervalHoursInputEl.addEventListener('input', scheduleTurnIntervalSave); turnIntervalHoursInputEl.onblur = flushTurnIntervalSave; }

  // Egg Tilting On/Off toggle (Manual only): send set_turning when toggled
  const eggTiltingSwitch = el('egg_tilting_switch');
  if(eggTiltingSwitch) eggTiltingSwitch.addEventListener('change', () => {
    const enabled = eggTiltingSwitch.checked;
    const turnEl = el('turn_interval_hours_input');
    const hours = turnEl ? parseFloat(turnEl.value) : NaN;
    const turnsPerDay = (!Number.isNaN(hours) && hours > 0 && hours <= 24) ? Math.max(1, Math.min(24, Math.round(24 / hours))) : 12;
    sendWsCommand({ type: 'set_turning', enabled, turns_per_day: enabled ? turnsPerDay : 0 });
  });

  connectWebSocket();
});
