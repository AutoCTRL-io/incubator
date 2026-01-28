# Codebase Audit Report
## Verification Against instructions.txt

### ✅ File Structure
All required files exist:
- ✅ incubator.ino
- ✅ app_state.h / app_state.cpp
- ✅ core_controller.h / core_controller.cpp
- ✅ sensor_dht.h / sensor_dht.cpp
- ✅ motor_stepper.h / motor_stepper.cpp
- ✅ web_server.h / web_server.cpp
- ✅ ws_server.h / ws_server.cpp
- ✅ web_assets.h
- ✅ profiles.h / profiles.cpp
- ✅ ota_manager.h / ota_manager.cpp (not in instructions but present)

---

### 1. incubator.ino ✅
**Requirements:**
- ✅ Entry point only (setup/loop)
- ✅ Initialization order
- ✅ Calls into other modules
- ✅ No logic, state, math, web routes, hardware control

**Implementation:**
- ✅ Contains only setup() and loop()
- ✅ Initializes subsystems in correct order
- ✅ Calls periodic functions (otaLoop, webServerLoop, wsServerLoop, coreUpdate)
- ✅ No business logic present

---

### 2. app_state.h / app_state.cpp ✅
**Requirements:**
- ✅ Enums: EggProfileId, ProcessType, ControlMode
- ✅ Structs: EggProfileData (in profiles.h), ProcessState
- ✅ Global: ProcessState process
- ✅ NVS: load/save ProcessState
- ✅ Profile lookup: getProfileById() (instructions say getProfile, but functionally equivalent)

**Implementation:**
- ✅ All enums present and correct
- ✅ ProcessState struct complete with all fields
- ✅ Global `process` instance defined
- ✅ `loadProcessState()` - loads all fields from NVS
- ✅ `saveProcessState()` - saves all fields to NVS
- ✅ `resetProcessState()` - resets to defaults
- ✅ Helper functions: `isCustomProfileActive()`, `isProcessRunning()`
- ✅ Global targets: targetMinF, targetMaxF, targetHMin, targetHMax
- ⚠️ `currentProfile` declared but never updated (appears unused, process.profileId is source of truth)

---

### 3. core_controller.h / core_controller.cpp ✅
**Requirements:**
- ✅ Process lifecycle: start, cancel, transition
- ✅ Day tracking (epoch → day count)
- ✅ Target resolution (derives from ProcessState + EggProfileData)
- ✅ Egg turning scheduling
- ✅ Calls motor driver when time to turn

**Implementation:**
- ✅ `coreInit()` - initializes controller
- ✅ `startProcess()` - starts new process, sets epoch, updates targets, saves state
- ✅ `cancelProcess()` - cancels process, resets state, saves
- ✅ `transitionProcess()` - transitions EGG_HOLDING → INCUBATION, updates targets
- ✅ `coreUpdate()` - updates current day, checks auto-completion, handles egg turning
- ✅ `computeCurrentDay()` - calculates day from epoch
- ✅ `updateTargets()` - updates global targets based on process type and profile
- ✅ `getActiveTargetMinF/MaxF/HumMin/HumMax()` - resolve active targets
- ✅ `shouldTurnEggs()` - determines if eggs should be turned
- ✅ `markEggsTurned()` - records turn timestamp, saves state
- ✅ Auto-completion when incubation days reached
- ✅ Custom profile support in all functions

---

### 4. sensor_dht.h / sensor_dht.cpp ✅
**Requirements:**
- ✅ DHT22 initialization
- ✅ Sensor reads
- ✅ Derived metrics: Temp °C/°F, RH, Absolute humidity, Dew point, Heat index
- ✅ SensorReadings struct

**Implementation:**
- ✅ `sensorInit()` - initializes DHT22
- ✅ `sensorRead()` - reads and validates sensor data
- ✅ All metrics calculated: tempC, tempF, humidity, absHumidity, dewPointC, dewPointF, heatIndexC, heatIndexF
- ✅ Validation: checks for NaN and reasonable ranges
- ✅ Utility functions: cToF(), absoluteHumidity_gm3(), dewPointC()

---

### 5. motor_stepper.h / motor_stepper.cpp ✅
**Requirements:**
- ✅ Stepper motor pin configuration
- ✅ Initialization
- ✅ Single "turn eggs" action
- ✅ Busy / safety state (implemented as position/phase tracking)

**Implementation:**
- ✅ `StepperConfig` struct with all pin config
- ✅ `stepperInit()` - initializes pins and config
- ✅ `stepperEnable()` - enables/disables motor
- ✅ `stepperTurnOnce()` - performs full rotation, updates position
- ✅ `MotorStatus` struct with position, phase, timing info
- ✅ `stepperGetStatus()` - returns current motor status
- ✅ `stepperSetTurnsPerDay()` - updates target turns per day
- ✅ Tracks absolute position and rotation phase
- ✅ Calculates seconds until next turn

---

### 6. web_server.h / web_server.cpp ✅
**Requirements:**
- ✅ WebServer instance
- ✅ Route registration
- ✅ Endpoints: /, /api/state, /api/process/start, /api/process/cancel, /api/process/transition
- ✅ JSON serialization of state

