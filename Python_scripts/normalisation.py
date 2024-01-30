#Ce script souscrit au topic MQTT spécifié et attend les messages.
#Lorsqu'un message est reçu, il est converti de JSON en un dictionnaire Python.
#Les valeurs sont extraites, normalisées, puis ajoutées à un DataFrame pandas.
#Le DataFrame est mis à jour à chaque réception de message.


import paho.mqtt.client as mqtt
import json
import numpy as np
import pandas as pd

# Configuration MQTT
MQTT_BROKER = "adresse_du_broker_mqtt"
MQTT_PORT = 1883
MQTT_TOPIC = "montopic"
# Fonctions de normalisation
def standardize(data):
    mean = np.mean(data)
    std = np.std(data)
    return (data - mean) / std

def min_max_scale(data):
    min_val = np.min(data)
    max_val = np.max(data)
    return (data - min_val) / (max_val - min_val)

# DataFrame pour stocker les données normalisées
dataframe = pd.DataFrame()

# Fonction de connexion
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker MQTT")
        client.subscribe(MQTT_TOPIC)
    else:
        print("Échec de la connexion, code :", rc)

# Fonction de déconnexion
def on_disconnect(client, userdata, rc):
    print("Déconnecté du broker MQTT, code :", rc)
    client.loop_stop()

# Callback pour la réception des messages MQTT
def on_message(client, userdata, msg):
    global dataframe
    payload = str(msg.payload.decode("utf-8"))
    data = json.loads(payload)
    
    # Création d'un DataFrame temporaire pour les données normalisées
    temp_df = pd.DataFrame({
        'tension': standardize(np.array(data["tension"])),
        'courant': standardize(np.array(data["courant"])),
        'puissance_active': min_max_scale(np.array(data["puissance_active"])),
        'puissance_apparente': min_max_scale(np.array(data["puissance_apparente"])),
       #'power_factor': np.array(data["power_factor"]), 
        'frequence': np.array(data["frequence"]),
        'energie': min_max_scale(np.array(data["energie"]))
    })
    
    # Ajout des données normalisées au DataFrame principal
    dataframe = dataframe.append(temp_df, ignore_index=True)

# Initialisation du client MQTT
client = mqtt.Client()
client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message

# Tentative de connexion
try:
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
except Exception as e:
    print("Impossible de se connecter au broker MQTT:", e)
    exit(1)

# Démarrage de la boucle MQTT
client.loop_start()

# Boucle principale pour garder le script en cours d'exécution
try:
    while True:
        # Ici, vous pouvez ajouter d'autres logiques si nécessaire
        pass
except KeyboardInterrupt:
    print("Arrêt du script...")
    client.loop_stop()
    client.disconnect()
# Le script continue de tourner indéfiniment
#`client.loop_stop()` pour arrêter la boucle et `dataframe.to_csv("data.csv")` pour sauvegarder les données.
