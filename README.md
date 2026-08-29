# ESP32-CAM QR Code MQTT Access Controller

An access control system based on **ESP32-CAM** that scans QR Codes, decodes alphanumeric strings, and communicates with **Home Assistant** via **MQTT** (supporting Auto Discovery).

This project allows you to trigger a relay (e.g., an electric lock) when the scanned QR Code matches a code pre-set in your Home Assistant dashboard.

## Features
- **QR Code Scanning:** Uses the `esp32qrcodereader` library for fast and reliable scanning.
- **MQTT Integration:** Real-time transmission of scanned codes via MQTT.
- **Home Assistant Auto Discovery:** The device automatically registers itself in Home Assistant as soon as it connects.
- **Dynamic Control:** The access code can be changed at any time from your Home Assistant dashboard via an `input_text` helper.
- **Local Logic:** The relay is triggered based on the comparison between the scanned code and the authorized code.

## Hardware Requirements
- **ESP32-CAM** (AI-Thinker module or compatible)
- **Relay Module** (5V)
- **FTDI Adapter** (required for firmware flashing)
- **Stable 5V Power Supply**

## Software Configuration
This project uses **PlatformIO** with the Arduino framework.

### `platformio.ini`
Ensure the necessary libraries are defined in your `platformio.ini` file:

```ini
[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
monitor_speed = 115200

lib_deps =
    knolleary/PubSubClient @ ^2.8
    alvarowolf/ESP32QRCodeReader @ ^1.1.0

build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -DMQTT_MAX_PACKET_SIZE=512


board_build.partitions = huge_app.csv

```

## Home Assistant Setup

1. **Create an Helper:**
Go to *Settings > Devices & Services > Helpers > Create Helper > Text* and create an entity named `input_text.codice_qr_valido`.
2. **Automation:**
Add the following automation (YAML mode) to manage the comparison between the scanned code and the one saved in your Helper:

```yaml
alias: Attiva Relè con QR Code Valido
description: >-
  Attiva il relè se il QR code corrisponde all'helper, anche in caso di letture
  ripetute.
triggers:
  - topic: esp32cam/qrcode/text
    trigger: mqtt
conditions:
  - condition: template
    value_template: |-
      {{ trigger.payload != 'unavailable' and trigger.payload != 'unknown' 
         and trigger.payload == states('input_text.codice_qr_valido') }}
actions:
  - target:
      entity_id: switch.modulo_portone_serratura_porta
    action: switch.turn_on
  - delay:
      seconds: 1
  - target:
      entity_id: switch.modulo_portone_serratura_porta
    action: switch.turn_off
mode: single

```

## License

This project is released under the MIT License. Feel free to fork, modify, and improve the code.

## Credits

* [ESP32QRCodeReader](https://www.google.com/search?q=https://github.com/alvarowolf/ESP32QRCodeReader) library by alvarowolf.
* MQTT support via [PubSubClient](https://pubsubclient.knolleary.net/).
