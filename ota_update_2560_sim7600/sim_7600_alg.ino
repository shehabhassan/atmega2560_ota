#include "WString.h"
#include "sim7600_alg.hpp"

// SIM7600 configuration
const char apn1[] = "internet.vodafone.net" ;  // APN for GPRS   TM "TM";   // mms.net.sa ;//STC: Jawalnet.com.sa
const char gprsUser[] = "";         // GPRS User, usually empty
const char gprsPass[] = "";         // GPRS Password, usually empty


int statusCode = -1;
uint32_t dataSize = 0;

/*
 * AT+IPREX Set local baud rate permently chatgpt 
 * AT+IFC   Enable RTS/CTS flow control  Set local data flow control    0 – none (default) 2 – RTS hardware flow control chatgpt
 *                                                                      0 – none (default) 2 – CTS hardware flow
 * AT+CNMI=2,1,0,0,0  Ensure proper message reception   AT+CNMI <mode>,<mt>,<bm>,<ds>,<bfr> 2,1,0,0  chatgpt
 * Use a Bigger Serial Buffer in ATmega2560 in hardwareserial Edit the  #define SERIAL_RX_BUFFER_SIZE 512
 */

 /*,
  * ATE Enable command echo   0 – Echo mode off    1-   Echo mode on
  * 
  *
 */
static String sendATCommand(String command, unsigned long timeout) {
    SerialAT.println(command);
    unsigned long start = millis();
    String ATResponse = "";
    while (millis() - start < timeout) {
        while (SerialAT.available()) {
            char c = SerialAT.read();
            ATResponse += c;
        }
    }
    DBG("AT Response: " + ATResponse);
    return ATResponse;
}
uint32_t no_of_line = 0;
static String sendATCommand_string(String command, unsigned long timeout) {
    SerialAT.println(command);
    unsigned long start = millis();
    String ATResponse = "";
    String combine = "";
    while (millis() - start < timeout) {
        String line = SerialAT.readStringUntil('\n'); // Read one line at a time
        if(line.startsWith(":")){
          ATResponse += line + "\n";
          no_of_line+=1;
        }
        else{
          if(line[0]>='0' && line[0]<='9'){
             ATResponse += line + "\n";
          }
        }
          // ATResponse += line + "\n";
    }
    Serial.println("AT Response: \n" + ATResponse);
    Serial.print("no of lines: ");
    Serial.println(no_of_line);
    return ATResponse;
}
 
static bool sendATCommand_bool(String command, unsigned long timeout) {
    SerialAT.println(command);
    unsigned long start = millis();
    String ATResponse = "";

    while (millis() - start < timeout) {
        while (SerialAT.available()) {
            char c = SerialAT.read();
            ATResponse += c;
        }
    }

    Serial.println("AT Response: " + ATResponse);

    // Check if response contains "OK" (success)
    if (ATResponse.indexOf("OK") != -1) {
        return true;  // Success
    } else {
        Serial.println("Error: AT command failed!");
        return false; // Failure
    }
}

void SIM7600_check_signal_quality()
{
  DBG("SIM7600 Signal Quality.......");
   // Send the AT command to check signal quality
  sendATCommand("AT+CSQ",1000); 
}
void timedelay(int timeout)
{
  long int time = millis();
  while ( (time + timeout) > millis()){}
}

/********************************************************************************************************/
void checkNetworkStatus()
{
  Serial.println("Check Network Status & flow control ......");
  sendATCommand("AT",1000);    // 
  // sendATCommand("ATE0",1000);  // 1
  /*
    AT+CREG?: This AT command asks the SIM module to return the current network registration status. The response will tell you whether the module is connected to a network, trying to connect, or not registered.
  */
  sendATCommand("AT+IFC=2,2", 1000);  // Enable RTS/CTS flow control
  sendATCommand("AT+CNMI=2,1,0,0,0", 1000);  // Ensure proper message reception
} 

