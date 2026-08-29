#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32QRCodeReader.h>

// --- CONFIGURAZIONE RETE E MQTT ---
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "YOUR_MQTT_SERVER_IP"; // IP del tuo broker MQTT (es. Home Assistant)
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USER";         // Lascia vuoto o compila se richiesto
const char* mqtt_password = "YOUR_MQTT_PASSWORD";   // Lascia vuoto o compila se richiesto



WiFiClient espClient;
PubSubClient client(espClient);
ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);

// Topic MQTT
const char* topic_scanned = "esp32cam/qrcode/text";
const char* topic_discovery = "homeassistant/sensor/esp32cam_qr/config";

unsigned long lastSend = 0;
String lastScannedCode = "";
bool discoverySent = false;

void setup_wifi() {
    delay(100);
    Serial.println();
    Serial.print("Connessione a ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi connesso!");
}

void sendMqttDiscovery() {
    // Configurazione MQTT Discovery per Home Assistant
    String payload = "{";
    payload += "\"name\": \"ESP32-CAM QR Scanned\",";
    payload += "\"uniq_id\": \"esp32cam_qr_code_01\",";
    payload += "\"stat_t\": \"esp32cam/qrcode/text\",";
    payload += "\"dev\": {";
    payload += "\"ids\": [\"esp32cam_scanner\"],";
    payload += "\"name\": \"ESP32-CAM QR Reader\",";
    payload += "\"mf\": \"Custom\",";
    payload += "\"mdl\": \"ESP32-CAM\"";
    payload += "}}";

    Serial.print("Invio MQTT Discovery... Dimensione payload: ");
    Serial.println(payload.length());

    // Invio con retained = true per Home Assistant
    if (client.publish(topic_discovery, payload.c_str(), true)) {
        Serial.println("MQTT Discovery inviato con successo!");
        discoverySent = true;
    } else {
        Serial.println("ERRORE: Invio MQTT Discovery fallito (buffer pieno o non connesso?)");
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Tentativo di connessione MQTT...");
        String clientId = "ESP32CAM-Client-";
        clientId += String(random(0xffff), HEX);
        
        if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
            Serial.println("connesso!");
            discoverySent = false; // Forza il reinvio del discovery alla riconnessione
        } else {
            Serial.print("fallito, rc=");
            Serial.print(client.state());
            Serial.println(" riprovo tra 5 secondi");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Inizializzazione ESP32-CAM QR Reader...");

    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(512); // Assicura lo spazio per i pacchetti MQTT più grandi

    // Inizializzazione Lettore QR
    reader.setup();
    Serial.println("Setup completato. Avvio scansione...");
    reader.begin();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    // Invia il discovery appena la connessione è stabile
    if (client.connected() && !discoverySent) {
        sendMqttDiscovery();
    }

    // Correzione: utilizzo del metodo corretto della libreria ESP32QRCodeReader
    struct QRCodeData qrCodeData;
    // Sostituito 'reader.receive' con la funzione nativa della libreria
    if (reader.receiveQrCode(&qrCodeData,100)) {
        if (qrCodeData.valid) {
            String scannedText = (const char*)qrCodeData.payload;
            Serial.print("QR Code Rilevato: ");
            Serial.println(scannedText);

            // Invia il codice decodificato via MQTT se è passato almeno 5 secondi (anti-flood)
            if (scannedText != lastScannedCode || (millis() - lastSend > 5000)) {
                if (client.publish(topic_scanned, scannedText.c_str())) {
                    Serial.println("Testo QR pubblicato correttamente su MQTT.");
                } else {
                    Serial.println("Errore pubblicazione testo QR.");
                }
                lastScannedCode = scannedText;
                lastSend = millis();
            }
        }
    }
}