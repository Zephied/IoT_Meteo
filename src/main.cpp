#include <Arduino.h>
#include "DHTesp.h"
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <iostream>
#include <string>
#include <random>
#include <Preferences.h>
#include "esp_system.h"
 
WiFiManager wm;
DHTesp dht;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMSG = 0;
float lastTemperature = 0.0;
float lastHumidity = 0.0;
String tempTopic = "WeatherB2/temperature/";
String humTopic = "WeatherB2/humidity/";

std::string generateRandomString(size_t length) {
  const char characters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const size_t charsetSize = sizeof(characters) - 1;

  std::string result;
  result.reserve(length);

  while (result.size() < length) {
    uint32_t r = esp_random();
    // consume the 32 bits in 8-bit chunks
    for (int i = 0; i < 4 && result.size() < length; ++i) {
      uint8_t v = r & 0xFF;
      result += characters[v % charsetSize];
      r >>= 8;
    }
  }
  return result;
}

String mqtt_id;

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(("WeatherB2-"+mqtt_id).c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}

void publish(float temperature, float humidity) {
  lastTemperature = temperature;
  lastHumidity = humidity;

  client.publish((tempTopic+mqtt_id).c_str(), String(lastTemperature).c_str());
  client.publish((humTopic+mqtt_id).c_str(), String(lastHumidity).c_str());
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Envoyer 'r' ou 'reset' sur le port série pour régénérer le token MQTT");

  Preferences prefs;
  prefs.begin("mqtt", false);

  if (prefs.isKey("id")) {
    String stored = prefs.getString("id");
    mqtt_id = stored;

    Serial.print("MQTT id loaded from NVS: ");
    Serial.println(mqtt_id);
  } else {
    std::string rand = generateRandomString(10);
    mqtt_id = String(rand.c_str());
    prefs.putString("id", mqtt_id);

    Serial.print("MQTT id generated and saved to NVS: ");
    Serial.println(mqtt_id);
  }
  prefs.end();

  String s = "<p>code d'appairage MQTT: "+mqtt_id+"</p>";
  static WiFiManagerParameter mqttid(s.c_str());
  wm.addParameter(&mqttid);
  WiFi.mode(WIFI_STA);
  dht.setup(25, DHTesp::DHT11);

  Serial.println();
  Serial.println("Tentative de connexion au réseau Wi-Fi...");

  if (!wm.autoConnect()) {
    Serial.println("Erreur de connexion au réseau Wi-Fi.");
  } else {
    Serial.println("Connexion au réseau Wi-Fi réussie !");
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());
  }

  client.setServer("broker.emqx.io", 1883);
  reconnect();
}

void loop() {
  // Handle serial commands (reset token)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("r") || cmd.equalsIgnoreCase("reset")) {
      Preferences prefs;
      prefs.begin("mqtt", false);
      prefs.remove("id");
      prefs.end();
      Serial.println("MQTT id removed from NVS, restarting...");
      delay(200);
      ESP.restart();
    } else if (cmd.equalsIgnoreCase("wr")) {
      wm.resetSettings();
      Serial.println("Wi-Fi settings reset, restarting...");
      delay(200);
      ESP.restart();
    }
  }

  long currentTime = millis();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (currentTime - lastMSG > 60000) {
    lastMSG = currentTime;
    publish(temperature, humidity);
  } else if (std::abs(temperature - lastTemperature) >= 1.0 || std::abs(humidity - lastHumidity) >= 5.0) {
    publish(temperature, humidity);
  }
}