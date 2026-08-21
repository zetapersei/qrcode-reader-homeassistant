# ESP32-CAM QR Code MQTT Access Controller

Un sistema di controllo accessi basato su **ESP32-CAM** che legge QR Code, decodifica stringhe alfanumeriche e comunica con **Home Assistant** tramite **MQTT** (con supporto all'Auto Discovery).

Questo progetto permette di gestire l'attivazione di un relè (es. una serratura elettrica) quando il QR Code scansionato corrisponde a un codice preimpostato in Home Assistant.

## Funzionalità
- **Scansione QR Code:** Utilizza la libreria `esp32qrcodereader` per una scansione rapida e affidabile.
- **Integrazione MQTT:** Invio dei codici scansionati in tempo reale tramite protocollo MQTT.
- **Home Assistant Auto Discovery:** Il dispositivo si configura automaticamente su Home Assistant non appena collegato.
- **Controllo Dinamico:** Il codice di accesso può essere modificato in qualsiasi momento dalla dashboard di Home Assistant tramite un `input_text`.
- **Logica Locale:** Il relè viene gestito in base al confronto tra il codice letto e quello autorizzato.

## Requisiti Hardware
- **ESP32-CAM** (Modulo AI-Thinker o compatibile)
- **Modulo Relè** (5V)
- **FTDI Adapter** (necessario per caricare il firmware su ESP32-CAM)
- **Alimentazione 5V** stabile

## Configurazione Software
Il progetto utilizza **PlatformIO** con framework Arduino.

### `platformio.ini`
Assicurati di avere le librerie necessarie definite nel file `platformio.ini`:

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

board_build.partitions = huge_app.csv