#define DEBUG true
#define DEBUG_PRINT(x)    if (DEBUG) Serial.print(x)
#define DEBUG_PRINTLN(x)  if (DEBUG) Serial.println(x)

#define ESP_SERIAL Serial1  // Mega2560 Serial1
#define AT_TIMEOUT  5000

#include <SD.h>
#include <SPI.h>
#include <avr/wdt.h>

const char* ssid = "Afaq-2";
const char* pass = "Afaq@2030";
const char* host = "192.168.0.92";
const int   port = 80;
const char* path = "/2560_1_0_0.hex";
File                   file; 

void triggerReset() {
  wdt_enable(WDTO_15MS);  // Enable watchdog for 15ms
  while (1);              // Wait for WDT reset
}

bool sendAT(String cmd, int timeout = AT_TIMEOUT, String expected = "OK") {
  ESP_SERIAL.println(cmd);
  unsigned long start = millis();
  while (millis() - start < timeout) {
    if (ESP_SERIAL.find(const_cast<char*>(expected.c_str()))) {
      DEBUG_PRINT("✅ ");
      DEBUG_PRINTLN(cmd);
      return true;
    }
  }
  DEBUG_PRINT("❌ ");
  DEBUG_PRINTLN(cmd);
  return false;
}

void setup() {
  Serial.begin(9600);
  ESP_SERIAL.begin(9600);
  delay(1000);
  

  // Init SD card
  DEBUG_PRINT("Initializing SD card... ");
  if (!SD.begin()) {
    DEBUG_PRINTLN("❌ SD card init failed!");
    return;
  }
  DEBUG_PRINTLN("✅ SD OK");
// Check to see if the file exists:
  if (SD.exists("firmware.hex")) {
    Serial.println("firmware.hex exists.");
    // delete the file:
  Serial.println("Removing firmware.hex...");
  SD.remove("firmware.hex");
  } else {
    Serial.println("firmware.hex doesn't exist.");
  }
  // // Start ESP8266
  // sendAT("AT");
  // sendAT("AT+CWMODE=1");
  // sendAT("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(pass) + "\"", 15000);
  // sendAT("AT+CIPMUX=0");
  // Start ESP8266
  sendAT("AT");
  // sendAT("AT+CWMODE=2");
  // sendAT("AT+CIPMUX=1");
  // sendAT("AT+CIPSERVER=1,80",15000);

  // // Start TCP connection
  // String startTCP = "AT+CIPSTART=\"TCP\",\"" + String(host) + "\"," + String(port);
  // if (!sendAT(startTCP, 5000, "OK")) {
  //   DEBUG_PRINTLN("❌ TCP Connect Failed");
  //   return;
  // }

  // // Create HTTP GET request
  // String request = "GET " + String(path) + " HTTP/1.1\r\nHost: " + String(host) + "\r\nConnection: close\r\n\r\n";
  // sendAT("AT+CIPSEND=" + String(request.length()), 5000, ">");

  // ESP_SERIAL.print(request); // Send HTTP request

  // // Save to SD
  // file = SD.open("firmware.hex", FILE_WRITE);
  // if (!file) {
  //   DEBUG_PRINTLN("❌ Failed to open firmware.hex for writing");
  //   return;
  // }

  // DEBUG_PRINTLN("📥 Receiving file...");
  // bool headerEnded = false;
  // unsigned long lastData = millis();

  // while (millis() - lastData < 10000) {
  //   if (ESP_SERIAL.available()) {
  //     lastData = millis();
  //     String line = ESP_SERIAL.readStringUntil('\n');
  //     line.trim();

  //     if (!headerEnded) {
  //       DEBUG_PRINT("↪ Header: ");
  //       DEBUG_PRINTLN(line);
  //       if (line.length() == 0) {
  //         headerEnded = true;
  //         DEBUG_PRINTLN("✅ Header Ended. Writing body...");
  //       } else {
  //         DEBUG_PRINT("↪ ");
  //         DEBUG_PRINTLN(line);
  //       }
  //     } else {
  //       file.println(line);
  //       DEBUG_PRINTLN("📄 Saved line: " + line);
  //     }
  //   }
  // }

  // file.close();
  // sendAT("AT+CIPCLOSE");
  // DEBUG_PRINTLN("✅ Done. File saved as 'firmware.hex'");
  // DEBUG_PRINTLN("🔁 Ready for bootloader reset.");

  //   // re-open the file for reading:
  // file = SD.open("firmware.hex");
  // if (file) {
  //   Serial.println("firmware.hex:");

  //   // read from the file until there's nothing else in it:
  //   while (file.available()) {
  //     Serial.print(file.read());
  //   }
  //   // close the file:
  //   file.close();
  // } else {
  //   // if the file didn't open, print an error:
  //   Serial.println("error opening firmware.hex");
  // }

  // WiFi connection
  sendAT("AT");
  sendAT("AT+CWMODE=1");
  sendAT("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(pass) + "\"", 10000);
  sendAT("AT+CIPMUX=0");

  // Start TCP connection
  sendAT("AT+CIPSTART=\"TCP\",\"" + String(host) + "\"," + String(port), 5000, "OK");

  // HTTP request
  String request = "GET " + String(path) + " HTTP/1.1\r\nHost: " + String(host) + "\r\nConnection: close\r\n\r\n";
  sendAT("AT+CIPSEND=" + String(request.length()), 3000, ">");
  ESP_SERIAL.print(request);  // send the request
  
  receiveFileFromESP();

  //   if (SD.exists("firmware.hex")) {
  //   Serial.println("✅ firmware.hex found. Triggering bootloader update...");
  //   delay(1000);
  //   triggerReset();  // Let bootloader flash it
  // } else {
  //   Serial.println("❌ firmware.hex not found.");
  // }
}

void loop() {}



void receiveFileFromESP() {
  Serial.println("⌛ Waiting for +IPD...");

  File file = SD.open("firmware.hex", FILE_WRITE);
  if (!file) {
    Serial.println("❌ Failed to open SD file.");
    return;
  }

  bool inBody = false;
  unsigned long start = millis();

  while (millis() - start < 20000) {
    if (ESP_SERIAL.available()) {
      String line = ESP_SERIAL.readStringUntil('\n');
      line.trim();

      if (!inBody) {
        if (line.startsWith("+IPD")) {
          Serial.println("✅ +IPD detected");
          continue;
        }
        if (line.length() == 0) {
          inBody = true;  // header done
          Serial.println("📄 Starting file write...");
          continue;
        }
        Serial.println("↪ Header: " + line);
      } else {
        if (line.startsWith(":")) {
          file.println(line);
          Serial.println("📝 Saved: " + line);
        }
      }
    }
  }

  file.close();
  Serial.println("✅ File saved to SD as firmware.hex");
  sendAT("AT+CIPCLOSE");
}

