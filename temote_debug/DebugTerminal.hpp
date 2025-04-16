// ===================== DebugTerminal.hpp =====================
#ifndef DEBUG_TERMINAL_HPP
#define DEBUG_TERMINAL_HPP
#include "secret_data.hpp"
#include <Arduino.h>

class DebugTerminal {
  public:
    void begin(long serialSpeed);
    void setupWiFi(const char* ssid, const char* pass,const char* p_address);
    void checkTelnetConnection();
    void receiveCommand();
    void sendPeriodicDebugMessage();

  private:
    bool clientConnected = false;
    String inputBuffer = "";

    void sendAT(String cmd, unsigned long timeout = 1000);
    bool sendMessage(String msg);
    bool checkClientConnected();
    void processCommand(String cmd);
    String getBoardName();
};

#endif // DEBUG_TERMINAL_HPP
