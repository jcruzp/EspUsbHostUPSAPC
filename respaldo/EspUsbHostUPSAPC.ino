#include "EspUsbHostUPSAPC.h"

class MyEspUsbHostUPSAPC : public EspUsbHostUPSAPC {

public:
  String value;
  void printUPSStatus(int flags) {
    Serial.print("UPS Status: ");
    if (flags & 0b00000001) Serial.print("ON BATTERY ");
    if (flags & 0b00000010) Serial.print("LOW BATTERY ");
    if (flags & 0b00000100) Serial.print("OVERLOAD ");
    if (flags & 0b00001000) Serial.print("ONLINE ");
    Serial.println();
  }

  // void parseUPSData(const uint8_t* data, const size_t len) {
  //   //if (len < 8) {  // Ensure at least 8 bytes received
  //   //  Serial.println("Invalid UPS data length!");
  //   //  return;
  //   //}
  //   Serial.print("Raw UPS Data: ");
  //   for (int i = 0; i < len; i++) {
  //     Serial.print(data[i], HEX);
  //     Serial.print(" ");
  //   }
  //   Serial.println();

  //   int battery_level = data[1];                    // Battery percentage (assumed)
  //   int input_voltage = (data[3] << 8) | data[2];   // Little-endian (low + high byte)
  //   int load_percentage = data[4];                  // Load %
  //   int output_voltage = (data[6] << 8) | data[5];  // Little-endian
  //   Serial.print("Battery: ");
  //   Serial.print(battery_level);
  //   Serial.println("%");
  //   Serial.print("Input Voltage: ");
  //   Serial.print(input_voltage);
  //   Serial.println("V");
  //   Serial.print("Output Voltage: ");
  //   Serial.print(output_voltage);
  //   Serial.println("V");
  //   Serial.print("Load: ");
  //   Serial.print(load_percentage);
  //   Serial.println("%");
  //   Serial.print("UPS Status Flags: ");
  //   Serial.println(data[7], BIN);  // Print status flags in binary
  //   printUPSStatus(data[7]);
  // }

  void parseUPSData(const uint8_t* data, const size_t len) {
       Serial.print("UPS en modo "); 
       Serial.println((data[2]==0x01) && (data[3]==0x10) ? "bateria" : "normal");
  }

  // Buffer to store the report descriptor
  uint8_t descBuffer[512];
  //uint16_t descSize = 0;

  // Parse HID Report Descriptor
  void ParseHIDReportDescriptor(uint8_t* desc, uint16_t len) {
  }


  int descriptorLength = this->reportDescriptor.size();

  void parseHIDDescriptor() {
    uint8_t currentReportID = 0;
    bool trackingReport19 = false;
    int currentPosition = 0;

    Serial.println("Analizando descriptor HID...\n");

    while (currentPosition < descriptorLength) {
      uint8_t prefix = this->reportDescriptor[currentPosition];
      uint8_t itemSize = prefix & 0x03;
      uint8_t itemType = (prefix >> 2) & 0x03;
      uint8_t itemTag = (prefix >> 4);

      int dataSize = 0;
      switch (itemSize) {
        case 0: dataSize = 0; break;
        case 1: dataSize = 1; break;
        case 2: dataSize = 2; break;
        case 3: dataSize = 4; break;
      }

      // Verificar si hay suficientes bytes restantes
      if (currentPosition + 1 + dataSize > descriptorLength) break;

      // Procesar ítems globales
      if (itemType == 1) {  // Ítem Global
        switch (itemTag) {
          case 0x8:  // Report ID
            currentReportID = this->reportDescriptor[currentPosition + 1];
            if (currentReportID == 0x13) {
              trackingReport19 = true;
              Serial.println("=== REPORT ID 19 ENCONTRADO ===");
              Serial.print("Posicion: ");
              Serial.print(currentPosition);
              Serial.print("-");
              Serial.println(currentPosition + 1);
              Serial.println("===============================");
            } else {
              trackingReport19 = false;
            }
            break;
        }
      }

      // Mostrar detalles si estamos en el Report ID 19
      if (trackingReport19) {
        Serial.print("[Pos ");
        Serial.print(currentPosition);
        Serial.print("] Tipo: ");

        switch (itemType) {
          case 0: Serial.print("Main"); break;
          case 1: Serial.print("Global"); break;
          case 2: Serial.print("Local"); break;
          default: Serial.print("Reservado");
        }

        Serial.print(" | Tag: 0x");
        Serial.print(itemTag, HEX);
        Serial.print(" | Datos: ");

        for (int i = 1; i <= dataSize; i++) {
          Serial.print("0x");
          Serial.print(this->reportDescriptor[currentPosition + i], HEX);
          Serial.print(" ");
        }

        Serial.println();
      }

      currentPosition += 1 + dataSize;
    }

    if (!trackingReport19) {
      Serial.println("\nReport ID 19 no encontrado en el descriptor");
    }
  }

