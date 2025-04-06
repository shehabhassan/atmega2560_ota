#ifndef SIM7600_ALG_H
#define SIM7600_ALG_H

#define SERIAL_RX_BUFFER_SIZE 512  // Increase buffer for incoming data
#define SERIAL_TX_BUFFER_SIZE 28

#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <math.h>
#include <HardwareSerial.h>

#define FILE_TEXT               "https://shehabhassan.github.io/FOTA_TEST/Test_Report.txt"
#define FILE_BIN_TEXT           "https://shehabhassan.github.io/FOTA_TEST/firm.txt"
#define FILE_HEX_LARGE          "https://shehabhassan.github.io/FOTA_TEST/firmware_2560_1_0_0.hex"
#define FILE_HEX_2              "https://shehabhassan.github.io/FOTA_TEST/firmware_2560_2_.hex"
#define FILE_HEX_SMALL_1        "https://shehabhassan.github.io/FOTA_TEST/2560_1_0_1.hex"
#define FILE_HEX_SMALL_2        "https://shehabhassan.github.io/FOTA_TEST/2560_1_0_0.hex"
#define FILE_HEX_7600           "https://shehabhassan.github.io/FOTA_TEST/2560_sim7600.hex"

/***************************************************************************************************************/
#define FILE_NAME_1    "hhh.hex"
#define FILE_NAME_2    "111.hex"

#define SerialAT  Serial1
// avr-objcopy -I ihex 2560_sim7600.hex -O binary 2560_sim7600.bin

// Read 512 bytes at a time (Adjustable) 60000/1000 = 60 * 3 s = 180s /60 = 3 min
// Define HTTP read chunk size
#define HTTP_READ_CHUNK   440  // read chunk of 10 line for each 43 bytes with adding 10 byte for space and ok unnessery data .
#define MIN_NO_OF_LENGTH  12
#define RETRY_LIMIT       3
// extern veriable shares to aother files 
extern char filename[];
extern char filename_PATH[];

// Function declarations (prototypes)
void SIM7600_check_signal_quality();
void checkNetworkStatus();
void setup_HTTP();
/***********************************************************************************/
void sim7600_download_file_from_Server_test_2(const char* URL, String filename);
/***********************************************************************************/
void convertHexToBin(String hexFilename, String binFilename);
// debug macro ( to print in the serial) u can remove the code will run normal 
#define DBG(x)           Serial.println(x)      
#define DBG_no_lin(y)    Serial.print(y)    

#define DEBUG_FETAL_(...) { Serial.print("Debug statue: "); Serial.println(__VA_ARGS__); delay(1000);}

#endif