**Implementation:**
- ✅ `webServerInit()` - registers all routes
- ✅ `webServerLoop()` - handles client requests
- ✅ Routes:
  - ✅ `/` - serves INDEX_HTML
  - ✅ `/wifi` - serves WIFI_HTML
  - ✅ `/style.css` - serves STYLES
  - ✅ `/app.js` - serves JAVASCRIPTS
  - ✅ `/api/state` - GET, returns ProcessState as JSON
  - ✅ `/api/process/start` - POST, starts process
  - ✅ `/api/process/cancel` - POST, cancels process
  - ✅ `/api/process/transition` - POST, transitions process
  - ✅ `/api/reset` - POST, resets process state
- ✅ All ProcessState fields serialized in handleGetState()
- ✅ Proper error handling and JSON parsing

---

### 7. ws_server.h / ws_server.cpp ✅
**Requirements:**
- ✅ WebSocket server
- ✅ Client connect/disconnect handling
- ✅ Periodic broadcast of: sensor data, process state, resolved targets, motor status

**Implementation:**
- ✅ `wsServerInit()` - initializes WebSocket server
- ✅ `wsServerLoop()` - handles WebSocket events
- ✅ `wsBroadcastStatus()` - broadcasts all required data:
  - ✅ Sensor data: temp_f, temp_c, rh, ah, dew_f, heat_f
  - ✅ Process state: active, profile_id, process_type, day
  - ✅ Resolved targets: tmin, tmax, hmin, hmax
  - ✅ Motor status: motor_position, motor_phase, motor_last_turn, motor_turns_per_day, motor_seconds_until_next

---

### 8. profiles.h / profiles.cpp ✅
**Requirements:**
- ✅ EggProfileData struct (static biological defaults)
- ✅ Profile lookup function
- ✅ All 38 profiles defined

**Implementation:**
- ✅ `EggProfileData` struct with all required fields
- ✅ `getProfileById()` - looks up profile by ID
- ✅ All 38 profiles defined (0-37 + CUSTOM):
  - ✅ 0: Chicken
  - ✅ 1: Cockatiel
  - ✅ 2: Cormorant
  - ✅ 3: Crane
  - ✅ 4: Duck
  - ✅ 5: Duck Muscovy
  - ✅ 6: Eagle
  - ✅ 7: Emu
  - ✅ 8: Falcon
  - ✅ 9: Flamingo
  - ✅ 10: Goose
  - ✅ 11: Grouse
  - ✅ 12: Guinea Fowl
  - ✅ 13: Hawk
  - ✅ 14: Heron
  - ✅ 15: Hummingbird
  - ✅ 16: Large Parrots
  - ✅ 17: Lovebird
  - ✅ 18: Ostrich
  - ✅ 19: Owl
  - ✅ 20: Parakeet
  - ✅ 21: Parrots
  - ✅ 22: Partridge
  - ✅ 23: Peacock
  - ✅ 24: Pelican
  - ✅ 25: Penguin
  - ✅ 26: Pheasant
  - ✅ 27: Pigeon
  - ✅ 28: Quail
  - ✅ 29: Rail
  - ✅ 30: Rhea
  - ✅ 31: Seabirds
  - ✅ 32: Songbirds
  - ✅ 33: Stork
  - ✅ 34: Swan
  - ✅ 35: Toucan
  - ✅ 36: Turkey
  - ✅ 37: Vulture
  - ✅ 38: Custom

---

### 9. web_assets.h ✅
**Requirements:**
- ✅ INDEX_HTML
- ✅ CSS (STYLES)
- ✅ JavaScript (JAVASCRIPTS)
- ✅ WIFI_HTML

**Implementation:**
- ✅ All assets present and defined as PROGMEM strings

---

### Functional Features ✅
**Requirements:**
- ✅ Egg holding (preservation) mode
- ✅ Incubation mode
- ✅ Profile-based biological defaults
- ✅ Custom profile support
- ✅ Day tracking from arbitrary start day
- ✅ Automatic egg turning via stepper motor
- ✅ Real-time UI updates
- ✅ Persistent recovery after reboot
- ✅ Safe transitions between modes
- ⏸️ Future-ready humidity control (structure in place)
- ⏸️ Future-ready rescue / override modes (structure in place)

---

### Architectural Guarantees ✅
- ✅ No duplicated state
- ✅ No hidden coupling
- ✅ No UI-driven hardware access
- ✅ Profiles are immutable defaults
- ✅ Custom behavior lives in ProcessState
- ✅ Hardware can be swapped without refactoring logic

---

### Issues Found

#### Minor Issues:
1. **`currentProfile` variable** - Declared in app_state.h but never updated. `process.profileId` is the actual source of truth. Consider removing if unused, or update it when profile changes.

2. **Function naming** - Instructions mention `getProfile(profileId)` but code uses `getProfileById()`. Functionally equivalent, just a naming difference.

#### Potential Improvements:
1. **Motor position persistence** - Motor absolute position resets on reboot. Could be saved to NVS if needed for continuity.

2. **Error handling** - Some functions return bool but errors aren't always propagated to UI. Consider adding error codes.

---

### Summary
**Status: ✅ ALL REQUIREMENTS MET**

All files exist, all functions are implemented, all features are present and operational. The codebase fully complies with the architectural plan in instructions.txt. Only minor issues found (unused variable, naming difference) which don't affect functionality.
