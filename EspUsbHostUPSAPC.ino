#include "EspUsbHostUPSAPC.h"

class MyEspUsbHostUPSAPC : public EspUsbHostUPSAPC {



public:

  void onNew() override {
    Serial.println("connected");
    Serial.println(("Manufacturer:" + getManufacturer()).c_str());
    Serial.println(("Product:" + getProduct()).c_str());
    //delay(10000);
    //shutDownUPS();
    //testAlarm();  //ok
    //testKill();
    //testKill(); //ok
    //verboseTestAlarm();
    //testMultipleInterfaces();
    //discoverCommands();
    //testAlarmWithResponse();
    //testAlarmAlternatives();
    delay(2000);
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
