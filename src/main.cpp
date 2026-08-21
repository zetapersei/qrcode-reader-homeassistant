#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32QRCodeReader.h>

// --- CONFIGURAZIONE RETE E MQTT ---
const char* WIFI_SSID = "IL_TUO_SSID";
const char* WIFI_PASS = "LA_TUA_PASSWORD";

const char* MQTT_SERVER   = "192.168.1.XXX"; // IP del broker MQTT (es. IP di Home Assistant)
const int   MQTT_PORT     = 1883;
const char* MQTT_USER     = "utente_mqtt";   // Lasciare vuoto se non richiesto
const char* MQTT_PASS     = "password_mqtt"; // Lasciare vuoto se non richiesto

// Topic MQTT per Home Assistant
const char* TOPIC_STATE  = "homeassistant/sensor/esp32cam_qr/state";
const char* TOPIC_CONFIG = "homeassistant/sensor/esp32cam_qr/config";

// --- ISTANZE E VARIABILI ---
WiFiClient espClient;
PubSubClient mqttClient(espClient);
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);

String lastScannedCode = "";
unsigned long lastScanTime = 0;
const unsigned long DEBOUNCE_DELAY = 3000; // Tempo minimo (ms) prima di rileggere lo stesso codice

// --- FUNZIONE CONNESSIONE WI-FI ---
void setupWiFi() {
  Serial.print("Connessione a ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connesso. IP: " + WiFi.localIP().toString());
}

// --- FUNZIONE CONFIGURAZIONE AUTO-DISCOVERY HOME ASSISTANT ---
void publishHAAutoDiscovery() {
  // Invia la configurazione iniziale per creare automaticamente il sensore in Home Assistant
  String payload = "{"
    "\"name\": \"QR Code Scanner\","
    "\"state_topic\": \"" + String(TOPIC_STATE) + "\","
    "\"unique_id\": \"esp32cam_qr_sensor\","
    "\"icon\": \"mdi:qrcode-scan\","
    "\"device\": {"
      "\"identifiers\": [\"esp32cam_qr_01\"],"
      "\"name\": \"ESP32-CAM QR Reader\","
      "\"model\": \"ESP32-CAM AI-Thinker\","
      "\"manufacturer\": \"Espressif\""
    "}"
  "}";

  mqttClient.publish(TOPIC_CONFIG, payload.c_str(), true);
}

// --- FUNZIONE CONNESSIONE MQTT ---
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connessione al broker MQTT...");
    String clientId = "ESP32CAM-QR-" + String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("Connesso!");
      publishHAAutoDiscovery();
    } else {
      Serial.print("Fallito, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Riprovo tra 5 secondi...");
      delay(5000);
    }
  }
}

// --- TASK FREERTOS PER LA LETTURA DEL QR CODE ---
void onQrCodeTask(void *pvParameters) {
  struct QRCodeData qrCodeData;

  while (true) {
    if (reader.receiveQrCode(&qrCodeData, 100)) {
      if (qrCodeData.valid) {
        String scannedPayload = String((const char *)qrCodeData.payload);
        unsigned long now = millis();

        // Evita letture doppie immediate dello stesso codice
        if (scannedPayload != lastScannedCode || (now - lastScanTime > DEBOUNCE_DELAY)) {
          lastScannedCode = scannedPayload;
          lastScanTime = now;

          Serial.print("QR Code rilevato: ");
          Serial.println(scannedPayload);

          if (mqttClient.connected()) {
            mqttClient.publish(TOPIC_STATE, scannedPayload.c_str());
          }
        }
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  setupWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  // Inizializzazione fotocamera e lettore QR
  reader.setup();
  reader.beginOnCore(1);

  // Creazione del task dedicato alla scansione sul Core 1
  xTaskCreate(onQrCodeTask, "onQrCodeTask", 4096, NULL, 4, NULL);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
}
