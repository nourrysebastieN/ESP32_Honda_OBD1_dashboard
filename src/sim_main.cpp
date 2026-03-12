#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <lvgl.h>

#include "src/drivers/sdl/lv_sdl_keyboard.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_mousewheel.h"
#include "src/drivers/sdl/lv_sdl_window.h"

#include "config.h"
#include "dashboard_ui.h"
#include "obd1_handler.h"
#include "obd1_decoder.h"
#include "obd1_samples.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

constexpr uint32_t SAMPLE_HOLD_MS = 3500;
constexpr uint32_t SCREEN_HOLD_MS = 5000;
constexpr float RPM_RAMP_UP_PER_SEC = 4500.0f;
constexpr float RPM_RAMP_DOWN_PER_SEC = 5500.0f;
constexpr float SPEED_RAMP_UP_PER_SEC = 120.0f;
constexpr float SPEED_RAMP_DOWN_PER_SEC = 160.0f;
constexpr float TEMP_RAMP_UP_PER_SEC = 35.0f;
constexpr float TEMP_RAMP_DOWN_PER_SEC = 45.0f;

static uint32_t gElapsedMs = 0;
static uint32_t clearDtcUntilMs = 0;
static float gSimRpm = 0.0f;
static float gSimSpeed = 0.0f;
static float gSimTemp = 0.0f;
static bool gPrevRKey = false;
static bool gPrevSKey = false;
static bool gPrevTKey = false;

bool onClearDtcRequest(void) {
    clearDtcUntilMs = gElapsedMs + 2000;
    return true;
}

void populateDemoData(OBD1Data &data, uint32_t elapsedMs, size_t sampleIndex) {
    if (!decodeObd1Packet(OBD1_SAMPLE_PACKETS[sampleIndex],
                          OBD1_PACKET_LENGTH,
                          data,
                          elapsedMs)) {
        data.connected = false;
        data.checkEngine = false;
        data.dtcCount = 0;
        std::memset(data.dtcCodes, 0, sizeof(data.dtcCodes));
        data.timestamp = elapsedMs;
        return;
    }

    if (elapsedMs < clearDtcUntilMs) {
        data.dtcCount = 0;
        std::memset(data.dtcCodes, 0, sizeof(data.dtcCodes));
        data.checkEngine = false;
    }

    data.connected = true;
}

void updateScreenCycle(uint32_t elapsedMs, bool autoCycleEnabled) {
    if (!autoCycleEnabled) {
        return;
    }
    const uint32_t slot = (elapsedMs / SCREEN_HOLD_MS) % 3;
    DashboardScreen target = DashboardScreen::MAIN;
    switch (slot) {
        case 0:
            target = DashboardScreen::MAIN;
            break;
        case 1:
            target = DashboardScreen::SETTINGS;
            break;
        case 2:
            target = DashboardScreen::DIAGNOSTICS;
            break;
        default:
            target = DashboardScreen::MAIN;
            break;
    }

    if (DashboardUI::getCurrentScreen() != target) {
        DashboardUI::switchScreen(target);
    }
}

void applyKeyOverrides(OBD1Data& data, uint32_t deltaMs) {
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    const bool rKey = state[SDL_SCANCODE_R] != 0;
    const bool sKey = state[SDL_SCANCODE_S] != 0;
    const bool tKey = state[SDL_SCANCODE_T] != 0;
    const float dt = static_cast<float>(deltaMs) / 1000.0f;

    if (rKey && !gPrevRKey) {
        gSimRpm = static_cast<float>(data.rpm);
    }
    if (rKey) {
        gSimRpm += RPM_RAMP_UP_PER_SEC * dt;
        if (gSimRpm > RPM_REDLINE) {
            gSimRpm = RPM_REDLINE;
        }
    } else if (gSimRpm > 0.0f) {
        gSimRpm -= RPM_RAMP_DOWN_PER_SEC * dt;
        if (gSimRpm < 0.0f) {
            gSimRpm = 0.0f;
        }
    }
    if (rKey || gSimRpm > 0.0f) {
        data.rpm = static_cast<uint16_t>(std::lround(gSimRpm));
    }

    if (sKey && !gPrevSKey) {
        gSimSpeed = static_cast<float>(data.speed);
    }
    if (sKey) {
        gSimSpeed += SPEED_RAMP_UP_PER_SEC * dt;
        if (gSimSpeed > SPEED_MAX) {
            gSimSpeed = SPEED_MAX;
        }
    } else if (gSimSpeed > 0.0f) {
        gSimSpeed -= SPEED_RAMP_DOWN_PER_SEC * dt;
        if (gSimSpeed < 0.0f) {
            gSimSpeed = 0.0f;
        }
    }
    if (sKey || gSimSpeed > 0.0f) {
        data.speed = static_cast<uint8_t>(std::lround(gSimSpeed));
    }

    if (tKey && !gPrevTKey) {
        gSimTemp = static_cast<float>(data.coolantTemp);
    }
    if (tKey) {
        gSimTemp += TEMP_RAMP_UP_PER_SEC * dt;
        if (gSimTemp > TEMP_MAX) {
            gSimTemp = TEMP_MAX;
        }
    } else if (gSimTemp > TEMP_MIN) {
        gSimTemp -= TEMP_RAMP_DOWN_PER_SEC * dt;
        if (gSimTemp < TEMP_MIN) {
            gSimTemp = TEMP_MIN;
        }
    }
    if (tKey || gSimTemp > TEMP_MIN) {
        data.coolantTemp = static_cast<uint8_t>(std::lround(gSimTemp));
    }

    gPrevRKey = rKey;
    gPrevSKey = sKey;
    gPrevTKey = tKey;
}

