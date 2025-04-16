#define SERIAL_RX_BUFFER_SIZE 512
#define SERIAL_TX_BUFFER_SIZE 28

#define DEBUG true
#define DEBUG_PRINT(x)    if (DEBUG) Serial.print(x)
#define DEBUG_PRINTLN(x)  if (DEBUG) Serial.println(x)

#include <WiFiEspAT.h>
#include <SD.h>
#include <SPI.h>

#define AT_BAUD_RATE 9600
#define ESP_SERIAL Serial1

char ssid[] = "Afaq-2";
char pass[] = "Afaq@2030";
const char* server = "192.168.0.92";

WiFiClient client;

void setup() {
  Serial.begin(9600);
  ESP_SERIAL.begin(AT_BAUD_RATE);
  WiFi.init(ESP_SERIAL);

  DEBUG_PRINTLN("Initializing WiFi...");

  if (WiFi.status() == WL_NO_MODULE) {
    DEBUG_PRINTLN("❌ WiFi module not found!");
    while (true);
  }

  DEBUG_PRINTLN("Setting WiFi credentials...");
  WiFi.begin(ssid, pass);

  DEBUG_PRINT("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    DEBUG_PRINT(".");
  }

  DEBUG_PRINTLN("\n✅ WiFi connected!");
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(ssid);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  DEBUG_PRINT("Initializing SD card... ");
  if (!SD.begin()) {
    DEBUG_PRINTLN("❌ SD card failed to initialize.");
    while (true);
  }
  DEBUG_PRINTLN("✅ SD card initialized.");

  downloadHexFile();
  printHexFile();  // Show downloaded contents
}

void loop() {
  // nothing
}

void downloadHexFile() {
  DEBUG_PRINTLN("📥 Starting firmware download...");

  const unsigned long CONNECT_TIMEOUT = 10000;
  const unsigned long BODY_TIMEOUT = 10000;

  DEBUG_PRINT("🔌 Connecting to server ");
  DEBUG_PRINTLN(server);

  unsigned long connectStart = millis();
  bool connected = false;

  while (millis() - connectStart < CONNECT_TIMEOUT) {
    if (client.connect(server, 80)) {
      connected = true;
      break;
    }
    delay(100);
  }

  if (!connected) {
    DEBUG_PRINTLN("❌ Connection to server timed out.");
    return;
  }

  DEBUG_PRINT("🌐 Connected after ");
  DEBUG_PRINT(millis() - connectStart);
  DEBUG_PRINTLN(" ms");

  // Send the request
  client.println("GET /2560_1_0_0.hex HTTP/1.1");
  client.print("Host: ");
  client.println(server);
  client.println("Connection: close");
  client.println();

  File file = SD.open("firmware.hex", FILE_WRITE);
  if (!file) {
    DEBUG_PRINTLN("❌ Failed to open file on SD card.");
    return;
  }
  DEBUG_PRINTLN("✅ File opened on SD for writing.");

  bool bodyStarted = false;
  int lineCount = 0;
  unsigned long lastDataTime = millis();
  bool timeout = false;

  DEBUG_PRINTLN("⌛ Waiting for HTTP response body...");

  while (client.connected()) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      line.trim();  // ← very important
      lastDataTime = millis();

      // Check if HTTP headers ended
      if (!bodyStarted) {
        DEBUG_PRINT("↪ Header Line: ");
        DEBUG_PRINTLN(line);

        if (line.length() == 0) {
          bodyStarted = true;
          DEBUG_PRINTLN("✅ HTTP header ended. Starting to write body...");
          continue;
        }
      } else {
        file.println(line);
        DEBUG_PRINT("📄 Writing line ");
        DEBUG_PRINTLN(lineCount++);
      }
    }

    if (millis() - lastDataTime > BODY_TIMEOUT) {
      DEBUG_PRINTLN("⚠️ Timeout while waiting for more data.");
      timeout = true;
      break;
    }
  }

  file.close();
  client.stop();

  if (lineCount > 0 && !timeout) {
    DEBUG_PRINT("✅ Firmware download complete with ");
    DEBUG_PRINT(lineCount);
    DEBUG_PRINTLN(" lines.");
  } else if (timeout) {
    DEBUG_PRINTLN("❌ Firmware download incomplete due to timeout.");
  } else {
    DEBUG_PRINTLN("❌ No data received.");
  }
}



void printHexFile() {
  DEBUG_PRINTLN("\n📄 Printing contents of 'firmware.hex'...\n");

  File file = SD.open("firmware.hex");
  if (!file) {
    DEBUG_PRINTLN("❌ Error opening firmware.hex for reading.");
    return;
  }

  int lineNum = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    DEBUG_PRINT("Line ");
    DEBUG_PRINT(lineNum++);
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(line);
  }

  file.close();
  DEBUG_PRINTLN("\n✅ Done reading firmware file.");
}
