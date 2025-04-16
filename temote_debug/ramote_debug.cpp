// ===================== DebugTerminal.cpp =====================
#include "DebugTerminal.hpp"

// #if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_NANO)
// #include <SoftwareSerial.h>
// SoftwareSerial Serial1(6, 7); // RX, TX
// #define AT_BAUD_RATE 9600
// // #else
// // #define AT_BAUD_RATE 115200
// #endif
// Use Serial1 on Mega2560 (pins 18=TX1, 19=RX1)
#include <HardwareSerial.h>
// #define SerialAT   Serial1
#define ESP_SERIAL Serial1
#define AT_BAUD_RATE 9600
DebugTerminal debug;
void DebugTerminal::begin(long serialSpeed) {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(serialSpeed);
  Serial1.begin(AT_BAUD_RATE);
  delay(2000);
}

void DebugTerminal::setupWiFi(const char* ssid, const char* pass,const char* p_address) {
  Serial.println("Initializing ESP8266...");
  debug.sendAT("AT", 1000);delay(500);
  debug.sendAT("AT+RST", 2000);delay(500);
  debug.sendAT("AT+CWMODE=1");delay(500);
  // debug.sendAT("AT+CIPSTA=\"" + String(p_address) + "\"", 1000);delay(500);
  debug.sendAT("AT+CWJAP=\"" + String(ssid) + "\",\"" + String(pass) + "\"", 8000);
  debug.sendAT("AT+CIFSR", 2000);
  debug.sendAT("AT+CIPMUX=1");
  debug.sendAT("AT+CIPSERVER=1,23");
  Serial.println("Connect to ESP IP using Telnet (port 23)");
}

void DebugTerminal::checkTelnetConnection() {
  if (!clientConnected) {
    clientConnected = debug.checkClientConnected();
    if (clientConnected) {
      debug.sendMessage(
        "Welcome to Arduino Debug Terminal\r\n"
        "\r\nAvailable commands:\r\n"
        "- status     : Show system status\r\n"
        "- reset      : Simulate system reset\r\n"
        "- led on     : Turn ON the built-in LED\r\n"
        "- led off    : Turn OFF the built-in LED\r\n"
        "- help       : Show this command list\r\n\r\n"
      );
    }
  }
}

void DebugTerminal::receiveCommand() {
  while (Serial1.available()) {
    char c = Serial1.read();
    inputBuffer += c;

    if (c == '\n') {
      inputBuffer.trim();
      int startIdx = inputBuffer.indexOf(':');
      if (inputBuffer.startsWith("+IPD") && startIdx != -1) {
        String actualCommand = inputBuffer.substring(startIdx + 1);
        actualCommand.trim();
        if (actualCommand.length() > 0) {
          Serial.print("[RX Command]: ");
          Serial.println(actualCommand);
          debug.processCommand(actualCommand);
        }
      }
      inputBuffer = "";
    }
  }
}

void DebugTerminal::sendPeriodicDebugMessage() {
  static unsigned long lastDebugTime = 0;
  unsigned long now = millis();
  if (now - lastDebugTime >= 5000 && clientConnected) {
    String debugMsg = "[" + String(now) + " ms] Debug: loop running OK\r\n";
    debug.sendMessage(debugMsg);
    lastDebugTime = now;
  }
}

void DebugTerminal::sendAT(String cmd, unsigned long timeout) {
  Serial1.println(cmd);
  unsigned long t = millis();
  while (millis() - t < timeout) {
  while (Serial1.available()) {
    char c = Serial1.read();
    Serial.write(c); // write instead of print to handle raw characters
  }
}
  Serial.println();
}

bool DebugTerminal::sendMessage(String msg) {
  String cmd = "AT+CIPSEND=0," + String(msg.length());
  Serial1.println(cmd);
  unsigned long start = millis();
  while (millis() - start < 2000) {
    if (Serial1.find(">")) {
      Serial1.print(msg);
      break;
    }
  }

  start = millis();
  while (millis() - start < 2000) {
    if (Serial1.find("SEND OK")) return true;
    if (Serial1.find("ERROR") || Serial1.find("CLOSED")) {
      clientConnected = false;
      return false;
    }
  }
  return false;
}

bool DebugTerminal::checkClientConnected() {
  Serial1.println("AT+CIPSEND=0,1");
  unsigned long start = millis();
  while (millis() - start < 1500) {
    if (Serial1.find(">")) {
      Serial1.print("X");
      return true;
    }
    if (Serial1.find("ERROR")) return false;
  }
  return false;
}

void DebugTerminal::processCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "status") {
    String boardName = debug.getBoardName();
    String response = "[" + String(millis()) + " ms] Board: " + boardName + "\r\n";
    debug.sendMessage(response);
  } else if (cmd == "reset") {
    sendMessage("Resetting system...\r\n");
  } else if (cmd == "led on") {
    digitalWrite(LED_BUILTIN, HIGH);
    debug.sendMessage("LED turned ON\r\n");
  } else if (cmd == "led off") {
    digitalWrite(LED_BUILTIN, LOW);
    debug.sendMessage("LED turned OFF\r\n");
  } else if (cmd == "help") {
    debug.sendMessage(
      "Available commands:\r\n"
      "- status     : Show system status\r\n"
      "- reset      : Simulate system reset\r\n"
      "- led on     : Turn ON the built-in LED\r\n"
      "- led off    : Turn OFF the built-in LED\r\n"
      "- help       : Show this command list\r\n"
    );
  } else {
    debug.sendMessage("Unknown command: " + cmd + "\r\n");
  }
}

String DebugTerminal::getBoardName() {
  #if defined(ARDUINO_AVR_UNO)
    return "ATmega328 - UNO";
  #elif defined(ARDUINO_AVR_NANO)
    return "ATmega328 - Nano";
  #elif defined(ARDUINO_AVR_MEGA2560)
    return "ATmega2560 - Mega";
  #else
    return "Unknown AVR Board";
  #endif
}
