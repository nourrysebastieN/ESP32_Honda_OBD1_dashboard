# ESP32-S3 Honda OBD1 Dashboard

A modern automotive dashboard project for Honda vehicles with OBD1 ECU, built using ESP32-S3 and a 7" LVGL-powered display.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-green.svg)
![LVGL](https://img.shields.io/badge/LVGL-v9.2-orange.svg)

## 📋 Features

- **Real-time Engine Monitoring**
  - RPM gauge with redline indicator
  - Digital speedometer
  - Coolant temperature with warning
  - Throttle position
  - Battery voltage
  - Check engine light indicator

- **Modern UI Design**
  - Dark theme optimized for automotive use
  - Smooth animations
  - Touch-enabled navigation
  - Multiple screen views (Main, Detailed, Settings, Diagnostics)

- **Honda OBD1 Support**
  - Compatible with Honda OBD1 ECUs (1992-1995)
  - Real-time data polling
  - ECU connection status indicator

## 🛠️ Hardware Requirements

- **MCU**: ESP32-S3 DevKitC-1 (or similar ESP32-S3 board with PSRAM)
- **Display**: 7" TFT LCD (800x480 resolution)
- **Touch**: Capacitive touch panel (GT911 controller)
- **ECU Interface**: Honda OBD1 diagnostic connector

### Recommended Display Modules

- Waveshare 7" ESP32-S3 Display
- Elecrow 7" ESP32-S3 Terminal
- Generic 7" ILI9488/ST7262 with GT911 touch

## 📁 Project Structure

```
ESP32_Honda_OBD1_dashboard/
├── include/
│   ├── config.h           # Hardware configuration & pin definitions
│   ├── dashboard_ui.h     # UI component declarations
│   ├── display_driver.h   # Display driver interface
│   ├── lv_conf.h          # LVGL configuration
│   └── obd1_handler.h     # OBD1 communication handler
├── lib/                   # Custom libraries (if any)
├── src/
│   ├── main.cpp           # Application entry point
│   ├── dashboard_ui.cpp   # UI implementation
│   ├── display_driver.cpp # Display driver implementation
│   └── obd1_handler.cpp   # OBD1 protocol implementation
├── platformio.ini         # PlatformIO configuration
└── README.md             # This file
```

## 🚀 Getting Started

### Prerequisites

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)

### Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/ESP32_Honda_OBD1_dashboard.git
   ```

2. Open the project in VS Code with PlatformIO

3. Configure your hardware:
   - Edit `include/config.h` to match your pin connections
   - Modify `src/display_driver.cpp` for your specific display panel

4. Build and upload:
   ```bash
   pio run -t upload
   ```

5. Open serial monitor to see debug output:
   ```bash
   pio device monitor
   ```

## ⚙️ Configuration

### Display Configuration (`include/config.h`)

Adjust these values based on your display:

```cpp
// Display dimensions
#define DISPLAY_WIDTH  800
#define DISPLAY_HEIGHT 480

// SPI pins
#define TFT_MOSI 11
#define TFT_MISO 13
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   46
```

### OBD1 Configuration

```cpp
// OBD1 serial pins
#define OBD1_RX_PIN 44
#define OBD1_TX_PIN 43

// Baud rate (9600 for most Honda ECUs)
#define OBD1_BAUD_RATE 9600
```

## 🔌 Wiring

### OBD1 Connector Pinout

| Pin | Description | ESP32 Pin |
|-----|-------------|-----------|
| 1   | Ground      | GND       |
| 2   | +12V        | VIN       |
| 3   | K-Line      | GPIO44 (RX) |
| 4   | K-Line      | GPIO43 (TX) |

*Note: A level shifter may be required for the K-Line interface*

## 🎮 Debug Commands

When connected via serial monitor, use these commands:

| Key | Action |
|-----|--------|
| 1   | Switch to Main screen |
| 2   | Switch to Detailed screen |
| 3   | Switch to Settings screen |
| 4   | Switch to Diagnostics screen |
| r   | Simulate random RPM |
| s   | Simulate random speed |
| t   | Simulate random temperature |
| i   | Print system info |
| h/? | Show help |

## 🎨 Customization

### Adding New Gauges

Use the helper functions in `dashboard_ui.h`:

```cpp
// Create a new gauge
lv_obj_t* myGauge = DashboardUI::createGauge(parent, 0, 100, 80);

// Create a bar indicator
lv_obj_t* myBar = DashboardUI::createBarIndicator(parent, "LABEL", 0, 100);
```

### Changing Colors

Edit the color definitions in `dashboard_ui.cpp`:

```cpp
static const lv_color_t COLOR_BG = lv_color_hex(0x1A1A2E);
static const lv_color_t COLOR_PRIMARY = lv_color_hex(0x16213E);
static const lv_color_t COLOR_ACCENT = lv_color_hex(0x0F3460);
static const lv_color_t COLOR_HIGHLIGHT = lv_color_hex(0xE94560);
```

## 📊 OBD1 Parameters

The following parameters are read from the Honda OBD1 ECU:

| Parameter | Unit | Description |
|-----------|------|-------------|
| RPM | rpm | Engine speed |
| Speed | km/h | Vehicle speed |
| Coolant Temp | °C | Engine coolant temperature |
| Intake Temp | °C | Intake air temperature |
| MAP | kPa | Manifold absolute pressure |
| TPS | % | Throttle position |
| O2 Voltage | V | Oxygen sensor voltage |
| Battery | V | Battery voltage |

## 🐛 Troubleshooting

### Display not working
- Check SPI connections
- Verify display type in `display_driver.cpp`
- Check backlight pin connection

### Touch not responding
- Verify I2C connections for GT911
- Check touch interrupt pin
- Ensure correct I2C address (0x5D or 0x14)

### No ECU connection
- Verify RX/TX pin connections
- Check baud rate setting
- Ensure ignition is ON
- Verify OBD1 connector wiring

## 📝 TODO

- [ ] Add more diagnostic screens
- [ ] Implement DTC reading/clearing
- [ ] Add data logging to SD card
- [ ] Create custom gauge graphics
- [ ] Add trip computer functionality
- [ ] Implement settings persistence (NVS)
- [ ] Add CAN bus support for newer vehicles

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- [LVGL](https://lvgl.io/) - Light and Versatile Graphics Library
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) - Display library for ESP32
- [PlatformIO](https://platformio.org/) - Build system
- Honda ECU documentation contributors

## 📬 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request