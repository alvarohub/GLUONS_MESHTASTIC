# GLUON V3 — Decentralized Dataflow Mesh Nodes

> Physical computing modules that snap together wirelessly to form dataflow programs.

Gluon nodes are small, self-contained computing units — each with sensors, actuators, and a logic core — that communicate over a LoRa mesh network. Connect them by proximity (no code, no wires), and data flows between them like a visual programming language made physical.

**Think of it as Max/MSP or Pure Data, but in hardware, over a city-scale mesh.**

## What is a Gluon?

Each Gluon node has:

- **Inlets** — Data input ports (receive from other nodes)
- **Outlets** — Data output ports (send to other nodes)
- **Sensors** — Read the physical world (buttons, knobs, light, distance, temperature…)
- **Actuators** — Act on the world (LEDs, buzzers, servos, displays…)
- **Logic** — A processing core that transforms inputs to outputs (passthrough, OR, AND, counter, metronome, threshold…)

Nodes discover each other over the mesh and form connections that persist across power cycles. The entire behavior of a node is configured via a JSON file — no recompilation needed.

## Quick Start

### 1. Install PlatformIO

```bash
# VS Code: install the PlatformIO IDE extension
# Or via pip:
pip install platformio
```

### 2. Build for your hardware

```bash
# For Seeedstudio XIAO ESP32S3 + Wio-SX1262:
pio run -e xiao_esp32s3

# For Heltec WiFi LoRa 32 V3/V4:
pio run -e heltec_v3

# Build all targets:
pio run
```

### 3. Configure your node

Edit `data/config.json` to define what your node does. See `data/examples/` for ready-made configurations.

### 4. Flash

```bash
# Upload firmware:
pio run -e xiao_esp32s3 -t upload

# Upload config to LittleFS:
pio run -e xiao_esp32s3 -t uploadfs
```

### 5. Monitor

```bash
pio device monitor
# Type 'help' for serial commands: status, connections, reset, types
```

## Supported Hardware

| Board                                     | MCU        | LoRa                                 | Display             | Price   | Notes                                   |
| ----------------------------------------- | ---------- | ------------------------------------ | ------------------- | ------- | --------------------------------------- |
| **Seeedstudio XIAO ESP32S3 + Wio-SX1262** | ESP32-S3   | External SX1262 (Meshtastic sidecar) | Via Expansion Board | ~$10-26 | Smallest, cheapest, Grove ecosystem     |
| **Heltec WiFi LoRa 32 V3**                | ESP32-S3   | On-board SX1262                      | Built-in 0.96" OLED | ~$18    | All-in-one, 21dBm                       |
| **Heltec WiFi LoRa 32 V4**                | ESP32-S3R2 | On-board SX1262                      | Built-in 0.96" OLED | ~$18-20 | V3-compatible, 28dBm, solar input, GNSS |

See [docs/HARDWARE.md](docs/HARDWARE.md) for detailed hardware information and comparison.

## Project Structure

```
GLUON_V3/
  platformio.ini              # Multi-board build configuration
  README.md                   # This file
  docs/
    ARCHITECTURE.md           # Software architecture deep-dive
    HARDWARE.md               # Hardware guide and board comparison
    CREATING_A_NODE.md        # Step-by-step guide to building a node
  src/
    main.cpp                  # Entry point (~130 lines for any node type)
    boards/
      BoardPins.h             # Per-board pin definitions
    core/
      GluonTypes.h            # Data, Link, Message, UpdateMode
      Inlet.h                 # Data input ports
      Outlet.h                # Data output ports
      BaseLogic.h             # Abstract logic with update modes
      BaseSensor.h            # Abstract sensor with event detection
      BaseActuator.h          # Abstract actuator with mode control
      GluonNode.h             # Main node orchestrator
    transport/
      MeshTransport.h         # Abstract mesh transport + implementations
    config/
      GluonConfig.h           # JSON config loader from LittleFS
    factory/
      ComponentFactory.h      # Runtime factory + self-registration macros
    modules/
      BuiltinSensors.h        # Digital, analog, I2C sensors
      BuiltinActuators.h      # Digital, PWM, display actuators
      BuiltinLogics.h         # Passthrough, OR, AND, counter, metro, threshold
  data/
    config.json               # Default node config (flashed to LittleFS)
    examples/                 # Example configurations for common node types
```

## Built-in Components

### Sensors

| Type          | JSON `"type"` | Description                                 |
| ------------- | ------------- | ------------------------------------------- |
| Digital input | `"switch"`    | Button, switch, tilt, PIR (with debounce)   |
| Analog input  | `"analog"`    | Potentiometer, LDR, thermistor, Sharp IR    |
| I2C sensor    | `"i2c"`       | Grove I2C devices (placeholder for drivers) |

### Actuators

| Type           | JSON `"type"` | Description                                   |
| -------------- | ------------- | --------------------------------------------- |
| Digital output | `"digital"`   | LED, buzzer, relay (steady or one-shot pulse) |
| PWM output     | `"pwm"`       | LED brightness, motor speed, servo angle      |
| Display        | `"display"`   | Virtual actuator for screen rendering         |

### Logic Modules

| Type        | JSON `"logic"`  | Description                                   |
| ----------- | --------------- | --------------------------------------------- |
| Passthrough | `"passthrough"` | Sensor → outlet (transparent relay)           |
| OR gate     | `"or_gate"`     | First inlet with data wins                    |
| AND gate    | `"and_gate"`    | Output only when ALL inlets have events       |
| Counter     | `"counter"`     | Count events, overflow at threshold           |
| Metronome   | `"metro"`       | Clock/beat generator (toggle on/off, set BPM) |
| Threshold   | `"threshold"`   | Schmitt trigger with hysteresis               |

## Example Configurations

A **Metro node** (metronome that sends beats to the mesh):

```json
{
  "name": "METRO-01",
  "logic": "metro",
  "updateMode": 5,
  "sensors": [{ "type": "switch", "name": "TAP", "pin": 2, "pullup": true }],
  "actuators": [
    { "type": "digital", "name": "BEAT", "pin": 3, "mode": "pulse" },
    { "type": "digital", "name": "RUN", "pin": 4, "mode": "normal" },
    { "type": "display", "name": "BPM" }
  ]
}
```

See `data/examples/` for more: rangefinder, switch+RGB, counter, smart city AND gate.

## History

Gluons were originally created in 2015 by Alvaro Cassinelli as a tangible programming system for STEM education and interactive art. The original hardware was ATmega328-based (Moteino R4 + RFM69W at 433 MHz), with point-to-point radio and compile-time configured modules.

V3 is a complete rewrite for ESP32-S3 + LoRa mesh networking, transforming Gluons from a classroom tool into a platform capable of smart city-scale deployments — while preserving the core "connect by proximity, no coding needed" philosophy.

Key evolution:

- ATmega328 (32KB Flash, 2KB RAM) → ESP32-S3 (16MB Flash, 512KB SRAM, 2MB PSRAM)
- RFM69 point-to-point 433 MHz → SX1262 LoRa mesh via Meshtastic
- Compile-time `#define GLUON` → Runtime JSON configuration
- Fixed 1KB EEPROM → LittleFS filesystem + NVS Preferences
- 1 sensor + 1 actuator type per build → Any mix via factory pattern

## License

TBD

## Contributing

This project is in active development. See [docs/CREATING_A_NODE.md](docs/CREATING_A_NODE.md) to learn how to add custom sensor, actuator, or logic modules.
