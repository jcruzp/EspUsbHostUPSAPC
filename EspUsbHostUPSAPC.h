#ifndef __EspUsbHostUPSAPC_H__
#define __EspUsbHostUPSAPC_H__

#include "EspUsbHostHID.h"

class EspUsbHostUPSAPC : public EspUsbHostHID {
public:
  EspUsbHostUPSAPC()
    : EspUsbHostHID(0x00){};
  //virtual void setCallback_onKey(void (*callback)(const uint8_t *data, const size_t length)) { onKeyCB = callback; }

  //protected:
  //  virtual void onKey(const uint8_t *data, const size_t length)              { if(onKeyCB) onKeyCB(data, length); }
  //private:
  //  virtual void onReceive(const uint8_t *data, const size_t length) override { onKey(data, length); }

  //  void (*onKeyCB)(const uint8_t *data, const size_t length) = nullptr;
  virtual void testAlarm() {
    // El valor CI_TESTALARM para UPS APC es 0x39
    uint8_t alarm_command[2] = { 0x79, 0x01 };  // Comando de prueba de alarma

    // Enviamos el comando mediante control transfer utilizando:
    // - requestType: 0x21 (USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)
    // - bRequest: 0x09 (HID_SET_REPORT)
    // - wValue: 0x0239 ((HID_REPORT_TYPE_OUTPUT << 8) | report_id) donde report_id es 0x39
    // - wIndex: 0x0000
    // - wLength: sizeof(alarm_command)
    // - data: alarm_command
    esp_err_t result = submit_control(0x21, 0x09, 0x0379, 0x0000, sizeof(alarm_command), &alarm_command);

    ESP_LOGI("TestAlarm", "Comando de prueba de alarma enviado: %d", result);
    if (result == ESP_OK) {
      Serial.println("Comando de prueba de alarma enviado con éxito");
    } else {
      Serial.printf("Error al enviar comando de prueba de alarma: %d\n", result);
    }
  }

    virtual void testKill() {
    // El valor CI_TESTALARM para UPS APC es 0x39
    uint8_t alarm_command[3] = { 0x15, 0x3C, 0x00 };  // Comando de prueba de alarma

    // Enviamos el comando mediante control transfer utilizando:
    // - requestType: 0x21 (USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)
    // - bRequest: 0x09 (HID_SET_REPORT)
    // - wValue: 0x0239 ((HID_REPORT_TYPE_OUTPUT << 8) | report_id) donde report_id es 0x39
    // - wIndex: 0x0000
    // - wLength: sizeof(alarm_command)
    // - data: alarm_command
    esp_err_t result = submit_control(0x21, 0x09, 0x0315, 0x0000, sizeof(alarm_command), &alarm_command);

    ESP_LOGI("TestAlarm", "Comando de prueba de alarma enviado: %d", result);
    if (result == ESP_OK) {
      Serial.println("Comando de prueba de alarma enviado con éxito");
    } else {
      Serial.printf("Error al enviar comando de prueba de alarma: %d\n", result);
    }
  }

  void verboseTestAlarm() {
    uint8_t alarm_command[1] = { 0x39 };

    Serial.printf("Enviando comando de prueba de alarma...\n");
    Serial.printf("Device Handle: %p\n", deviceHandle);
    Serial.printf("Client Handle: %p\n", clientHandle);
    Serial.printf("Interface Number: %d\n", InterfaceNumber);

    esp_err_t result = submit_control(0x21, 0x09, 0x0239, 0x0000,
                                      sizeof(alarm_command), &alarm_command);

    Serial.printf("Resultado: %d (%s)\n", result, esp_err_to_name(result));
  }

