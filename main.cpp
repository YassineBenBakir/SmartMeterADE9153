#include <ArduinoJson.h>
#include <NTPClient.h>
#include <SPI.h>
#include "ADE9153A.h"
#include "ADE9153AAPI.h"
#include <LoRaWan.h>
#include <CayenneLPP.h>

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


void readandwrite(void);
void resetADE9153A(void);

unsigned long lastReport = 0;
const long reportInterval = 2000;  // Intervalle de rapport/ 2s
const long blinkInterval = 500;
int inputState = LOW;
int ledState = LOW;

unsigned char data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA};
char buffer[256];

CayenneLPP lpp(51); // Initialise CayenneLPP avec une taille de buffer de 51 octets




void setup() {
  Serial.begin(115200);
  pinMode(CS, OUTPUT);     
  pinMode(LED, OUTPUT);
  pinMode(USER_INPUT, INPUT);
  pinMode(ADE9153A_RESET_PIN, OUTPUT);
  digitalWrite(ADE9153A_RESET_PIN, HIGH);  
  Serial.begin(115200);
      while (!SerialUSB);

    lora.init();

    memset(buffer, 0, 256);
    lora.getVersion(buffer, 256, 1);
    SerialUSB.print(buffer);

    memset(buffer, 0, 256);
    lora.getId(buffer, 256, 1);
    SerialUSB.print(buffer);

    lora.setKey(0, 0, "2C2E0EC80603B6134C1B5658337BDD4A");

    lora.setDeciveMode(LWOTAA);
    lora.setDataRate(DR0, EU868);

    lora.setChannel(0, 868.1);
    lora.setChannel(1, 868.3);
    lora.setChannel(2, 868.5);

    lora.setReceiceWindowFirst(0, 868.1);
    lora.setReceiceWindowSecond(869.5, DR3);

    lora.setDutyCycle(false);
    lora.setJoinDutyCycle(false);

    lora.setPower(14);

    while (!lora.setOTAAJoin(JOIN));
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

}

void loop() {
  lpp.reset();
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
    short length;
    short rssi;

    memset(buffer, 0, 256);
    length = lora.receivePacket(buffer, 256, &rssi);

      if (length) {
          SerialUSB.print("Length is: ");
          SerialUSB.println(length);
          SerialUSB.print("RSSI is: ");
          SerialUSB.println(rssi);
          SerialUSB.print("Data is: ");
          for (unsigned char i = 0; i < length; i++) {
              SerialUSB.print("0x");
              SerialUSB.print(buffer[i], HEX);
              SerialUSB.print(" ");
          }
          SerialUSB.println();
      }
  }

}

void readandwrite() {
 /* Read and Print WATT Register using ADE9153A Read Library */
  ade9153A.ReadPowerRegs(&powerVals);    //Template to read Power registers from ADE9000 and store data in Arduino MCU
  ade9153A.ReadRMSRegs(&rmsVals);
  ade9153A.ReadPQRegs(&pqVals);
  ade9153A.ReadTemperature(&tempVal);
  
  Serial.print("RMS Current:\t");        
  lpp.addCurrent(4, rmsVals.CurrentRMSValue/1000); 
  Serial.println(" A");
  
  Serial.print("RMS Voltage:\t");        
  lpp.addVoltage(3, rmsVals.VoltageRMSValue/100);
  Serial.println(" V");
  
  Serial.print("Active Power:\t");        
  lpp.addAnalogInput(5,powerVals.ActivePowerValue);
  Serial.println(" W");
  
  Serial.print("Reactive Power:\t");        
  lpp.addAnalogInput(6,powerVals.FundReactivePowerValue);
  Serial.println(" VAR");
  
  Serial.print("Apparent Power:\t");        
  lpp.addAnalogInput(7,powerVals.ActivePowerValue);
  Serial.println(" VA");
  
  Serial.print("Power Factor:\t");        
  lpp.addAnalogInput(9,powerVals.ApparentPowerValue);
  
  Serial.print("Temperature:\t");   
  lpp.addTemperature(8, temp)     
  Serial.println(" degC");

  Serial.println("");
  Serial.println("");

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