void processKeyboardShortcuts(bool& autoCycleEnabled) {
    SDL_PumpEvents();
    const Uint8* state = SDL_GetKeyboardState(nullptr);
    static bool prevKeys[3] = { false, false, false };
    const bool keys[3] = {
        state[SDL_SCANCODE_1] != 0,
        state[SDL_SCANCODE_2] != 0,
        state[SDL_SCANCODE_3] != 0,
    };

    if (keys[0] && !prevKeys[0]) {
        DashboardUI::switchScreen(DashboardScreen::MAIN);
        autoCycleEnabled = false;
    } else if (keys[1] && !prevKeys[1]) {
        DashboardUI::switchScreen(DashboardScreen::SETTINGS);
        autoCycleEnabled = false;
    } else if (keys[2] && !prevKeys[2]) {
        DashboardUI::switchScreen(DashboardScreen::DIAGNOSTICS);
        autoCycleEnabled = false;
    }

    for (size_t i = 0; i < 3; ++i) {
        prevKeys[i] = keys[i];
    }
}

} // namespace

int main() {
    lv_init();

    lv_display_t * disp = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (disp == nullptr) {
        return 1;
    }

    lv_display_set_default(disp);
    lv_sdl_window_set_title(disp, "Honda OBD1 Dashboard (Simulator)");
    lv_sdl_window_set_resizeable(disp, true);

    lv_sdl_mouse_create();
#if LV_SDL_MOUSEWHEEL_MODE == LV_SDL_MOUSEWHEEL_MODE_ENCODER
    lv_sdl_mousewheel_create();
#endif
    lv_sdl_keyboard_create();

    DashboardUI::init();
    DashboardUI::setClearDtcCallback(onClearDtcRequest);

    OBD1Data demoData{};
    uint32_t elapsedMs = 0;
    uint32_t lastTick = SDL_GetTicks();
    size_t lastSampleIndex = OBD1_SAMPLE_COUNT;
    bool autoCycleEnabled = true;

    while (lv_display_get_default() != nullptr) {
        uint32_t now = SDL_GetTicks();
        uint32_t delta = now - lastTick;
        lastTick = now;
        elapsedMs += delta;
        gElapsedMs = elapsedMs;

        const size_t sampleIndex = (elapsedMs / SAMPLE_HOLD_MS) % OBD1_SAMPLE_COUNT;
        populateDemoData(demoData, elapsedMs, sampleIndex);
        const uint8_t fuelLevel = static_cast<uint8_t>(100 - ((elapsedMs / 200) % 101));
        applyKeyOverrides(demoData, delta);

        DashboardUI::update(demoData);
        DashboardUI::setFuelLevel(fuelLevel);
        processKeyboardShortcuts(autoCycleEnabled);
        updateScreenCycle(elapsedMs, autoCycleEnabled);

        if (sampleIndex != lastSampleIndex) {
            char title[128];
            std::snprintf(title,
                          sizeof(title),
                          "Honda OBD1 Dashboard (Simulator) - %s",
                          OBD1_SAMPLE_NAMES[sampleIndex]);
            lv_sdl_window_set_title(disp, title);
            lastSampleIndex = sampleIndex;
        }

        lv_timer_handler();
        SDL_Delay(16);
    }

    lv_sdl_quit();
    return 0;
}
