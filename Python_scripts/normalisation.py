#Ce script souscrit au topic MQTT spécifié et attend les messages.
#Lorsqu'un message est reçu, il est converti de JSON en un dictionnaire Python.
#Les valeurs sont extraites, normalisées, filtrées, puis ajoutées à un DataFrame pandas.
#Le DataFrame est mis à jour à chaque réception de message.

import paho.mqtt.client as mqtt
import json
import numpy as np
import pandas as pd

# Configuration MQTT
MQTT_BROKER = "adresse_du_broker_mqtt"
MQTT_PORT = 1883
MQTT_TOPIC = "montopic"

# Fonctions de normalisation et de filtrage
def standardize(data):
    mean = np.mean(data)
    std = np.std(data)
    return (data - mean) / std

def min_max_scale(data):
    min_val = np.min(data)
    max_val = np.max(data)
    return (data - min_val) / (max_val - min_val)

def low_pass_filter(data, window_size=3):
    return np.convolve(data, np.ones(window_size)/window_size, mode='valid')
    #le choix de la taille de la fenêtre (window_size) doit être adapté en fonction de la fréquence d'échantillonnage des données et des caractéristiques des appareils qu'on souhaite detecter
def median_filter(data, window_size=3):
    return pd.Series(data).rolling(window=window_size, center=True).median().fillna(method='bfill').fillna(method='ffill')

# DataFrame pour stocker les données traitées
dataframe = pd.DataFrame()

# Callbacks pour la gestion MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connecté au broker MQTT")
        client.subscribe(MQTT_TOPIC)
    else:
        print("Échec de la connexion, code :", rc)

def on_disconnect(client, userdata, rc):
    print("Déconnecté du broker MQTT, code :", rc)
    client.loop_stop()

# Callback pour la réception des messages MQTT
def on_message(client, userdata, msg):
    global dataframe
    payload = str(msg.payload.decode("utf-8"))
    data = json.loads(payload)
    
    # Filtrer les données
    filtered_data = {
        'tension': low_pass_filter(np.array(data["tension"])),
        'courant': low_pass_filter(np.array(data["courant"])),
        'puissance_active': median_filter(np.array(data["puissance_active"])),
        'puissance_apparente': median_filter(np.array(data["puissance_apparente"])),
        'frequence': low_pass_filter(np.array(data["frequence"])),
    }

    # Normaliser les données filtrées
    normalized_data = {key: min_max_scale(value) for key, value in filtered_data.items()}

    # Ajouter les données normalisées au DataFrame
    temp_df = pd.DataFrame(normalized_data)
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
        pass
except KeyboardInterrupt:
    print("Arrêt du script...")
    client.loop_stop()
    client.disconnect()
    dataframe.to_csv("data.csv")
