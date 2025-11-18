#include <Arduino.h>
#include "DHTesp.h"
#include <WiFiManager.h>
#include <PubSubClient.h>
 
WiFiManager wm;
DHTesp dht;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMSG = 0;
float lastTemperature = 0.0;
float lastHumidity = 0.0;

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("WeatherB2")) {
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

  WiFi.mode(WIFI_STA);

  Serial.begin(115200);

  dht.setup(25, DHTesp::DHT11);

  delay(1000);
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
    client.connect("WeatherB2");
}

void loop() {
  long currentTime = millis();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  if (currentTime - lastMSG > 60000 || abs(temperature - lastTemperature) >= 0.5 || abs(humidity - lastHumidity) >= 1.0) {
    lastMSG = currentTime;
    lastTemperature = temperature;
    lastHumidity = humidity;

    client.publish("WeatherB2/temperature", String(lastTemperature).c_str());
    client.publish("WeatherB2/humidity", String(lastHumidity).c_str());
  }
}