# Software Architecture

> How the Gluon V3 firmware is structured and why.

## Overview

Gluon V3 is a dataflow engine. Each node is a small program that reads sensors, receives data from the mesh, processes it through a logic core, and sends results to actuators and/or back out to the mesh.

```
                    ┌──────────────────────────────────┐
                    │           GluonNode               │
                    │                                   │
  Physical World    │  ┌──────────┐    ┌──────────┐    │    LoRa Mesh
  ─────────────────►│  │ Sensors  │───►│  Logic   │───►│──────────────────►
                    │  └──────────┘    │          │    │
                    │                  │ compute()│    │
  LoRa Mesh         │  ┌──────────┐   │ evolve() │    │
  ─────────────────►│  │  Inlets  │───►│          │    │
                    │  └──────────┘    └────┬─────┘    │
                    │                       │          │
                    │                  ┌────▼─────┐    │
                    │                  │ Outlets  │────│──► LoRa Mesh
                    │                  ├──────────┤    │
                    │                  │Actuators │────│──► Physical World
                    │                  └──────────┘    │
                    └──────────────────────────────────┘
```

## Main Loop

The update cycle runs in `GluonNode::update()`, called every `loop()` iteration. The order is preserved from V2 and is critical for correct dataflow:

```
1. sensors_.update()          — Poll all sensors, detect events
2. transport_->update()       — Receive incoming mesh messages
   → processMessage()        — Route DATA to inlets, handle LINK_*
3. logic_->update()           — Check trigger conditions, call compute()
4. sendOutletData()           — Rate-limited send to mesh (latch period)
5. actuators_.update()        — Drive physical outputs (pulse decay, etc.)
6. sendHeartbeat()            — Periodic keepalive broadcast (every 30s)
```

## Core Classes

### Data

The universal data packet. Every piece of information flowing through the system is a `Data`:

```cpp
struct Data {
    String label;       // What this data represents ("button", "temp", "BPM")
    float value;        // Numeric payload (was int16_t in V2)
    bool event;         // Boolean event flag — the "bang"
    uint32_t timestamp; // millis() at creation
};
```

**Design note:** The `event` flag is key. Much of Gluon's logic operates on events (bangs), not continuous values. A button press is `event=true`, a temperature reading is `event=false, value=23.5`.

### Inlets and Outlets

Inlets and Outlets are the node's I/O ports — the "plugs" that connect nodes together.

- **Inlet**: Receives data from remote nodes. Has a list of `Link`s (connections). Supports fan-in (multiple senders to one inlet).
- **Outlet**: Sends data to remote nodes. Has a list of `Link`s. Supports fan-out (one outlet to many receivers).

Each connection goes through a **LOOSE → FIXED lifecycle**:

1. A new connection starts as LOOSE (within `LOOSE_PERIOD_MS = 2000ms`)
2. After the period, it becomes FIXED
3. If all inlets are full and a new LOOSE connection arrives, it displaces the oldest LOOSE link (re-patching)
4. A second connection request from an already-FIXED node **deletes** the connection (toggle behavior)

This lifecycle enables the proximity-based connection discovery without needing a screen or buttons.

### BaseLogic — Update Modes

The logic core has two methods:

- `evolve()` — Called **every loop** iteration (for timers, decay, animations)
- `compute()` — Called **only when trigger conditions are met**

Trigger conditions use a bitmask (preserved from V2):

| Flag           | Value | Meaning                            |
| -------------- | ----- | ---------------------------------- |
| `MANUAL`       | 0x00  | Only via explicit `forceCompute()` |
| `SENSOR_EVENT` | 0x01  | Any local sensor fired an event    |
| `FIRST_INLET`  | 0x02  | Inlet[0] received new data         |
| `ANY_INLET`    | 0x04  | Any inlet received new data        |
| `SYNC`         | 0x08  | External sync trigger              |
| `PERIODIC`     | 0x10  | Timer elapsed (`periodMs`)         |
| `HEARTBEAT_IN` | 0x20  | Received a heartbeat (new in V3)   |

Combine with bitwise OR: `updateMode = SENSOR_EVENT | ANY_INLET` (0x05) means "compute when a sensor fires OR when any inlet gets data".

### GluonNode

The orchestrator. Owns all arrays (Inlets, Outlets, Sensors, Actuators) and a Logic module. Handles:

- The main update loop (see above)
- Message routing (DATA_UPDATE → inlets, LINK_REQUEST/ACK/DELETE)
- Connection persistence (NVS Preferences, survives reboot)
- Rate-limited outlet sending (latch period, default 200ms)
- Heartbeat broadcasting (30s interval)

### Message Protocol

Messages travel over the mesh as MsgPack-encoded binary (via ArduinoJson):

```
{ "ty": uint8,     // MessageType enum
  "si": uint32,    // Sender NodeID
  "ri": uint32,    // Receiver NodeID (or 0xFFFFFFFF for broadcast)
  "sp": uint8,     // Sender port (outlet index)
  "rp": uint8,     // Receiver port (inlet index)
  "d":  { ... }    // Data payload (for DATA_UPDATE messages)
}
```

