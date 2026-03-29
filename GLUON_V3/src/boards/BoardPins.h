#pragma once
// ==========================================================================
//  BoardPins.h — Hardware abstraction for supported boards
//
//  Each board defines:
//    - OLED display pins (I2C address, SDA, SCL, RST)
//    - LoRa radio pins (NSS, RST, DIO1/DIO0, BUSY, SPI bus)
//    - User-accessible GPIO for sensors/actuators
//    - LED, button, power control (Vext)
//    - Whether LoRa is on-board or an external sidecar
//
//  To add a new board:
//    1. Add a new #elif block below
//    2. Add a [env:xxx] section in platformio.ini
//    3. Add -DGLUON_BOARD_XXX to build_flags
// ==========================================================================

#include <Arduino.h>

// =========================================================================
//  Board: Seeedstudio XIAO ESP32S3 + Wio-SX1262 Kit
//
//  The XIAO has very few exposed pins (11 GPIO). LoRa is a separate
//  module running Meshtastic, connected via UART (Serial1).
//  Display requires the XIAO Expansion Board (SSD1306 on pins 5/6).
//
//  Pin reference: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
// =========================================================================
#if defined(GLUON_BOARD_XIAO_ESP32S3)

namespace gluon::board {

    // --- Identity ---
    constexpr const char* BOARD_NAME = "XIAO ESP32S3";

    // --- LoRa transport mode ---
    // XIAO uses an external Meshtastic node over Serial1 (sidecar pattern)
    constexpr bool LORA_ONBOARD    = false;
    constexpr int  SERIAL1_TX      = 43;   // D6 on XIAO
    constexpr int  SERIAL1_RX      = 44;   // D7 on XIAO
    constexpr uint32_t SERIAL1_BAUD = 115200;

    // --- OLED Display (XIAO Expansion Board: SSD1306 128x64, I2C) ---
    constexpr bool DISPLAY_PRESENT = true;  // true if Expansion Board attached
    constexpr uint8_t OLED_ADDR    = 0x3C;
    constexpr int  OLED_SDA        = 5;    // D4/SDA
    constexpr int  OLED_SCL        = 6;    // D5/SCL
    constexpr int  OLED_RST        = -1;   // no reset pin on Expansion Board

    // --- Built-in LED ---
    constexpr int  LED_PIN         = 21;

    // --- User button (XIAO Expansion Board has one on D1) ---
    constexpr int  BUTTON_PIN      = 2;    // D1 — user button on Expansion Board
    constexpr bool BUTTON_ACTIVE_LOW = true;

    // --- Power control ---
    constexpr int  VEXT_PIN        = -1;   // no Vext on XIAO

    // --- Common interaction hardware (V2 common objects) ---
    // On XIAO, these share the limited USER_GPIO via Grove connectors.
    // Assign based on your wiring; -1 = not connected (disabled).
    constexpr int  CHIRP_PIN       = 3;    // D2 — piezo speaker
    constexpr int  VIBRATOR_PIN    = 4;    // D3 — vibration motor (PWM)
    constexpr int  TILT_PIN        = 1;    // D0 — tilt/shake sensor (interrupt)
    constexpr int  NEOPIXEL_PIN    = 2;    // D1 — NeoPixel LED strip (4 LEDs)
    constexpr int  LEARNING_BTN_PIN = -1;  // analog button (not wired by default)
    constexpr uint8_t VIBRATOR_LEDC_CH = 4; // LEDC PWM channel for vibrator

    // --- IR proximity (connection discovery) ---
    // XIAO has very limited pins. Assign from Grove connectors.
    // Use a 38kHz IR receiver (e.g. TSOP38238) and an IR LED.
    // Set to -1 if not wired (IR proximity disabled).
    constexpr int  IR_SEND_PIN     = -1;   // IR LED (38kHz via RMT) — assign from Grove
    constexpr int  IR_RECV_PIN     = -1;   // IR demodulator output — assign from Grove

    // --- User GPIO (remaining pins for sensors/actuators via Grove) ---
    // XIAO Expansion Board exposes: D0(1), D1(2), D2(3), D3(4), A0(1), A1(2)
    // (D4/D5 used by I2C, D6/D7 used by Serial1 to LoRa)
    // NOTE: Some USER_GPIO overlap with interaction pins above.
    // The JSON config should only reference pins NOT used by interaction hardware.
    constexpr int  USER_GPIO[] = { 1, 2, 3, 4 };
    constexpr int  USER_ADC[]  = { 1, 2 };
    constexpr int  NUM_USER_GPIO = 4;
    constexpr int  NUM_USER_ADC  = 2;
}