void setup_HTTP()
{
    char define_PDP_str[50];

    Serial.println("Initializing HTTP setup...");

    // Check APN before setting up
    if (strlen(apn1) == 0) {
        Serial.println("Error: APN is empty!");
        return;
    }
     
    // 1. Check network registration
    if (!sendATCommand_bool("AT+CREG?", 2000)) {
        Serial.println("Error: Network registration failed!");
        return;
    }

    // 2. Attach to GPRS
    if (!sendATCommand_bool("AT+CGATT=1", 2000)) {
        Serial.println("Error: Failed to attach to GPRS!");
        return;
    }

    // 3. Activate PDP context
    if (!sendATCommand_bool("AT+CGACT=1,1", 2000)) {
        Serial.println("Error: Failed to activate PDP context!");
        return;
    }

    // 4. Set APN
    sprintf(define_PDP_str, "AT+CGDCONT=1,\"IP\",\"%s\"", apn1);
    if (!sendATCommand_bool(define_PDP_str, 2000)) {
        Serial.println("Error: Failed to set APN!");
        return;
    }

    Serial.println("HTTP setup completed successfully!");
}

void sim7600_download_file_from_Server_test_2(const char* URL, String filename) {
    Serial.println("Initializing HTTP...");
    sendATCommand("AT+HTTPINIT", 1000);

    // Set target URL
    Serial.println("Setting URL...");
    String Path_Command = "AT+HTTPPARA=\"URL\",\"" + String(URL) + "\"";
    sendATCommand(Path_Command, 1000);

    // Trigger HTTP GET request
    Serial.println("Starting HTTP GET...");
    String receive_response = sendATCommand("AT+HTTPACTION=0", 5000);
    delay(3000);

    // Parse HTTP response
    int statusCode = 0;
    uint32_t dataSize = 0;
    int index = receive_response.indexOf("+HTTPACTION:");
    if (index == -1) {
        Serial.println("HTTPACTION response missing!");
        sendATCommand("AT+HTTPTERM", 1000);
        return;
    }

    String actionResponse = receive_response.substring(index);
    if (sscanf(actionResponse.c_str(), "+HTTPACTION: %*d,%d,%d", &statusCode, &dataSize) != 2) {
        Serial.println("Failed to parse HTTP response!");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    if (statusCode != 200) {
        Serial.print("HTTP Error: ");
        Serial.println(statusCode);
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    // Open SD file
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("SD file open failed!");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    Serial.print("SD file open :");
    Serial.println(filename);

    uint32_t bytesRead = 0;
    String buffer = "";
    bool endMarkerFound = false;

    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
        String httpData = sendATCommand_string(readCommand, 3000);
        
        // Remove unnecessary parts
        httpData.replace("OK", "");
        httpData.replace("\r", "");
        
        buffer += httpData;  // Append the data to buffer
         int lineStart = 0;
        while (true) {
            int lineEnd = buffer.indexOf('\n', lineStart);
            if (lineEnd == -1) break;  // No more complete lines

            String line = buffer.substring(lineStart, lineEnd);
            line.trim();  // Remove extra spaces

            if (line.startsWith(":")) {
                // Check for the end marker
                if (line == ":00000001FF") {
                    if (endMarkerFound) continue;  // Skip duplicates
                    endMarkerFound = true;  // Mark the end of the file
                }
                  // Append valid line to the file
                if (line.length() < 10) { // the uncomplate line 30 is min of byte in each line
                    file.print(line);  // Append without adding a newline
                }
                // Append valid line to the file
                if (line.length() > 10) {
                    file.println(line);  // Append without adding a newline
                }
            }else{
              // handle the line without starting : 
              if((line[0]>='0' && line[0]<='9') || (line[0]>='A' && line[0]<='Z')){
                 line += "\n"; // add the new line to the currunt line 
              // Append valid line to the file
                if (line.length() > 0) {
                    file.print(line);  // Append without adding a newline
                }
          }
        }
            lineStart = lineEnd + 1;
        }

        buffer = buffer.substring(lineStart);  // Keep remaining incomplete data
        bytesRead += bytesToRead;
        Serial.print("Processed: ");
        Serial.print(bytesRead);
        Serial.print("/");
        Serial.println(dataSize);
    }
    if(bytesRead==dataSize){ Serial.println("File Recived successfully"); } 
    file.close();
    sendATCommand("AT+HTTPTERM", 500);
    Serial.println("File saved successfully"); 
}

void saveCleanHexToSD(String filename, String data) {
    Serial.print("Processing data for file: ");
    Serial.println(filename);

    String cleanedData = "";  // Store only valid HEX lines
    int startIndex = 0;

    while (startIndex != -1) {
        // Find the next line that starts with ':'
        startIndex = data.indexOf(':', startIndex);
        if (startIndex != -1) {
            int endIndex = data.indexOf('\n', startIndex);
            if (endIndex == -1) endIndex = data.length();
            
            // Extract the valid HEX line
            String hexLine = data.substring(startIndex, endIndex);

            // Check if it's a valid HEX format (starts with `:` and has hex digits)
            if (hexLine.length() > MIN_NO_OF_LENGTH) {
                cleanedData += hexLine + "\n";
            }
            
            startIndex = endIndex;
        }
    }

    // Write cleaned data to SD card
    File myfile = SD.open(filename, FILE_WRITE);
    if (!myfile) {
        Serial.println("Error opening file for writing.");
        return;
    }

    myfile.print(cleanedData);
    myfile.close();
    Serial.println("Clean HEX file saved to SD card.");
}

// **Reading and saving data in chunks**
void downloadAndSaveHexFile(String filename, uint32_t dataSize) {
    Serial.println("Reading HTTP data in chunks...");

    uint32_t bytesRead = 0;
  
    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);  // Read in chunks
        Serial.print("bytesToRead: ");Serial.println(bytesToRead);
        
        Serial.print("Total Data Size: ");Serial.println(dataSize);
        
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
        // Serial.println("Reading chunk: " + readCommand);
        //String httpData = sendATCommand(readCommand, 3000);  // Get chunk of data
        Serial.print("🔹 Reading chunk at offset: "); Serial.println(bytesRead);
        String httpData = "";
        int retryCount = 0;

        while (retryCount < RETRY_LIMIT) {
            httpData = sendATCommand_string(readCommand, 3000);
            if (httpData.length() > 0) break;
            retryCount++;
            Serial.println("🔄 Retrying read...");
        }

        if (httpData.length() == 0) {
            Serial.println("❌ Failed to receive data chunk!");
            break;
        }

        // **Filter and save only HEX lines**
        saveCleanHexToSD(filename, httpData);
          
        bytesRead += bytesToRead;
        Serial.print("Bytes received so far: ");   Serial.println(bytesRead);
    }

    Serial.println("Download complete! File saved to SD card.");
}
/*********************************************************************************************/

