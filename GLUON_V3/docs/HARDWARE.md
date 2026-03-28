# Hardware Guide

> Choosing and setting up hardware for Gluon V3 nodes.

## Supported Boards

Gluon V3 currently supports two board families. Both use the **ESP32-S3** MCU and the **Semtech SX1262** LoRa transceiver — the difference is form factor, integration level, and how LoRa is connected.

---

### Option A: Seeedstudio XIAO ESP32S3 + Wio-SX1262

**The modular approach: smallest, cheapest, Grove-based.**

| Spec        | Value                                               |
| ----------- | --------------------------------------------------- |
| MCU         | ESP32-S3 (Xtensa dual-core LX7, 240 MHz)            |
| Flash / RAM | 8MB / 512KB SRAM + 8MB PSRAM                        |
| LoRa        | Wio-SX1262 module (separate board, runs Meshtastic) |
| Display     | None on XIAO; 128×64 OLED on Expansion Board        |
| Dimensions  | 21 × 17.8 mm (XIAO alone)                           |
| USB         | USB-C (native USB on ESP32-S3)                      |
| Battery     | Via Expansion Board (JST connector)                 |
| Price       | ~$9.90 (XIAO + Wio-SX1262 kit)                      |
| Expansion   | Grove connectors via Expansion Board                |

**How it works:** The XIAO runs the Gluon firmware. The Wio-SX1262 runs Meshtastic independently as a "sidecar" LoRa radio. They communicate over Serial1 (UART). The XIAO has 11 user GPIO pins; with the Expansion Board, you get an OLED, a button, a buzzer, and several Grove connectors.

**Best for:**

- Mass production (tiny, cheap, Seeedstudio partnership)
- STEM education kits
- Ultra-compact nodes
- Projects that need Grove sensor ecosystem

**PlatformIO target:** `pio run -e xiao_esp32s3`

**Accessories:**

