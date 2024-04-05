#include <LoRaWan.h>
#include <CayenneLPP.h>

unsigned char data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA};
char buffer[256];

CayenneLPP lpp(51); // Initialise CayenneLPP avec une taille de buffer de 51 octets

void setup(void) {
    SerialUSB.begin(115200);
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
}

void loop(void) {
    bool result = false;
  
    lpp.reset();

    float voltage = random(0,300);
    lpp.addVoltage(3, voltage/100);

    float current = random(0,200);
    lpp.addCurrent(4, current/1000);


    // Envoyer le paquet via LoRaWAN
    result = lora.transferPacket(lpp.getBuffer(), lpp.getSize(), 120);

    if (result) {
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

    // Délai avant la prochaine transmission pour éviter le surchargement du réseau
    delay(10000); // Délai de 10 secondes
}
