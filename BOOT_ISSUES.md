# Potential Boot Issues Identified

## Critical Issues Found:

### 1. **WiFi AP Not Ready When Server Starts** ⚠️ CRITICAL
**Problem:** `WiFi.softAP()` is called but the code doesn't wait for the AP to be ready before calling `webServerInit()` and `server.begin()`. ESP32 needs time to initialize the access point.

**Location:** `incubator.ino:37-38` → `webServerInit()` called immediately after

**Impact:** Web server may fail to start, OTA won't work, device won't be accessible

**Fix Needed:** Add delay and check WiFi.status() before starting servers

---

### 2. **No Error Checking on WiFi Initialization** ⚠️ CRITICAL
**Problem:** `WiFi.softAP()` can fail, but there's no check. Code continues even if WiFi fails.

**Location:** `incubator.ino:38`

**Impact:** If WiFi fails, device continues but is unreachable

**Fix Needed:** Check return value and add error handling

---

### 3. **Potential Null Pointer in coreInit()** ⚠️ HIGH
**Problem:** `coreInit()` calls `updateTargets()` which calls `getProfileById(process.profileId)`. If NVS has corrupted data with invalid `profileId` (e.g., 255 or out of range), `getProfileById()` returns `nullptr`, and `updateTargets()` tries to access `p->incTempMinF` etc.

**Location:** `core_controller.cpp:46-49` → `coreInit()` → `updateTargets()` → `getProfileById()`

**Impact:** Crash/reboot loop if NVS has invalid profileId

**Fix Needed:** Validate profileId after loading from NVS, or add null check in updateTargets()

---

### 4. **Missing Delay After WiFi Setup** ⚠️ MEDIUM
**Problem:** ESP32 needs time to initialize WiFi hardware. No delay between `WiFi.softAP()` and server initialization.

**Location:** `incubator.ino:38-53`

**Impact:** Race condition where server starts before WiFi is ready

**Fix Needed:** Add `delay(100)` or wait for `WiFi.status() == WL_CONNECTED` (for AP mode, check `WiFi.softAPgetStationNum()` or similar)

---

### 5. **NVS Data Validation Missing** ⚠️ MEDIUM
**Problem:** `loadProcessState()` loads data from NVS without validating ranges. Invalid enum values could cause crashes.

**Location:** `app_state.cpp:54-81`

**Impact:** Corrupted NVS could cause crashes

**Fix Needed:** Validate enum values after loading (e.g., profileId < EGG_PROFILE_COUNT)

---

### 6. **Serial Output May Not Be Visible** ℹ️ INFO
**Problem:** If device crashes before Serial is ready, you won't see error messages. Serial.begin() needs time.

**Location:** `incubator.ino:35`

**Impact:** Hard to debug if crash happens early

**Fix Needed:** Add `delay(1000)` after Serial.begin() to ensure Serial is ready

---

## Recommended Fixes (Priority Order):

1. **Add WiFi ready check and delay** (CRITICAL)
2. **Add null pointer check in updateTargets()** (CRITICAL)
3. **Validate profileId after NVS load** (HIGH)
4. **Add error checking for WiFi.softAP()** (HIGH)
5. **Add delay after Serial.begin()** (MEDIUM)
