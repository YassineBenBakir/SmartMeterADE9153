#include <vector>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include "ADE9153A.h"
#include "ADE9153AAPI.h"

#define SPI_SPEED 1000000  // SPI Speed
#define CS 15
#define USER_INPUT 5          //On-board User Input Button Pin
#define ADE9153A_RESET_PIN 4  //On-board Reset Pin
#define LED 6                 //On-board LED pin

ADE9153AClass ade9153A;

struct EnergyRegs energyVals;  //Energy register values are read and stored in EnergyRegs structure
struct PowerRegs powerVals;    //Metrology data can be accessed from these structures
struct RMSRegs rmsVals;  
struct PQRegs pqVals;
struct AcalRegs acalVals;
struct Temperature tempVal;

// Vecteurs pour stocker les lectures
std::vector<float> tensions, courants, puissances_actives, puissances_apparentes, facteurs_puissance, frequences, energies;
const int nombreLecturesAvantEnvoi = 10;  // Nombre de lectures avant envoi Par exemple, 10 lectures

void readandwrite(void);
void resetADE9153A(void);

unsigned long lastReport = 0;
const long reportInterval = 2000;  // Intervalle de rapport/ 2s
const long blinkInterval = 500;
int inputState = LOW;
int ledState = LOW;

// Paramètres WiFi
const char *ssid = "";  // Nom du WiFi
const char *password = "";  // Mot de passe WiFi
// Paramètres MQTT
const char *mqtt_broker = "broker.mqttdashboard.com";
const char *topic = "montopic";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

char buffer[200];
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClient espClient;
PubSubClient client(espClient);

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connexion WiFi perdue. Tentative de reconnexion...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnecté");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(CS, OUTPUT);     
  pinMode(LED, OUTPUT);
  pinMode(USER_INPUT, INPUT);
  pinMode(ADE9153A_RESET_PIN, OUTPUT);
  digitalWrite(ADE9153A_RESET_PIN, HIGH);  
  Serial.begin(115200);
  resetADE9153A();            //Reset ADE9153A for clean startup
  delay(1000);
  // Initialize the ADE9153A chip and SPI communication and check whether the device has been properly connected. 
  bool commscheck = ade9153A.SPI_Init(SPI_SPEED, CS); //Initialize SPI
  if (!commscheck) {
    Serial.println("ADE9153A Shield not detected. Plug in Shield and reset the Arduino");
    while (!commscheck) {   //Hold until arduino is reset
      delay(1000);
    }
  }
  ade9153A.SetupADE9153A(); //Setup ADE9153A according to ADE9153AAPI.h
  /* Read and Print Specific Register using ADE9153A SPI Library */
  Serial.println(String(ade9153A.SPI_Read_32(REG_VERSION_PRODUCT), HEX)); // Version of IC
  ade9153A.SPI_Write_32(REG_AIGAIN, -268435456); //AIGAIN to -1 to account for IAP-IAN swap
  delay(500); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.begin();
  timeClient.setTimeOffset(3600);
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  mqttReconnect();
  client.subscribe(topic);
}

void loop() {
  reconnectWiFi();
  if (!client.connected()) {
    mqttReconnect();
  }
  client.loop();
  /* Returns metrology to the serial monitor and waits for USER_INPUT button press to run autocal */

  unsigned long currentReport = millis();
  if (currentReport - lastReport >= reportInterval) {
    readandwrite();
    lastReport = currentReport;
  }
   inputState = digitalRead(USER_INPUT);

  if (inputState == LOW) {
    Serial.println("Autocalibrating Current Channel");
    ade9153A.StartAcal_AINormal();
    runLength(20);
    ade9153A.StopAcal();
    Serial.println("Autocalibrating Voltage Channel");
    ade9153A.StartAcal_AV();
    runLength(40);
    ade9153A.StopAcal();
    delay(100);
    
    ade9153A.ReadAcalRegs(&acalVals);
    Serial.print("AICC: ");
    Serial.println(acalVals.AICC);
    Serial.print("AICERT: ");
    Serial.println(acalVals.AcalAICERTReg);
    Serial.print("AVCC: ");
    Serial.println(acalVals.AVCC);
    Serial.print("AVCERT: ");
    Serial.println(acalVals.AcalAVCERTReg);
    long Igain = (-(acalVals.AICC / 838.190) - 1) * 134217728;
    long Vgain = ((acalVals.AVCC / 13411.05) - 1) * 134217728;
    ade9153A.SPI_Write_32(REG_AIGAIN, Igain);
    ade9153A.SPI_Write_32(REG_AVGAIN, Vgain);
    
    Serial.println("Autocalibration Complete");
    delay(2000);
  }
}