  struct HIDReportInfo {
    uint8_t reportId;
    uint8_t reportSize;
    uint8_t reportCount;
    uint8_t usagePage;
  };

  HIDReportInfo targetReport;

  void parseHIDDescriptor2(uint8_t targetId) {
    uint8_t currentReportId = 0;
    bool inTargetReport = false;

    for (size_t i = 0; i < this->reportDescriptor.size();) {
      uint8_t item = this->reportDescriptor[i];
      uint8_t size = item & 0x03;
      uint8_t type = (item >> 2) & 0x03;
      uint8_t tag = (item >> 4);

      // Procesar Report ID
      if (type == 1 && tag == 8) {  // Global Item - Report ID
        currentReportId = this->reportDescriptor[i + 1];
        if (currentReportId == targetId) {
          inTargetReport = true;
          targetReport.reportId = currentReportId;
          Serial.println("\nReporte 19 encontrado!");
        } else {
          inTargetReport = false;
        }
      }

      // Capturar parámetros específicos
      if (inTargetReport) {
        switch (tag) {
          case 0x00:  // Usage Page
            if (type == 1) targetReport.usagePage = this->reportDescriptor[i + 1];
            break;

          case 0x07:  // Report Size
            if (type == 1) targetReport.reportSize = this->reportDescriptor[i + 1];
            break;

          case 0x09:  // Report Count
            if (type == 1) targetReport.reportCount = this->reportDescriptor[i + 1];
            break;
        }
      }

      // Avanzar al siguiente ítem
      i += 1 + size;
    }

    // Mostrar resultados
    Serial.println("=== Información del Reporte 19 ===");
    Serial.print("Tamaño total: ");
    Serial.print(targetReport.reportSize * targetReport.reportCount / 8);
    Serial.println(" bytes");
    Serial.print("Usage Page: 0x");
    Serial.println(targetReport.usagePage, HEX);
  }

  void onNew() override {
    Serial.println("connected");
    Serial.println(("Manufacturer:" + getManufacturer()).c_str());
    Serial.println(("Product:" + getProduct()).c_str());
    Serial.print("ReportDescriptor:");

    parseHIDDescriptor2(19);

    // //descSize = this->reportDescriptor.size();
    // //memcpy(descBuffer, this->reportDescriptor[], this->reportDescriptor.size());
    // uint16_t offset = 0;

    // while (offset < this->reportDescriptor.size()) {
    //   uint8_t item = this->reportDescriptor[offset];
    //   uint8_t tag = (item >> 4) & 0x0F;  // For Main items (simplified)
    //   uint8_t type = item & 0x03;
    //   uint8_t size = (item & 0x0C) >> 2;

    //   // Handle item size
    //   uint8_t dataSize = 0;
    //   if (type == 2) {  // Long item (rare)
    //     dataSize = this->reportDescriptor[offset + 1];
    //     offset += 3;  // Skip prefix and size bytes
    //   } else {        // Short item
    //     dataSize = (item & 0x03);
    //     offset += 1;
    //   }

    //   // Extract data bytes
    //   uint32_t data = 0;
    //   for (uint8_t i = 0; i < dataSize; i++) {
    //     data |= ((uint32_t)this->reportDescriptor[offset + i]) << (8 * i);
    //   }

    //   // Print item info
    //   Serial.print("Item: ");
    //   Serial.print(item, HEX);
    //   Serial.print(" | Type: ");
    //   switch (type) {
    //     case 0: Serial.print("Main"); break;
    //     case 1: Serial.print("Global"); break;
    //     case 2: Serial.print("Local"); break;
    //     default: Serial.print("Reserved"); break;
    //   }
    //   Serial.print(" | Tag: 0x");
    //   Serial.print(tag, HEX);
    //   Serial.print(" | Data: 0x");
    //   Serial.println(data, HEX);

    //   offset += dataSize;
    // }

    // Serial.println("Descriptor HID capturado:");
    // for (uint16_t i = 0; i < this->reportDescriptor.size(); i++) {
    //   Serial.printf(" %02x",this->reportDescriptor[i]);
    //   //Serial.print(" ");
    // }
    // Serial.println();


    // Parse the report descriptor
    //for (int i = 0; i < this->reportDescriptor.size(); i++) {
    //if (!(i % 16))
    //  Serial.println();
    //  Serial.printf(" %02x", this->reportDescriptor[i]);
    //}
    Serial.println(" End report descriptor...");
  }

  // void onKey(const uint8_t* data, const size_t length) override {
  //   Serial.printf("length=%d", length);
  //   for (int i = 0; i < length; i++)
  //     Serial.printf(" %02x", data[i]);
  //   Serial.println();
  //   //parseUPSData(data, length);
  // }


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