// =========================================================================
//  Board: Heltec WiFi LoRa 32 V3 / V4
//
//  All-in-one board: ESP32-S3 + SX1262 + 0.96" OLED.
//  V4 adds: 28dBm TX, 16MB Flash, 2MB PSRAM, solar input, GNSS header.
//  V3 and V4 are pin-compatible (use same PlatformIO board).
//
//  The LoRa radio is on-board, sharing the ESP32-S3's SPI bus.
//  Meshtastic can run natively on this same chip (not sidecar).
//
//  Pin reference: Heltec V3 pins_arduino.h in PlatformIO
//    OLED: SDA=17, SCL=18, RST=21 (I2C, SSD1306 128x64)
//    LoRa: NSS=8, RST=12, BUSY=13, DIO1=14, SPI(SCK=9, MOSI=10, MISO=11)
//    Vext=36 (external power rail control for OLED/sensors)
//    LED=35, PRG button=0
// =========================================================================
#elif defined(GLUON_BOARD_HELTEC_V3)

namespace gluon::board {

    // --- Identity ---
    constexpr const char* BOARD_NAME = "Heltec LoRa32 V3/V4";

    // --- LoRa transport mode ---
    // Heltec has SX1262 on-board, sharing this ESP32's SPI
    constexpr bool LORA_ONBOARD    = true;

    // LoRa SPI pins (directly connected on the PCB)
    constexpr int  LORA_NSS        = 8;    // SPI SS / Chip Select
    constexpr int  LORA_SCK        = 9;
    constexpr int  LORA_MOSI       = 10;
    constexpr int  LORA_MISO       = 11;
    constexpr int  LORA_RST        = 12;
    constexpr int  LORA_BUSY       = 13;
    constexpr int  LORA_DIO1       = 14;   // Interrupt line from SX1262

    // Serial1 still available for GNSS or external devices
    constexpr int  SERIAL1_TX      = 43;
    constexpr int  SERIAL1_RX      = 44;
    constexpr uint32_t SERIAL1_BAUD = 9600; // Typical for GNSS

    // --- OLED Display (on-board SSD1306 128x64, I2C) ---
    constexpr bool DISPLAY_PRESENT = true;
    constexpr uint8_t OLED_ADDR    = 0x3C;
    constexpr int  OLED_SDA        = 17;
    constexpr int  OLED_SCL        = 18;
    constexpr int  OLED_RST        = 21;

    // --- Built-in LED ---
    constexpr int  LED_PIN         = 35;

    // --- User button (PRG button on Heltec, active low) ---
    constexpr int  BUTTON_PIN      = 0;
    constexpr bool BUTTON_ACTIVE_LOW = true;

    // --- Vext power control (controls power to OLED and external sensors) ---
    constexpr int  VEXT_PIN        = 36;   // LOW = ON, HIGH = OFF

    // --- Common interaction hardware (V2 common objects) ---
    // Heltec has many more GPIO. Pick from the header pins.
    constexpr int  CHIRP_PIN       = 19;   // piezo speaker
    constexpr int  VIBRATOR_PIN    = 20;   // vibration motor (PWM)
    constexpr int  TILT_PIN        = 33;   // tilt/shake sensor (interrupt)
    constexpr int  NEOPIXEL_PIN    = 34;   // NeoPixel LED strip (4 LEDs)
    constexpr int  LEARNING_BTN_PIN = 1;   // analog button (ADC1 ch0)
    constexpr uint8_t VIBRATOR_LEDC_CH = 4; // LEDC PWM channel for vibrator

    // --- IR proximity (connection discovery) ---
    // Uses 38kHz IR receiver (TSOP38238) and IR LED via RMT peripheral.
    // V2 used PD3/INT1 for receive and PB1/OC1A for send.
    constexpr int  IR_SEND_PIN     = 38;   // IR LED (38kHz carrier via RMT)
    constexpr int  IR_RECV_PIN     = 39;   // IR demodulator (TSOP38238) output

    // --- User GPIO (exposed on header pins, not used by LoRa/OLED) ---
    // Available: GPIO 1-7, 19, 20, 33, 34, 38, 39, 45, 46, 47, 48
    // (GPIO 8-14 used by LoRa SPI, 17-18 by OLED, 21 OLED RST)
    // NOTE: Some overlap with interaction pins above — JSON config should
    // only reference pins NOT used by interaction hardware.
    constexpr int  USER_GPIO[] = { 1, 2, 3, 4, 5, 6, 7, 19, 20, 33, 34, 38, 39, 45, 46, 47, 48 };
    constexpr int  USER_ADC[]  = { 1, 2, 3, 4, 5, 6, 7 };
    constexpr int  NUM_USER_GPIO = 17;
    constexpr int  NUM_USER_ADC  = 7;
}

// =========================================================================
//  Fallback: unknown board — compile with a helpful error
// =========================================================================
#else
    #error "No board defined! Add -DGLUON_BOARD_XIAO_ESP32S3 or -DGLUON_BOARD_HELTEC_V3 to build_flags in platformio.ini"
#endif