- [XIAO Expansion Board](https://wiki.seeedstudio.com/Seeeduino-XIAO-Expansion-Board/) ($16.40) — OLED, button, buzzer, battery holder, Grove connectors
- [Grove sensors/actuators](https://wiki.seeedstudio.com/Grove_System/) — Plug-and-play 4-pin modules

---

### Option B: Heltec WiFi LoRa 32 V3 / V4

**The all-in-one approach: everything on a single board.**

| Spec        | V3                                    | V4                                    |
| ----------- | ------------------------------------- | ------------------------------------- |
| MCU         | ESP32-S3FN8                           | ESP32-S3R2                            |
| Flash / RAM | 8MB / 512KB SRAM                      | 16MB / 512KB SRAM + 2MB PSRAM         |
| LoRa        | On-board SX1262                       | On-board SX1262                       |
| TX Power    | 21 ± 1 dBm                            | 28 ± 1 dBm (high-power version)       |
| Display     | Built-in 0.96" OLED (SSD1306, 128×64) | Built-in 0.96" OLED (SSD1306, 128×64) |
| Dimensions  | 50.2 × 25.4 × 10.7 mm                 | 51.7 × 25.4 × 10.7 mm                 |
| USB         | USB-C (via CP2102)                    | USB-C (native USB, no CP2102)         |
| Battery     | JST 1.25mm 2-pin                      | JST 1.25mm 2-pin + solar input        |
| GNSS        | None                                  | JST 1.25mm 8-pin GNSS header          |
| User GPIO   | 18 accessible pins                    | 20 accessible pins                    |
| Price       | ~$18                                  | ~$18-20                               |
| Meshtastic  | Native support                        | Native support                        |

**How it works:** The ESP32-S3 and SX1262 are on the same board. The LoRa radio is connected via SPI (GPIOs 8-14). The OLED is connected via I2C (GPIOs 17-18). Meshtastic can run natively on this chip. The board exposes many more GPIO pins than the XIAO for connecting sensors and actuators directly.

**V3 vs V4:** V4 is a drop-in upgrade — same pinout, same PlatformIO board target. V4 adds more TX power (28 vs 21 dBm), more memory (16MB Flash + 2MB PSRAM), solar input, and a GNSS header. **If buying new, get V4.**

**Best for:**

- Prototyping and development (all-in-one, no wiring)
- Long-range deployments (28 dBm on V4)
- Outdoor/solar-powered installations
- Projects that need GPS tracking
- Meshtastic/MeshCore experimentation

**PlatformIO target:** `pio run -e heltec_v3`

**Accessories:**

- [WiFi LoRa 32 Expansion Kit](https://heltec.org/project/wifi-lora-32-v4-expansion-housing/) — Enclosure with extra I/O
- [Solar Kit](https://heltec.org/project/solar-kit-for-dev-board-waterproof-enclosure-for-outdoor-meshtastic-meshcore/) — Waterproof outdoor enclosure
- [L76K GNSS Module](https://heltec.org/project/l76-gnss-module/) — GPS for V4's GNSS header

---

## Pin Comparison

Both boards use ESP32-S3 but allocate GPIO differently. The `BoardPins.h` header abstracts this:

| Function            | XIAO ESP32S3                     | Heltec V3/V4                                                            |
| ------------------- | -------------------------------- | ----------------------------------------------------------------------- |
| **OLED SDA**        | GPIO 5 (via Expansion Board)     | GPIO 17                                                                 |
| **OLED SCL**        | GPIO 6 (via Expansion Board)     | GPIO 18                                                                 |
| **OLED RST**        | None (-1)                        | GPIO 21                                                                 |
| **LoRa**            | External (Serial1: TX=43, RX=44) | On-board SPI (NSS=8, SCK=9, MOSI=10, MISO=11, RST=12, BUSY=13, DIO1=14) |
| **LED**             | GPIO 21                          | GPIO 35                                                                 |
| **Button**          | GPIO 2 (Expansion Board)         | GPIO 0 (PRG button)                                                     |
| **Vext**            | None                             | GPIO 36 (power to OLED/sensors)                                         |
| **Serial1 TX/RX**   | GPIO 43 / 44 (to LoRa sidecar)   | GPIO 43 / 44 (available for GNSS)                                       |
| **User GPIO count** | 4 (GPIOs 1-4)                    | 17 (GPIOs 1-7, 19, 20, 33, 34, 38, 39, 45-48)                           |
| **User ADC count**  | 2 (GPIOs 1-2)                    | 7 (GPIOs 1-7)                                                           |

## Which Board Should I Pick?

```
                                  ┌───────────────────────┐
                                  │ Do you need LoRa for  │
                                  │ long-range mesh?      │
                                  └─────────┬─────────────┘
                                     Yes    │    No (WiFi/BLE only)
                                            │    → Either works, XIAO is cheaper
                              ┌─────────────▼────────────────┐
                              │ Need lots of GPIO pins       │
                              │ for sensors/actuators?       │
                              └─────────┬──────────┬─────────┘
                                Yes     │          │  No (1-2 sensors is enough)
                                        │          │
                          ┌─────────────▼──┐  ┌────▼─────────────────────┐
                          │  Heltec V3/V4  │  │  XIAO ESP32S3            │
                          │  17 user GPIO  │  │  Smallest, cheapest      │
                          │  Built-in OLED │  │  Grove ecosystem         │
                          │  28dBm (V4)    │  │  Best for mass production│
                          └────────────────┘  └──────────────────────────┘
```

## LoRa Transport Modes

The transport architecture depends on your board:

### XIAO: Meshtastic Sidecar Pattern

```
┌──────────────┐   Serial1 (UART)   ┌──────────────┐
│  XIAO ESP32S3│◄──────────────────►│ Wio-SX1262   │
│  (Gluon FW)  │   115200 baud      │ (Meshtastic) │
└──────────────┘                     └──────────────┘
                                           │
                                      LoRa antenna
                                           │
                                     ~~~~ mesh ~~~~
```

The XIAO runs Gluon. The Wio-SX1262 runs Meshtastic. They talk over UART using a framed binary protocol. Meshtastic handles all radio, routing, encryption, and mesh duties.

### Heltec: Native Radio (future)

```
┌────────────────────────────────────────┐
│  Heltec V3/V4                          │
│  ┌──────────┐    SPI     ┌──────────┐ │
│  │ ESP32-S3 │◄──────────►│  SX1262  │ │
│  │ (Gluon)  │            │ (on-PCB) │ │
│  └──────────┘            └──────────┘ │
└────────────────────────────────────────┘
                                │
                           LoRa antenna
                                │
                          ~~~~ mesh ~~~~
```

On Heltec, the SX1262 is directly wired to the ESP32's SPI bus. Currently, Gluon uses the same MeshtasticSerialTransport (you'd run Meshtastic on a second Heltec as a bridge). In the future, a `RadioLibTransport` implementation will drive the SX1262 directly via RadioLib, enabling native mesh without a sidecar.

## Power Considerations

| Scenario                | XIAO                | Heltec V3/V4        |
| ----------------------- | ------------------- | ------------------- |
| Active (WiFi + LoRa TX) | ~120 mA             | ~150 mA             |
| CPU active, radio idle  | ~40 mA              | ~40 mA              |
| Deep sleep              | ~14 µA              | ~20 µA (V4)         |
| Battery connector       | Via Expansion Board | Built-in JST 1.25mm |
| Solar input             | No                  | V4 only (4.7-6V)    |

**Heltec Vext pin:** GPIO 36 controls power to the OLED and external sensor rail. Pull LOW to turn ON, HIGH to turn OFF. Use this to save power when the display isn't needed.

## Antenna Notes

- **LoRa antenna**: Both boards use SMA or IPEX/U.FL connectors. **Always connect an antenna before transmitting** — operating without an antenna can damage the SX1262.
- **Frequency bands**: Match your antenna to your region (868 MHz for EU, 915 MHz for US/AU, 433 MHz for some regions).
- **WiFi/BLE antenna**: XIAO has a built-in PCB antenna. Heltec V4 has FPC + IPEX connector.

## Future Hardware Targets

The board abstraction in `BoardPins.h` makes it straightforward to add support for:

- **M5Stack AtomS3** — Tiny ESP32-S3. No LoRa, but great for WiFi/BLE-only nodes.
- **LilyGO T-Deck** — ESP32-S3 + SX1262 + keyboard + screen. Excellent for a "master controller" node.
- **Heltec Wireless Paper** — E-ink display for persistent labels.
- **Custom PCB** — The ultimate goal for mass production with Seeedstudio.