void convertHexToBin(String hexFilename, String binFilename) {
    File hexFile = SD.open(hexFilename, FILE_READ);
    if (!hexFile) {
        Serial.println("Failed to open HEX file!");
        return;
    }

    File binFile = SD.open(binFilename, FILE_WRITE);
    if (!binFile) {
        Serial.println("Failed to open BIN file for writing!");
        hexFile.close();
        return;
    }

    while (hexFile.available()) {
        String line = hexFile.readStringUntil('\n');
        line.trim();  // Remove extra spaces or line breaks

        if (line.startsWith(":")) { // Skip Intel HEX format markers
            line = line.substring(9, line.length() - 2); // Remove header and checksum parts
        }

        int lineLength = line.length();
        for (int i = 0; i < lineLength; i += 2) {
            String hexByte = line.substring(i, i + 2);
            uint8_t byteValue = (uint8_t)strtol(hexByte.c_str(), NULL, 16);
            binFile.write(byteValue);  // Save as binary data
        }
    }

    hexFile.close();
    binFile.close();
    Serial.println("HEX file successfully converted to BIN file.");
}

/*
static String sendATCommand_string(String command, unsigned long timeout) {
    SerialAT.println(command);
    unsigned long start = millis();
    String ATResponse = "";

    while (millis() - start < timeout) {
        while (SerialAT.available()) {
            char c = (char)SerialAT.read();  // Read raw byte
            ATResponse += c;  // Append to the response as-is (binary data supported)
        }
    }
    Serial.println("AT Response Received.");
    return ATResponse;  // Return the raw binary data as a String
}

uint32_t bytesRead = 0;
String buffer = "";

while (bytesRead < dataSize) {
    uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);
    String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
    
    String httpData = sendATCommand_string(readCommand, 3000);
    
    buffer += httpData;  // Append the received data

    // Write to file directly as raw binary data
    for (int i = 0; i < buffer.length(); i++) {
        file.write((uint8_t)buffer[i]);  // Save byte by byte to the file
    }

    buffer = "";  // Clear buffer after writing to the file
    bytesRead += bytesToRead;

    Serial.print("Processed: ");
    Serial.print(bytesRead);
    Serial.print("/");
    Serial.println(dataSize);
}
*/