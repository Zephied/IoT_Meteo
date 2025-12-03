
# Projet Station Météo
- **But**: Lire température et humidité depuis un capteur DHT11, publier les valeurs via MQTT et fournir un code d'appairage MQTT unique généré une seule fois au premier démarrage (persisté en NVS).

**Fichiers clés**
- `src/main.cpp`: code principal — lecture DHT, gestion WiFi (WiFiManager), MQTT (PubSubClient), génération et persistance du token.
- `platformio.ini`: configuration PlatformIO (environnement de build).

**Matériel / Configuration**
- **Plateforme cible**: ESP32 (le code utilise `Preferences` et `esp_random()`). Si vous utilisez un ESP8266 il faudra adapter la persistance et la génération aléatoire.
- **Capteur**: DHT11 branché sur la broche GPIO `25` (modifiez dans `dht.setup(25, DHTesp::DHT11)` si besoin).
- **Broker MQTT**: par défaut `broker.emqx.io:1883` (modifiable dans `src/main.cpp`).

**Fonctionnement important**
- Au premier démarrage, le firmware génère un identifiant aléatoire de 10 caractères et le sauvegarde en NVS (`Preferences`, namespace `mqtt`, clé `id`).
- Cet identifiant est ensuite réutilisé à chaque redémarrage. Les topics publiés sont de la forme `WeatherB2/temperature/<id>` et `WeatherB2/humidity/<id>`, modifiables dans `src/main.cpp` en fonction de vos topics.

**Commandes utiles (PlatformIO / PowerShell)**
- Compiler et téléverser:
	```
  	.platformio/penv/Scripts/platformio.exe run --target upload
  	```
- Ouvrir le moniteur série (baud 115200) :
	```
	.platformio\penv\Scripts\platformio.exe device monitor -p COM3 -b 115200
	```

**Réinitialiser / régénérer le token MQTT**

1. Ouvrez le moniteur série (```.platformio\penv\Scripts\platformio.exe device monitor -p COM3 -b 115200```).
2. Envoyez `r` puis appuyez sur Entrée (ou envoyez `reset`).
3. L'appareil supprimera la clé NVS et redémarrera : il affichera ensuite `MQTT id generated and saved to NVS: <nouvel_id>`.

**Autres astuces**
- Forcer la réinitialisation des paramètres Wi‑Fi (WiFiManager) : envoyez `wr` puis appuyez sur Entrée dans le moniteur série.
- Si le token généré est toujours identique : assurez-vous d'utiliser un ESP32 (le code utilise `esp_random()`), sinon remplacez la génération aléatoire pour ESP8266.
- Pour changer le broker, port, topics ou le format d'ID, éditez `src/main.cpp` et recherchez `client.setServer` / `tempTopic` / `humTopic` / `generateRandomString`.

**Dépannage rapide**
- Pas de log série : vérifiez que `Serial.begin(115200);` est présent et que vous ouvrez le moniteur à `115200`.
- Erreur upload : vérifiez le câble USB, le port COM et les drivers CH340/CP210x si nécessaire.

retrouvez la [partie web](https://github.com/gabichcochet/NathanMeteoWEB) pour visualiser les données publiées par cette station météo via MQTT.