  void testMultipleInterfaces() {
    for (int interface_num = 0; interface_num < 3; interface_num++) {
      uint8_t alarm_command[1] = { 0x39 };
      Serial.printf("Probando en interfaz %d...\n", interface_num);
      esp_err_t result = submit_control(0x21, 0x09, 0x0239, interface_num,
                                        sizeof(alarm_command), &alarm_command);
      Serial.printf("Resultado: %d\n", result);
      delay(1000);
    }
  }
  void discoverCommands() {
    // Prueba diferentes comandos comunes de UPS
    const uint8_t commands[] = { 0x39, 0x16, 0x01, 0x12, 0x4F };
    const char* cmd_names[] = { "TestAlarm APC", "TestAlarm CyberPower",
                                "Status", "SelfTest", "TestAlarm Alt" };

    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
      uint8_t cmd[1] = { commands[i] };
      Serial.printf("Probando comando %s (0x%02X)...\n", cmd_names[i], commands[i]);
      esp_err_t result = submit_control(0x21, 0x09, 0x0200 | commands[i],
                                        0x0000, sizeof(cmd), &cmd);
      Serial.printf("Resultado: %d (%s)\n", result, esp_err_to_name(result));
      delay(2000);  // Esperar para ver/escuchar respuesta
    }
  }

  void testAlarmWithResponse() {
    uint8_t alarm_command[1] = { 0x39 };

    // Enviar comando con callback personalizado para capturar respuesta
    esp_err_t result = submit_control(0x21, 0x09, 0x0239, 0x0000, sizeof(alarm_command),
                                      &alarm_command, [](usb_transfer_t* transfer) {
                                        Serial.printf("Respuesta recibida: status=%d, bytes=%d\n",
                                                      transfer->status, transfer->actual_num_bytes);
                                        // Imprimir bytes recibidos si hay alguno
                                        if (transfer->actual_num_bytes > 0) {
                                          Serial.print("Datos: ");
                                          for (int i = 0; i < transfer->actual_num_bytes; i++) {
                                            Serial.printf("%02X ", transfer->data_buffer[i]);
                                          }
                                          Serial.println();
                                        }
                                        usb_host_transfer_free(transfer);
                                      });
  }


   /**
   * Implementación de usb_write_int_to_ups para enviar comandos al UPS
   * 
   * @param data Array de enteros con los comandos a enviar
   * @param len Número de enteros en el array
   * @param report_id ID del informe HID
   * @return ESP_OK si la operación tiene éxito, código de error en caso contrario
   */
  esp_err_t usb_write_int_to_ups(const int *data, int len, int report_id) {
    // Verificar parámetros
    if (!data || len <= 0 || len > 1023) {
      ESP_LOGI("usb_write_int_to_ups", "Parámetros inválidos");
      return ESP_ERR_INVALID_ARG;
    }
    
    // Crear buffer para el comando
    uint8_t buf[1024];
    
    // Preparar buffer
    buf[0] = report_id & 0xFF;  // Primer byte es el report ID
    
    // Copiar datos al buffer, después del report ID
    for (int i = 0; i < len; i++) {
      buf[i + 1] = data[i] & 0xFF;  // Solo se usa el byte bajo
    }
    
    // Enviar comando HID SET_REPORT
    return usb_control_msg(0x21, 0x09, (0x02 << 8) | report_id, 0, len + 1, buf);
  }
  
  /**
   * Implementación de usb_control_msg para enviar comandos de control USB
   * 
   * @param requestType Tipo de petición USB
   * @param request Petición USB
   * @param value Valor de la petición
   * @param index Índice de la petición
   * @param length Longitud de los datos
   * @param data Puntero a los datos
   * @return ESP_OK si la operación tiene éxito, código de error en caso contrario
   */
  esp_err_t usb_control_msg(uint8_t requestType, uint8_t request, uint16_t value,
                           uint16_t index, uint16_t length, const void *data) {
    // Usar el método submit_control de la clase base
    return submit_control(requestType, request, value, index, length, data);
  }
  
  
  /**
   * Método alternativo para probar la alarma del UPS APC
   * Intenta diferentes enfoques para mayor compatibilidad
   */
  void testAlarmAlternatives() {
    Serial.println("Probando alternativas para comando de alarma...");
    
    // Alternativa 1: Usando usb_write_int_to_ups con report_id diferente
    int cmd1[9] = { 0x39,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
    esp_err_t result1 = usb_write_int_to_ups(cmd1, 1, 0x39);
    Serial.printf("Alternativa 1: %d (%s)\n", result1, esp_err_to_name(result1));
    delay(1000);
    
    // Alternativa 2: Usando submit_control directamente
    uint8_t cmd2[1] = { 0x39 };
    esp_err_t result2 = submit_control(0x21, 0x09, 0x0239, 0x0000, 1, cmd2);
    Serial.printf("Alternativa 2: %d (%s)\n", result2, esp_err_to_name(result2));
    delay(1000);
    
    // Alternativa 3: Probando con valor CyberPower
    int cmd3[1] = { 0x16 };
    esp_err_t result3 = usb_write_int_to_ups(cmd3, 1, 0x00);
    Serial.printf("Alternativa 3: %d (%s)\n", result3, esp_err_to_name(result3));
    delay(1000);
    
    // Alternativa 4: Formato diferente de buffer
    uint8_t cmd4[2] = { 0x39, 0x39 };
    esp_err_t result4 = submit_control(0x21, 0x09, 0x0200, 0x0000, 2, cmd4);
    Serial.printf("Alternativa 4: %d (%s)\n", result4, esp_err_to_name(result4));
  }

};

#endif