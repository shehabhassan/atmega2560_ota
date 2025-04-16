#include "DebugTerminal.hpp"

DebugTerminal Debug;

void setup() {
  Debug.begin(115200);
  Debug.setupWiFi(SECRET_SSID, SECRET_PASS,IP_A);
}

void loop() {
  Debug.checkTelnetConnection();
  Debug.receiveCommand();
  Debug.sendPeriodicDebugMessage();
}
