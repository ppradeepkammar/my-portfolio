#include <Wire.h>
#include <QMC5883LCompass.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// =====================================================
// DEVICE CONFIGURATION & IDENTIFIER
// =====================================================
#define DEVICE_ID "BIKE-TRACK-001"

// =====================================================
// QMC5883L ACCELERATION / SHAKE SENSOR
// =====================================================
QMC5883LCompass compass;
#define LED_PIN 2
#define BUZZER 14
#define SHAKE_THRESHOLD 800

int previousX, previousY, previousZ;
int currentX = 0, currentY = 0, currentZ = 0;

// =====================================================
// WiFi (Mobile Hotspot or Router)
// =====================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// =====================================================
// ADAFRUIT IO CLOUD CREDENTIALS & MQTT
// =====================================================
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "YOUR_ADAFRUIT_USERNAME"
#define AIO_KEY         "YOUR_ADAFRUIT_IO_KEY"

WiFiClient mqttClient;
Adafruit_MQTT_Client mqtt(&mqttClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish locationFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bike-location/csv");
Adafruit_MQTT_Publish statusFeed   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bike-status");

// =====================================================
// TELEGRAM ALERT CONFIGURATION
// =====================================================
#define BOTtoken "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID "YOUR_CHAT_ID"

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// =====================================================
// GPS CONFIGURATION (HardwareSerial 2)
// =====================================================
#define GPS_RX 16
#define GPS_TX 17
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// =====================================================
// TIMERS & ACCIDENT VARIABLES
// =====================================================
unsigned long lastBotCheckTime = 0;
const unsigned long botRequestInterval = 1000;

unsigned long lastCloudPushTime = 0;
const unsigned long cloudPushInterval = 3000;

bool accidentDetected = false;
unsigned long accidentTime = 0;

// =====================================================
// MQTT CONNECT & KEEP-ALIVE
// =====================================================
void MQTT_connect() {
  int8_t ret;
  if (mqtt.connected()) return;

  Serial.print("Connecting to Adafruit Cloud... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying cloud MQTT in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
    retries--;
    if (retries == 0) return;
  }
  Serial.println("Cloud Dashboard Connected!");
}

// =====================================================
// SEND ACCIDENT NOTIFICATION TO TELEGRAM & CLOUD
// =====================================================
void sendAccidentLocation() {
  Serial.println();
  Serial.println("================================");
  Serial.println("ACCIDENT DETECTED!");
  Serial.println("Getting GPS location...");
  Serial.println("================================");

  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    while (GPS_Serial.available()) {
      gps.encode(GPS_Serial.read());
    }
    if (gps.location.isValid()) break;
    delay(10);
  }

  String alertMsg = "CRASH DETECTED | ID: " + String(DEVICE_ID);
  statusFeed.publish(alertMsg.c_str());

  String message = "🚨 *ACCIDENT DETECTED!*\n";
  message += "🆔 Device ID: `" + String(DEVICE_ID) + "`\n";
  message += "⚠️ Sudden movement registered by QMC5883L.\n\n";

  if (gps.location.isValid()) {
    double latitude = gps.location.lat();
    double longitude = gps.location.lng();
    String mapsLink = "https://maps.google.com/?q=" + String(latitude, 6) + "," + String(longitude, 6);

    message += "📍 *Location:*\n";
    message += "Latitude: " + String(latitude, 6) + "\n";
    message += "Longitude: " + String(longitude, 6) + "\n\n";
    message += "🗺️ *Google Maps Link:*\n" + mapsLink;
  } else {
    message += "❌ GPS location fix not available yet.";
  }

  bool sent = bot.sendMessage(CHAT_ID, message, "Markdown");
  if (sent) {
    Serial.println("Telegram accident message sent successfully!");
  } else {
    Serial.println("Failed to send Telegram message!");
  }
}

// =====================================================
// HANDLE INCOMING TELEGRAM COMMANDS
// =====================================================
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized access.", "");
      continue;
    }

    String text = bot.messages[i].text;

    if (text == "/status" || text == "/start") {
      String status = "🟢 *Bike Safety System*\n";
      status += "🆔 Device ID: `" + String(DEVICE_ID) + "`\n";
      status += "📡 WiFi: " + String((WiFi.status() == WL_CONNECTED) ? "CONNECTED" : "DISCONNECTED") + "\n";
      status += "☁️ Cloud Broker: " + String(mqtt.connected() ? "ONLINE" : "OFFLINE") + "\n";
      status += "🛰️ GPS Fix: " + String(gps.location.isValid() ? "LOCKED" : "SEARCHING...") + "\n";
      status += "📊 Live Dashboard: https://io.adafruit.com/" + String(AIO_USERNAME) + "/dashboards";

      bot.sendMessage(chat_id, status, "Markdown");
    }
    else if (text == "/location") {
      if (gps.location.isValid()) {
        String msg = "📍 *Current Location:*\n";
        msg += "https://maps.google.com/?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
        bot.sendMessage(chat_id, msg, "Markdown");
      } else {
        bot.sendMessage(chat_id, "⚠️ GPS fix searching satellites...", "");
      }
    }
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  compass.init();
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER, LOW);
  compass.read();
  previousX = compass.getX();
  previousY = compass.getY();
  previousZ = compass.getZ();

  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  client.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "🚀 *Tracker Online*\nDevice: `" + String(DEVICE_ID) + "`\nConnected to Cloud Dashboard.", "Markdown");
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }

  compass.read();
  currentX = compass.getX();
  currentY = compass.getY();
  currentZ = compass.getZ();

  int changeX = abs(currentX - previousX);
  int changeY = abs(currentY - previousY);
  int changeZ = abs(currentZ - previousZ);

  if (changeX > SHAKE_THRESHOLD || changeY > SHAKE_THRESHOLD || changeZ > SHAKE_THRESHOLD) {
    Serial.println("Strong movement detected! Triggering alert immediately...");

    if (!accidentDetected) {
      accidentDetected = true;
      accidentTime = millis();

      Serial.println();
      Serial.println("============================");
      Serial.println("!!! ACCIDENT DETECTED !!!");
      Serial.println("============================");

      sendAccidentLocation();
    }
  }

  if (accidentDetected) {
    if (millis() - accidentTime < 10000) {
      digitalWrite(LED_PIN, (millis() / 200) % 2);
      digitalWrite(BUZZER, (millis() / 200) % 2);
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER, LOW);
      accidentDetected = false;
      statusFeed.publish("NORMAL");
    }
  }

  previousX = currentX;
  previousY = currentY;
  previousZ = currentZ;

  MQTT_connect();

  if (millis() - lastCloudPushTime > cloudPushInterval) {
    lastCloudPushTime = millis();

    if (gps.location.isValid()) {
      float speed = gps.speed.isValid() ? gps.speed.kmph() : 0.0;
      double lat = gps.location.lat();
      double lng = gps.location.lng();
      double alt = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;

      String csvData = String(speed, 1) + "," + String(lat, 6) + "," + String(lng, 6) + "," + String(alt, 1);
      locationFeed.publish(csvData.c_str());

      if (!accidentDetected) {
        statusFeed.publish("TRACKING ACTIVE");
      }
    } else {
      statusFeed.publish("GPS SEARCHING...");
    }
  }

  if (millis() - lastBotCheckTime > botRequestInterval) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheckTime = millis();
  }

  delay(50);
}
