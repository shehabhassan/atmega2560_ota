#ifndef SIM7600_ALG_H
#define SIM7600_ALG_H

#include <avr/wdt.h>
#include <Wire.h>
#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>

#define SERIAL_RX_BUFFER_SIZE 512  // Increase buffer for incoming data
#define SERIAL_TX_BUFFER_SIZE 28
#include <HardwareSerial.h>

#define MODEM_RST 5
#define MODEM_PWRKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 17
#define MODEM_RX 16


#define SerialAT  Serial1

 // Read 512 bytes at a time (Adjustable) 60000/1000 = 60 * 3 s = 180s /60 = 3 min
// Define HTTP read chunk size
#define HTTP_READ_CHUNK   440  // read chunk of 10 line for each 43 bytes with adding 10 byte for space and ok unnessery data .
#define MIN_NO_OF_LENGTH  12
#define RETRY_LIMIT       3
extern char filename[];
extern char filename_PATH[];

// Function declarations (prototypes)
void SIM7600_check_signal_quality();
void checkNetworkStatus();
void setup_HTTP();
void GSM7600_download_file_from_Server(const char* URL, String filename);
void GSM7600_download_file_from_Server_deep(const char* URL, String filename);
void SIM7600_download_file_from_Server(const char* URL, String filename);
void sim7600_download_file_from_Server(const char* URL, String filename);
void GSM7600_download_file_from_Server_url(const char* URL);

void sim7600_download_file_from_github(const char* URL, String filename);
/***********************************************************************************/
void download_file_from_server(const char* url,String filename);
/***********************************************************************************/
void sim7600_download_file_from_Server_test(const char* URL, String filename);
void sim7600_download_file_from_Server_test_2(const char* URL, String filename);
/***********************************************************************************/
void saveToSD(String filename, String data);

// debug macro ( to print in the serial) u can remove the code will run normal 
#define DBG(x)           Serial.println(x)      
#define DBG_no_lin(y)    Serial.print(y)    

#endif
