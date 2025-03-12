#ifndef __EspUsbHostUPSAPC_H__
#define __EspUsbHostUPSAPC_H__

#include "EspUsbHostHID.h"

class EspUsbHostUPSAPC : public EspUsbHostHID {

private:
  unsigned long previousMillis = 0;  // Almacena el tiempo en el que se ejecutó por última vez su función
  const long interval = 30000;       // Intervalo de 10 segundos (10,000 milisegundos)
  boolean shutdown_ups = false;

public:
  EspUsbHostUPSAPC()
    : EspUsbHostHID(0x00){};

  void parseUPSData(const uint8_t* data, const size_t len) {
    unsigned long currentMillis = millis();
    Serial.print("UPS en modo ");
    // Data length=4 06 00 00 08 --> Normal
    // Data length=4 06 00 01 10 --> Bateria
    // Data length=4 06 00 00 00 --> Apagado

    if ((data[2] == 0x00) && (data[3] == 0x00))
      Serial.println("apagado");
    else
      Serial.println((data[2] == 0x01) && (data[3] == 0x10) ? "bateria" : "normal");
    if ((data[2] == 0x01) && (data[3] == 0x10)) {  //Modo bateria
      if (previousMillis == 0) previousMillis = currentMillis;
      if ((currentMillis - previousMillis >= interval) && (!shutdown_ups)) {
        // Guarda el "tiempo actual" como el "tiempo anterior", para la próxima comparación
        Serial.println("Ejecutar Shutdown..");
        soundAlarm();
        delay(1000);
        shutDown();
        shutdown_ups = true;
      }
    } else
      previousMillis = 0;
  }

  virtual void soundAlarm() {
    uint8_t alarm_command[2] = { 0x79, 0x01 };  // Comando de prueba de alarma

    // Enviamos el comando mediante control transfer utilizando:
    // - requestType: 0x21 (USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)
    // - bRequest: 0x09 (HID_SET_REPORT)
    // - wValue: 0x0379 ((HID_REPORT_TYPE_OUTPUT << 8) | report_id) donde report_id es 0x79
    // - wIndex: 0x0000
    // - wLength: sizeof(alarm_command)
    // - data: alarm_command
    esp_err_t result = submit_control(0x21, 0x09, 0x0379, 0x0000, sizeof(alarm_command), &alarm_command);

    ESP_LOGI("soundAlarm", "Comando de prueba de alarma enviado: %d", result);
    if (result == ESP_OK) {
      Serial.println("Comando de prueba de alarma enviado con éxito");
    } else {
      Serial.printf("Error al enviar comando de prueba de alarma: %d\n", result);
    }
  }

  virtual void shutDown() {
    uint8_t shutdown_command[3] = { 0x15, 0x3C, 0x00 };  // Comando de shutdown

    esp_err_t result = submit_control(0x21, 0x09, 0x0315, 0x0000, sizeof(shutdown_command), &shutdown_command);

    ESP_LOGI("shutDown", "Comando Shutdown: %d", result);
    if (result == ESP_OK) {
      Serial.println("Comando shutdown enviado con éxito");
    } else {
      Serial.printf("Error al enviar comando shutdown: %d\n", result);
    }
  }
};

#endif