Message types:

- `DATA_UPDATE (0x01)` — Payload data from outlet to inlet
- `LINK_REQUEST (0x10)` — "I want to connect to your inlet"
- `LINK_ACK (0x11)` — "Connection accepted"
- `LINK_DELETE (0x12)` — "Connection removed"
- `NODE_ANNOUNCE (0x20)` — "I exist, here are my capabilities"
- `HEARTBEAT (0x31)` — Keepalive (new in V3)
- `NETWORK_RESET (0x41)` — "Clear all connections"

## Transport Abstraction

The transport layer is fully abstracted via the `MeshTransport` interface:

```cpp
class MeshTransport {
    virtual void init(NodeID myNodeId) = 0;
    virtual void update() = 0;
    virtual bool send(const Message& msg) = 0;
    virtual bool broadcast(const Message& msg) = 0;
    virtual bool hasMessage() const = 0;
    virtual Message receive() = 0;
};
```

Current implementations:

1. **`MeshtasticSerialTransport`** — Talks to a Meshtastic node over UART (framed binary: `[0x94][0xC3][len_hi][len_lo][msgpack...]`). Used with XIAO + external Wio-SX1262.
2. **`LoopbackTransport`** — Messages sent to self come back immediately. For development without radio hardware.

Future implementations:

- **`RadioLibTransport`** — Direct SX1262 via RadioLib (for Heltec on-board radio)
- **`MQTTTransport`** — Bridge to web dashboards via WiFi
- **`BLETransport`** — Short-range for proximity discovery

## Configuration System

V2 used compile-time `#define GLUON` blocks — each module type was a different firmware build. V3 uses a JSON config file on LittleFS:

```json
{
  "name": "METRO-01",
  "nodeId": 0,
  "inlets": 3,
  "outlets": 1,
  "logic": "metro",
  "updateMode": 5,
  "sensors": [{ "type": "switch", "pin": 2 }],
  "actuators": [{ "type": "digital", "pin": 3, "mode": "pulse" }]
}
```

The config is loaded at boot from `/config.json` on LittleFS. Change behavior by swapping config files — no recompilation.

## Factory Pattern

Components self-register with type factories via macros:

```cpp
// In BuiltinSensors.h:
REGISTER_SENSOR(switch, [](JsonObjectConst cfg) -> BaseSensor* {
    return new SensorDigitalPin(cfg["name"] | "SW", cfg["pin"] | 0);
});
```

At startup, `NodeBuilder` iterates the config's sensor/actuator arrays and calls the factory:

```cpp
NodeBuilder::buildSensors(node.sensors(), config.sensorsDoc.as<JsonArrayConst>());
```

Adding a new component type requires only:

1. Write a class that extends `BaseSensor`/`BaseActuator`/`BaseLogic`
2. Add `REGISTER_SENSOR("mytype", ...)` at the bottom
3. Include the header in `main.cpp`

No other files need to change.

## Board Abstraction

Hardware-specific pin definitions live in `src/boards/BoardPins.h`, selected at compile time by `-DGLUON_BOARD_XXX` build flags. Each board defines:

- OLED pins (SDA, SCL, RST, I2C address)
- LoRa pins (NSS, SCK, MOSI, MISO, RST, BUSY, DIO1) — if on-board
- Serial1 pins (for external LoRa sidecar or GNSS)
- Available user GPIO and ADC pins
- LED, button, Vext power control

The same firmware source compiles for both XIAO ESP32S3 and Heltec V3/V4 — only the pin mapping changes.

## Persistence

Two persistence mechanisms:

1. **LittleFS** (`/config.json`) — Node configuration, component definitions. Survives firmware updates. Uploaded via PlatformIO.
2. **NVS Preferences** (`"gluon_links"`, `"gluon"`) — Runtime state: active connections, logic state (counter values, metro BPM, update mode). Survives reboot, written on connection changes.

## V2 → V3 Key Differences

| Aspect        | V2 (2015)                         | V3 (2026)                         |
| ------------- | --------------------------------- | --------------------------------- |
| MCU           | ATmega328 (32KB Flash, 2KB RAM)   | ESP32-S3 (16MB Flash, 512KB SRAM) |
| Radio         | RFM69 433MHz, point-to-point      | SX1262 LoRa, multi-hop mesh       |
| NodeID        | uint8_t (max 255 nodes)           | uint32_t (Meshtastic-compatible)  |
| Data value    | int16_t                           | float                             |
| Serialization | ASCII strings                     | MsgPack binary                    |
| Configuration | `#define GLUON 7` at compile time | JSON on LittleFS at runtime       |
| Persistence   | 1KB EEPROM (wear-limited)         | ESP32 NVS (flash-backed, no wear) |
| Fan-in/out    | Fixed `MAX_FAN_IN=3`              | Dynamic, per-node configurable    |
| Module types  | 1 per firmware build              | Any mix in one build              |
| Heartbeat     | None                              | 30-second broadcast               |
| Sniffer       | Separate SNIFFER firmware         | Serial console in every node      |
