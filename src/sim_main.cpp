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

#include <cmath>

namespace {

void populateDemoData(OBD1Data &data, uint32_t elapsedMs) {
    const float t = static_cast<float>(elapsedMs) / 1000.0f;

    data.connected = true;
    data.timestamp = elapsedMs;

    // Cycle RPM and speed with sine waves to show different ranges
    const float rpmWave = 0.5f + 0.5f * std::sin(t * 1.2f);
    const float speedWave = 0.5f + 0.5f * std::sin(t * 0.5f);
    const float tempWave = 0.5f + 0.5f * std::sin(t * 0.3f);
    const float throttleWave = 0.5f + 0.5f * std::sin(t * 1.7f);
    const float voltageWave = 0.5f + 0.5f * std::sin(t * 0.2f);

    data.rpm = static_cast<uint16_t>(800 + rpmWave * (RPM_MAX - 800));
    data.speed = static_cast<uint8_t>(speedWave * SPEED_MAX);
    data.coolantTemp = static_cast<uint8_t>(TEMP_MIN + tempWave * (TEMP_MAX - TEMP_MIN));
    data.intakeTemp = data.coolantTemp - 5;
    data.throttlePosition = static_cast<uint8_t>(throttleWave * 100.0f);
    data.batteryVoltage = static_cast<uint8_t>((13.2f + voltageWave * 1.0f) * 10.0f);
    data.mapPressure = static_cast<uint16_t>(30.0f + speedWave * 70.0f);
    data.fuelTrim = static_cast<uint8_t>(rpmWave * 100.0f);
    data.ignitionAdvance = static_cast<uint8_t>(10.0f + (1.0f - rpmWave) * 25.0f);
    data.injectorPulse = static_cast<uint8_t>(20.0f + rpmWave * 80.0f);
    data.checkEngine = (elapsedMs / 8000) % 4 == 3;
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

    OBD1Data demoData{};
    uint32_t elapsedMs = 0;
    uint32_t lastTick = SDL_GetTicks();

    while (lv_display_get_default() != nullptr) {
        uint32_t now = SDL_GetTicks();
        uint32_t delta = now - lastTick;
        lastTick = now;
        elapsedMs += delta;

        populateDemoData(demoData, elapsedMs);
        const uint8_t fuelLevel = static_cast<uint8_t>(100 - ((elapsedMs / 200) % 101));

        DashboardUI::update(demoData);
        DashboardUI::setFuelLevel(fuelLevel);

        lv_timer_handler();
        SDL_Delay(16);
    }

    lv_sdl_quit();
    return 0;
}
