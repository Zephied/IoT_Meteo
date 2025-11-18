#include <Arduino.h>
#include "DHTesp.h"
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <iostream>
#include <string>
#include <random>
#include <Preferences.h>
 
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
    std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 generator(rd()); // Générateur aléatoire
    std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += characters[distribution(generator)];
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

void setup() {
  // wm.resetSettings(); // Uncomment to reset Wi-Fi settings

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
  
  Serial.begin(115200);
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
  long currentTime = millis();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (currentTime - lastMSG > 60000) {
    lastMSG = currentTime;
    lastTemperature = temperature;
    lastHumidity = humidity;

    client.publish((tempTopic+mqtt_id).c_str(), String(lastTemperature).c_str());
    client.publish((humTopic+mqtt_id).c_str(), String(lastHumidity).c_str());

  } else if (std::abs(temperature - lastTemperature) >= 0.5 || std::abs(humidity - lastHumidity) >= 5.0) {
    lastTemperature = temperature;
    lastHumidity = humidity;

    client.publish((tempTopic+mqtt_id).c_str(), String(lastTemperature).c_str());
    client.publish((humTopic+mqtt_id).c_str(), String(lastHumidity).c_str());
  }
}