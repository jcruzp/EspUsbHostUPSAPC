#include "EspUsbHostUPSAPC.h"

class MyEspUsbHostUPSAPC : public EspUsbHostUPSAPC {

private:
  unsigned long previousMillis = 0;  // Almacena el tiempo en el que se ejecutó por última vez su función
  const long interval = 30000;       // Intervalo de 10 segundos (10,000 milisegundos)
  boolean shutdown_ups = false;

public:

  void parseUPSData(const uint8_t* data, const size_t len) {
    unsigned long currentMillis = millis();
    Serial.print("UPS en modo ");
    // Data length=4 06 00 00 08 --> Normal
    // Data length=4 06 00 01 10 --> Bateria
    Serial.println((data[2] == 0x01) && (data[3] == 0x10) ? "bateria" : "normal");
    if ((data[2] == 0x01) && (data[3] == 0x10)) {  //Modo bateria
      if (previousMillis == 0) previousMillis = currentMillis;
      if ((currentMillis - previousMillis >= interval) && (!shutdown_ups)) {
        // Guarda el "tiempo actual" como el "tiempo anterior", para la próxima comparación
        Serial.println("Ejecutar Shutdown..");
        //testKill();
        shutdown_ups = true;
      }
    } else
      previousMillis = 0;
  }

  void onNew() override {
    Serial.println("connected");
    Serial.println(("Manufacturer:" + getManufacturer()).c_str());
    Serial.println(("Product:" + getProduct()).c_str());
    //delay(10000);
    //shutDownUPS();
    //testAlarm();  //ok
    testKill();
    //testKill(); //ok
    //verboseTestAlarm();
    //testMultipleInterfaces();
    //discoverCommands();
    //testAlarmWithResponse();
    //testAlarmAlternatives();
    delay(10000);
  }

  virtual void
  onReceive(const uint8_t* data, const size_t length) override {
    Serial.printf("length=%d", length);
    for (int i = 0; i < length; i++)
      Serial.printf(" %02x", data[i]);
    Serial.println();
    if (data[0] == 0x06)
      parseUPSData(data, length);
  }

  virtual void onGone() {
    Serial.println("disconnected");
  }
};

MyEspUsbHostUPSAPC usbDev;

void setup() {
  Serial.begin(115200);

  usbDev.begin();
}

void loop() {
  usbDev.task();
}