void readandwrite() {
 /* Read and Print WATT Register using ADE9153A Read Library */
  ade9153A.ReadPowerRegs(&powerVals);    //Template to read Power registers from ADE9000 and store data in Arduino MCU
  ade9153A.ReadRMSRegs(&rmsVals);
  ade9153A.ReadPQRegs(&pqVals);
  ade9153A.ReadTemperature(&tempVal);
  
  Serial.print("RMS Current:\t");        
  Serial.print(rmsVals.CurrentRMSValue/1000); 
  Serial.println(" A");
  
  Serial.print("RMS Voltage:\t");        
  Serial.print(rmsVals.VoltageRMSValue/1000);
  Serial.println(" V");
  
  Serial.print("Active Power:\t");        
  Serial.print(powerVals.ActivePowerValue/1000);
  Serial.println(" W");
  
  Serial.print("Reactive Power:\t");        
  Serial.print(powerVals.FundReactivePowerValue/1000);
  Serial.println(" VAR");
  
  Serial.print("Apparent Power:\t");        
  Serial.print(powerVals.ApparentPowerValue/1000);
  Serial.println(" VA");
  
  Serial.print("Power Factor:\t");        
  Serial.println(pqVals.PowerFactorValue);
  
  Serial.print("Frequency:\t");        
  Serial.print(pqVals.FrequencyValue);
  Serial.println(" Hz");
  
  Serial.print("Temperature:\t");        
  Serial.print(tempVal.TemperatureVal);
  Serial.println(" degC");

  Serial.println("");
  Serial.println("");
 // Ajout des lectures aux vecteurs pour l'envoi
  tensions.push_back(rmsVals.VoltageRMSValue/1000);
  courants.push_back(rmsVals.CurrentRMSValue/1000);
  puissances_actives.push_back(powerVals.ActivePowerValue/1000);
  puissances_apparentes.push_back(powerVals.ApparentPowerValue/1000);
  facteurs_puissance.push_back(pqVals.PowerFactorValue);
  frequences.push_back(pqVals.FrequencyValue);
  energies.push_back(powerVals.ActiveEnergyValue/1000);

  if (tensions.size() >= nombreLecturesAvantEnvoi) {
    // [Préparation et envoi des données]
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = "0001";
    doc["time_stamp"] = timeClient.getFormattedTime();
    doc["tension"] = tensions;
    doc["courant"] = courants;
    doc["puissance_active"] = puissances_actives;
    doc["puissance_apparente"] = puissances_apparentes;
    doc["power_factor"] = facteurs_puissance;
    doc["frequence"] = frequences;
    doc["energie"] = energies;

    serializeJson(doc, buffer);
    client.publish("montopic", buffer);

    // Vider les vecteurs après l'envoi
    tensions.clear();
    courants.clear();
    puissances_actives.clear();
    puissances_apparentes.clear();
    facteurs_puissance.clear();
    frequences.clear();
    energies.clear();
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}

void mqttReconnect() {
  while (!client.connected()) {
    Serial.println("Tentative de connexion au broker MQTT...");
    if (client.connect("esp32-client")) {
      Serial.println("Connecté au broker MQTT");
      client.subscribe(topic);
    } else {
      Serial.print("Echec de la connexion, rc=");
      Serial.print(client.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}
void resetADE9153A(void)
{
 digitalWrite(ADE9153A_RESET_PIN, LOW);
 delay(100);
 digitalWrite(ADE9153A_RESET_PIN, HIGH);
 delay(1000);
 Serial.println("Reset Done");
}

void runLength(long seconds)
{
  unsigned long startTime = millis();
  
  while (millis() - startTime < (seconds*1000)){
    digitalWrite(LED, HIGH);
    delay(blinkInterval);
    digitalWrite(LED, LOW);
    delay(blinkInterval);
  }  
}


