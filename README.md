# SmartMeterADE9153
SmartMeter using ADE9153 shield and NILM 
# Structure du Projet
## Arduino_Code : Contient le code principal pour ESP.
Fonctionnalités Principales:
- Lecture des Données Énergétiques :
Mesure de différents paramètres tels que tension, courant, puissance active/apparente, facteur de puissance, fréquence et température.
Stockage des mesures dans des vecteurs pour une transmission groupée.

- Communication MQTT :
Connexion à un broker MQTT pour envoyer les données mesurées.
Publication des données sur un topic MQTT spécifié.

- Gestion WiFi :
Connexion automatique au réseau WiFi spécifié.
Reconnexion automatique en cas de perte de la connexion WiFi.

- Autocalibration :
Fonctionnalité d'autocalibration pour les canaux de courant et de tension en appuyant sur un bouton utilisateur (USER_INPUT).
Boucle Principale (loop()) :

- Gestion de la connexion MQTT et de la communication réseau.

- Pins et Définitions :
Utilisation de divers pins pour la communication SPI, le bouton utilisateur, le LED, et le reset du Shield ADE9153A.

- Sérialisation JSON :
Conversion des données énergétiques en format JSON pour la transmission MQTT.


## Libraries/ADE9153A : Bibliothèque ADE9153A pour Arduino.
## Python_Scripts : Scripts pour la normalisation et le filtrage du bruit.

## Fonctionnement
- Collecte de Données: Les mesures énergétiques sont envoyées régulièrement via MQTT.
- Traitement de Données: Le script Python sur Raspberry Pi reçoit ces données, les normalise et les stocke pour l'analyse NILM.
- Analyse NILM: Les données normalisées sont ensuite utilisées pour identifier les signatures spécifiques des différents appareils électriques.
