# SmartMeterADE9153
SmartMeter using ADE9153 shield and NILM 
## Structure du Projet
- Arduino_Code : Contient le code principal pour Arduino.
- Libraries/ADE9153A : Bibliothèque ADE9153A pour Arduino.
- Python_Scripts : Scripts pour la normalisation et le filtrage du bruit.

## Fonctionnement
- Collecte de Données: Les mesures énergétiques sont envoyées régulièrement via MQTT.
- Traitement de Données: Le script Python sur Raspberry Pi reçoit ces données, les normalise et les stocke pour l'analyse NILM.
- Analyse NILM: Les données normalisées sont ensuite utilisées pour identifier les signatures spécifiques des différents appareils électriques.
