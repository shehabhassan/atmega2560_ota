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
                                                                         0 – none (default) 2 – CTS hardware flow
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
    Serial.println("AT Response: " + ATResponse);
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

/****************************************************************************************************/ 
  String  Path_Command , Content_Command ;
  String receive_response = ""; // this the output of receive the response 200 ,400 , 404 , 
  String json_response = "";
  String responseData = "";
/****************************************************************************************************/
// helper function for debug

void SIM7600_check_signal_quality()
{
  Serial.println("SIM7600 Signal Quality.......");
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
                if (line.length() < 10) { // the uncomplate line 
                    file.print(line);  // Append without adding a newline
                }
                // Append valid line to the file
                if (line.length() > 10) {
                    file.println(line);  // Append without adding a newline
                }
            }else{
              if(line[0]>='0' && line[0]<='9'){
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
    file.close();
    sendATCommand("AT+HTTPTERM", 500);
    Serial.println("File saved successfully"); 
}
/**
function test 2 
 int lineStart = 0;
        // // while (true) {
        // //     int lineEnd = buffer.indexOf('\n', lineStart);
        // //     if (lineEnd == -1) break;  // No more complete lines

        // //     String line = buffer.substring(lineStart, lineEnd);
        // //     line.trim();  // Remove extra spaces

        // //     if (line.startsWith(":")) {
        // //         // Check for the end marker
        // //         if (line == ":00000001FF") {
        // //             if (endMarkerFound) continue;  // Skip duplicates
        // //             endMarkerFound = true;  // Mark the end of the file
        // //         }

        // //         // Append valid line to the file
        // //         if (line.length() > 0) {
        // //             file.print(line);  // Append without adding a newline
        // //         }
        // //     }
        // //     lineStart = lineEnd + 1;
        // // }

        // buffer = buffer.substring(lineStart);  // Keep remaining incomplete data

*/
void sim7600_download_file_from_Server_test(const char* URL, String filename) {
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
//   /*
  
  uint32_t bytesRead = 0;
    String buffer = "";
    bool endMarkerFound = false;

    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
        String httpData = sendATCommand_string(readCommand, 3000);
        
        // Remove unnecessary parts
        httpData.replace("OK", "");
        httpData.replace("+HTTPREAD", "");
        httpData.replace("\r", ""); 

        buffer += httpData;  // Append the data to buffer

        int lineStart = 0;
        while (true) {
            int lineEnd = buffer.indexOf('\n', lineStart);
            if (lineEnd == -1) break;  // No more complete lines

            String line = buffer.substring(lineStart, lineEnd);
            line.trim();  // Remove extra spaces
            
            if (line.startsWith(":")) {
                // Clean unnecessary data
                if (line.startsWith(":440") || line.startsWith("+HTTPREAD: DATA,440") || 
                    line.startsWith("OK")   || line == " "   || line.startsWith(": 0")|| 
                    line.startsWith(": DATA,440") || line.startsWith(": DATA,92")) { 
                  //  OK line.startsWith(":100") || line.startsWith(":92") || line == ":0"
                    line = "";  // Remove these lines
                }
                
                // Check for the end marker
                if (line == ":00000001FF") {
                    if (endMarkerFound) continue;  // Skip duplicates
                    endMarkerFound = true;  // Mark the end of the file
                }

                // Write valid lines to the file
                if (line.length() > 0) {
                    file.println(line);
                }
            }
            lineStart = lineEnd + 1;
        }

        buffer = buffer.substring(lineStart);  // Keep the remaining incomplete data

        bytesRead += bytesToRead;
        Serial.print("Processed: ");
        Serial.print(bytesRead);
        Serial.print("/");
        Serial.println(dataSize);
    }

    file.close();
    sendATCommand("AT+HTTPTERM", 500);
    Serial.println("File saved successfully"); 
  
//   */
//     uint32_t bytesRead = 0;
//     String buffer = ""; // Persistent buffer for partial lines

//     while (bytesRead < dataSize) {
//         uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);
//         String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
//         String httpData = sendATCommand_string(readCommand, 3000);

//         // Process and clean received data
//         httpData.replace("OK", "");         // Remove all 'OK' responses
//         httpData.replace("+HTTPREAD", "");   // Remove '+HTTPREAD' texts
//         httpData.replace("DATA,", "");       // Remove 'DATA,' text
//         httpData.replace("\r", "");          // Remove carriage returns
//         httpData.replace(": 0", "");         // Remove '0' or extra zeros
//         httpData.replace(" ", "");           // Remove any spaces

//         String cleanedData = "";
//         int index = 0;
        
//         //  int index = 0;
//         while (index < httpData.length()) {
//             int lineStart = httpData.indexOf(':', index);
//             if (lineStart == -1) break;
            
//             String line = httpData.substring(lineStart);
//             line.trim();
//             // Check if the line starts with valid Intel Hex format
//             if (line.startsWith(":10") || line.startsWith(":0E") || line.startsWith(":00")) {
//                 cleanedData += line + "\n";
//             }
            
//             // Move to the next part of the string
//             index = lineStart + 1;
//         }
// /*    
//             while (index < httpData.length()) {
//             int lineEnd = httpData.indexOf('\n', index);
//             if (lineEnd == -1) lineEnd = httpData.length();
            
//             String line = httpData.substring(index, lineEnd);
//             line.trim();  // Remove extra newlines and spaces
            
//             index = lineEnd + 1;
            
//             // Process valid HEX lines only
//             if (line.startsWith(":") && (line.length() <= 43)) {
//                 cleanedData += line + "\n";  // Store clean line to write in SD
//             }
//         }
// */
//         // Write cleaned data to SD
//         if (cleanedData.length() > 0) {
//             file.print(cleanedData);
//             bytesRead += bytesToRead;
//             Serial.print("Processed: ");
//             Serial.print(bytesRead);
//             Serial.print("/");
//             Serial.println(dataSize);
//         }
//     }

//     file.close();
//     sendATCommand("AT+HTTPTERM", 500);
//     Serial.println("File saved successfully");
}



/********************************************************************************************************/
/********************************************************************************************************/
void sim7600_download_file_from_Server(const char* URL, String filename) {
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

    uint32_t bytesRead = 0;
    String buffer = ""; // Persistent buffer for partial lines

    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
        String httpData = sendATCommand_string(readCommand, 3000);
        // Process data chunk
        httpData.replace("OK", ""); // Remove all OK responses
        httpData.replace("+HTTPREAD", ""); // Remove carriage returns
        httpData.replace("\r", ""); // Remove carriage returns
        String cleanedData = "";
        int index = 0;

        // while (index < httpData.length()) {
        //     int lineEnd = httpData.indexOf('\n', index);
        //     if (lineEnd == -1) lineEnd = httpData.length();
            
        //     String line = httpData.substring(index, lineEnd);
        //     line.trim(); // Remove \r, \n, spaces
        //     index = lineEnd + 1;
             
        //     // Skip non-HEX data
        //     if (line.startsWith("+HTTPREAD") || line == "OK") {
        //         continue;
        //     }
        //     Serial.print("line: ");
        //     Serial.println(line);
        //     // HEX line processing
        //     if (line.startsWith(":")) {
        //         if (buffer.length() > 0) {
        //             buffer += line;
        //             if (buffer.length() >= 43) { // Valid HEX line length
        //                 cleanedData += buffer + "\n";
        //                 buffer = "";
        //             }
        //         } else if (line.length() < 43) {
        //             buffer = line; // Store partial line
        //         } else {
        //             cleanedData += line + "\n"; // Complete line
        //         }
        //     } else if (buffer.length() > 0) {
        //         buffer += line; // Continue partial line
        //         if (buffer.length() <= 43) {
        //             cleanedData += buffer + "\n";
        //             buffer = "";
        //         }
        //     }
        // }

            // Serial.print("cleanedData: ");
            // Serial.println(cleanedData);
        // Write cleaned data to SD
        if (cleanedData.length() > 0) {
            file.print(cleanedData);
            bytesRead += bytesToRead;
            Serial.print("Processed: ");
            Serial.print(bytesRead);
            Serial.print("/");
            Serial.println(dataSize);
        }
    // }
     
    // Handle remaining buffer data
    // if (buffer.length() > 0) {
    //     if (buffer.length() >= 43) { // chang the condition from >= to <= in the same value 
    //         file.print(buffer + "\n");
    //     } else {
    //         Serial.println("Warning: Incomplete final line detected");
    //     }
    }

    // file.close();
    sendATCommand("AT+HTTPTERM", 500);
    Serial.println("File saved successfully");
}



/********************************************************************************************************/
/********************************************************************************************************/
/********************************************************************************************************/
void GSM7600_download_file_from_Server_url(const char* URL)
{
    Serial.println("Initializing HTTP...");
  
    sendATCommand("AT+HTTPINIT", 1000);

    Serial.println("Setting URL...");
    String Path_Command = "AT+HTTPPARA=\"URL\",\"" + String(URL) + "\"";
    sendATCommand(Path_Command, 1000);

    Serial.println("Starting HTTP GET request...");
    receive_response = sendATCommand("AT+HTTPACTION=0", 5000);
    delay(3000);  // Give time for response

    String actionResponse = "";
    int index = receive_response.indexOf("+HTTPACTION:");

    if (index != -1) {
    actionResponse = receive_response.substring(index);
    Serial.println("Extracted HTTPACTION response: " + actionResponse);
    
    if (sscanf(actionResponse.c_str(), "+HTTPACTION: %*d,%d,%d", &statusCode, &dataSize) == 2) {
        Serial.print("HTTP Status Code: ");
        Serial.println(statusCode);
        Serial.print("Data Size: ");
        Serial.println(dataSize);
    } else {
        Serial.println("Failed to parse response!");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    } else {
    Serial.println("HTTPACTION response not found!");
    sendATCommand("AT+HTTPTERM", 1000);
    return;
}
    
    if (statusCode != 200) {
        Serial.println("HTTP request failed. Terminating HTTP session.");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }
     // third change 
     Serial.print(" Total File Size: "); Serial.println(dataSize);
     //  Read data in chunks and store it properly
    uint32_t bytesRead = 0;
    File file = SD.open(filename, FILE_WRITE);
    // new line of code 
    String buffer = "";  // Store cleaned data before writing to SD

    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(512, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
        Serial.print(" Reading chunk at offset: "); Serial.println(bytesRead);
        String httpData = sendATCommand(readCommand, 3000);
        
         //  **Filter out unnecessary lines**
        String cleanedData = "";
        int index = 0;
        while (index < httpData.length()) {
            String line = httpData.substring(index, httpData.indexOf('\n', index));
            index = httpData.indexOf('\n', index) + 1;

            // **Skip unwanted lines**
            if (line.startsWith("+HTTPREAD") || line == "OK") {
                continue;
            }

            cleanedData += line + "\n";  // Keep valid lines
        }
            // // **Ensure line has at least 43 characters by merging broken lines**
            // buffer += line;
            // if (buffer.length() >= 43) {
            //     cleanedData += buffer + "\n";  // Save only complete lines
            //     buffer = "";  // Reset buffer for next line
            // }
        // }

        // **Verify data integrity before writing**
        if (cleanedData.length() > 0) {
            file.print(cleanedData);  // Append chunk to file
            bytesRead += bytesToRead;
            Serial.print(" Bytes received so far: "); Serial.println(bytesRead);
        } else {
            Serial.println(" Missing data! Retrying...");
            delay(500); // Wait before retrying
        }
    }
    file.close();
    Serial.println("✅ File downloaded and saved successfully.");
    sendATCommand("AT+HTTPTERM", 500);
}

/********************************************************************************************************/

void GSM7600_download_file_from_Server_deep(const char* URL, String filename) {
    Serial.println("Initializing HTTP...");
    sendATCommand("AT+HTTPINIT", 1000);

    Serial.println("Setting URL...");
    String Path_Command = "AT+HTTPPARA=\"URL\",\"" + String(URL) + "\"";
    sendATCommand(Path_Command, 1000);

    Serial.println("Starting HTTP GET request...");
    String receive_response = sendATCommand("AT+HTTPACTION=0", 5000);
    delay(3000);

    int statusCode = 0;
    uint32_t dataSize = 0;
    int index = receive_response.indexOf("+HTTPACTION:");

    if (index != -1) {
        String actionResponse = receive_response.substring(index);
        if (sscanf(actionResponse.c_str(), "+HTTPACTION: %*d,%d,%d", &statusCode, &dataSize) == 2) {
            Serial.print("HTTP Status Code: "); Serial.println(statusCode);
            Serial.print("Data Size: "); Serial.println(dataSize);
        } else {
            Serial.println("Failed to parse response!");
            sendATCommand("AT+HTTPTERM", 500);
            return;
        }
    } else {
        Serial.println("HTTPACTION response not found!");
        sendATCommand("AT+HTTPTERM", 1000);
        return;
    }

    if (statusCode != 200) {
        Serial.println("HTTP request failed. Terminating HTTP session.");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file!");
        return;
    }

    uint32_t bytesRead = 0;
    String buffer = "";

    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(512, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        String httpData = sendATCommand(readCommand, 3000);

        String cleanedData = "";
        int index = 0;
        while (index < httpData.length()) {
            int lineEnd = httpData.indexOf('\n', index);
            if (lineEnd == -1) lineEnd = httpData.length();
            String line = httpData.substring(index, lineEnd);
            index = lineEnd + 1;

            // Skip unwanted lines
            if (line.startsWith("+HTTPREAD") || line == "OK" || line.length() == 0) {
                continue;
            }

            // Merge lines
            if (line.startsWith(":")) {
                if (line.length() < 43) {
                    buffer = line; // Partial line
                } else {
                    cleanedData += line + "\n"; // Complete line
                }
            } else if (buffer.length() > 0) {
                buffer += line;
                if (buffer.length() >= 43) {
                    cleanedData += buffer + "\n";
                    buffer = "";
                }
            }
        }

        if (cleanedData.length() > 0) {
            file.print(cleanedData);
            bytesRead += bytesToRead;
        }
    }

    file.close(); // <-- Properly placed
    Serial.println("File saved successfully.");
    sendATCommand("AT+HTTPTERM", 500);
}

/********************************************************************************************************/

void SIM7600_download_file_from_Server(const char* URL, String filename) {
    Serial.println("Initializing HTTP...");
    sendATCommand("AT+HTTPINIT", 1000);

    Serial.println("Setting URL...");
    String Path_Command = "AT+HTTPPARA=\"URL\",\"" + String(URL) + "\"";
    sendATCommand(Path_Command, 1000);

    Serial.println("Starting HTTP GET request...");
    String receive_response = sendATCommand("AT+HTTPACTION=0", 5000);
    delay(3000);

    int statusCode = 0;
    uint32_t dataSize = 0;
    int index = receive_response.indexOf("+HTTPACTION:");

    if (index != -1) {
        String actionResponse = receive_response.substring(index);
        if (sscanf(actionResponse.c_str(), "+HTTPACTION: %*d,%d,%d", &statusCode, &dataSize) == 2) {
            Serial.print("HTTP Status Code: "); Serial.println(statusCode);
            Serial.print("Data Size: "); Serial.println(dataSize);
        } else {
            Serial.println("Failed to parse response!");
            sendATCommand("AT+HTTPTERM", 500);
            return;
        }
    } else {
        Serial.println("HTTPACTION response not found!");
        sendATCommand("AT+HTTPTERM", 1000);
        return;
    }

    if (statusCode != 200) {
        Serial.println("HTTP request failed. Terminating HTTP session.");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }
    File file = SD.open(filename, FILE_WRITE);
    String buffer = ""; // Buffer for partial lines
    uint32_t bytesRead = 0;
    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(512, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        String httpData = sendATCommand(readCommand, 3000);
        String cleanedData = "";
        int index = 0;

        while (index < httpData.length()) {
            int lineEnd = httpData.indexOf('\n', index);
            if (lineEnd == -1) lineEnd = httpData.length();
            String line = httpData.substring(index, lineEnd);
            line.trim(); // Remove spaces/CR/LF

            // Skip unwanted lines
            if (line.startsWith("+HTTPREAD") || line == "OK" ) {    //|| line.length() == 0
                index = lineEnd + 1;
                continue;
            }

            // Merge partial lines
            if (line.startsWith(":")) {
                if (buffer.length() > 0) {
                    buffer += line;
                    if (buffer.length() >= 43) {
                        cleanedData += buffer + "\n";
                        buffer = "";
                    }
                } else if (line.length() < 43) {
                    buffer = line; // Partial line
                } else {
                    cleanedData += line + "\n"; // Complete line
                }
            } else if (buffer.length() > 0) {
                buffer += line;
                if (buffer.length() >= 43) {
                    cleanedData += buffer + "\n";
                    buffer = "";
                }
            }

            index = lineEnd + 1;
        }

        // Write cleaned data to SD
        if (cleanedData.length() > 0) {
            file.print(cleanedData);
            bytesRead += bytesToRead;
        }
    }

    // Write any remaining buffer
    if (buffer.length() > 0) {
        file.print(buffer);
    }

    file.close();
    Serial.println("File saved successfully.");
    sendATCommand("AT+HTTPTERM", 500);
}

/********************************************************************************************************/

void GSM7600_download_file_from_Server(const char* URL, String filename) {
    Serial.println("Initializing HTTP...");
  
    sendATCommand("AT+HTTPINIT", 1000);

    Serial.println("Setting URL...");
    String Path_Command = "AT+HTTPPARA=\"URL\",\"" + String(URL) + "\"";
    sendATCommand(Path_Command, 1000);

    Serial.println("Starting HTTP GET request...");
    receive_response = sendATCommand("AT+HTTPACTION=0", 5000);
    delay(3000);  // Give time for response

    String actionResponse = "";
    int index = receive_response.indexOf("+HTTPACTION:");

    if (index != -1) {
    actionResponse = receive_response.substring(index);
    Serial.println("Extracted HTTPACTION response: " + actionResponse);
    
    if (sscanf(actionResponse.c_str(), "+HTTPACTION: %*d,%d,%d", &statusCode, &dataSize) == 2) {
        Serial.print("HTTP Status Code: ");
        Serial.println(statusCode);
        Serial.print("Data Size: ");
        Serial.println(dataSize);
    } else {
        Serial.println("Failed to parse response!");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    } else {
    Serial.println("HTTPACTION response not found!");
    sendATCommand("AT+HTTPTERM", 1000);
    return;
}
    
    if (statusCode != 200) {
        Serial.println("HTTP request failed. Terminating HTTP session.");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }
     // third change 
     Serial.print(" Total File Size: "); Serial.println(dataSize);
    
    //  Read data in chunks and store it properly
    uint32_t bytesRead = 0;
    File file = SD.open(filename, FILE_WRITE);
    
    String buffer = "";  // Stores partial lines to merge correctly
    while (bytesRead < dataSize) {
        uint32_t bytesToRead = min(512, dataSize - bytesRead);
        String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);
        
        Serial.print(" Reading chunk at offset: "); Serial.println(bytesRead);
        String httpData = sendATCommand(readCommand, 3000);

        //  **Process and clean data**
        String cleanedData = "";
        int index = 0;
         while (index < httpData.length()) {
            int lineEnd = httpData.indexOf('\n', index);
            if (lineEnd == -1) lineEnd = httpData.length();
            String line = httpData.substring(index, lineEnd);
            index = lineEnd + 1;

            // Skip unwanted lines
            if (line.startsWith("+HTTPREAD") || line == "OK" || line.length() == 0) {
                continue;
            }

            // Merge lines
            if (line.startsWith(":")) {
                if (line.length() < 43) {
                    buffer = line; // Partial line
                } else {
                    cleanedData += line + "\n"; // Complete line
                }
            } else if (buffer.length() > 0) {
                buffer += line;
                if (buffer.length() >= 43) {
                    cleanedData += buffer + "\n";
                    buffer = "";
                }
            }
        }

        if (cleanedData.length() > 0) {
            file.print(cleanedData);
            bytesRead += bytesToRead;
        }
    }

    file.close();
    Serial.println(" Cleaned File downloaded and saved successfully.");
    sendATCommand("AT+HTTPTERM", 500);
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

/********************************************************************************************************/


/**********************************************************************************************************************/
void sim7600_download_file_from_Server_fixed_version(const char* URL, String filename) {
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
    int statusCode = -1;
    int dataSize = 0;
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

    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("SD error");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    String buffer = "";
    uint32_t bytesRead = 0;
    bool eofDetected = false;

    while (bytesRead < dataSize && !eofDetected) {
        uint32_t chunkSize = min(512, dataSize - bytesRead);
        String httpData = sendATCommand("AT+HTTPREAD=" + String(bytesRead) + "," + String(chunkSize), 3000);
        bytesRead += chunkSize;

        // Remove protocol artifacts
        httpData.replace("OK", "");
        httpData.replace("+HTTPREAD: DATA", "");
        httpData.replace("\r", "");
        httpData.replace(" ", "");

        // Character-level processing
        for (int i = 0; i < httpData.length(); i++) {
            char c = httpData[i];
            
            if (c == ':') {
                // Finalize previous line if needed
                if (buffer.length() > 0) {
                    if (buffer.startsWith(":00000001FF")) {
                        file.println(buffer);
                        eofDetected = true;
                        break;
                    }
                    if (buffer.length() == 43) {
                        file.println(buffer);
                    } else {
                        Serial.print("Discarded invalid line: ");
                        Serial.println(buffer);
                    }
                }
                buffer = String(c);
            } else {
                buffer += c;
            }

            // Check for complete line
            if (buffer.length() == 43) {
                file.println(buffer);
                buffer = "";
            }
            
            // Check for early EOF
            if (buffer.startsWith(":00000001FF")) {
                file.println(buffer);
                eofDetected = true;
                break;
            }
        }
    }

    // Handle remaining data
    if (buffer.length() > 0) {
        if (buffer.startsWith(":00000001FF")) {
            file.println(buffer);
        } else if (buffer.length() == 43) {
            file.println(buffer);
        } else {
            Serial.print("Discarded final fragment: ");
            Serial.println(buffer);
        }
    }

    // Add missing EOF if not found
    if (!eofDetected) {
        file.println(":00000001FF");
        Serial.println("Added missing EOF marker");
    }

    file.close();
    sendATCommand("AT+HTTPTERM", 500);
    Serial.println("File saved successfully");
}
/******************************************************************************/
void sim7600_download_file_from_github(const char* URL, String filename) {
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
    int statusCode = -1;
    int dataSize = 0;
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
    /**************************************/ 
    uint32_t bytesRead = 0;

    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("SD file open failed!");
        sendATCommand("AT+HTTPTERM", 500);
        return;
    }

    String buffer = "";
    bool eofFound = false;
    uint32_t bytesProcessed = 0;

    while (bytesRead < dataSize && !eofFound) {
        uint32_t chunkSize = min(440, dataSize - bytesRead);
        String httpData = sendATCommand("AT+HTTPREAD=" + String(bytesRead) + "," + String(chunkSize), 3000);
        bytesRead += chunkSize;

        // Clean response
        httpData.replace("OK", "");
        httpData.replace("\r", "");

        int index = 0;
        while (index < httpData.length() && !eofFound) {
            int lineEnd = httpData.indexOf('\n', index);
            if (lineEnd == -1) lineEnd = httpData.length();
            
            String chunk = httpData.substring(index, lineEnd);
            chunk.trim();
            index = lineEnd + 1;

            // Detect final line pattern
            if (chunk.startsWith(":00000001FF")) {
                file.println(chunk);
                eofFound = true;
                break;
            }

            // Standard line processing
            if (chunk.startsWith(":")) {
                if (buffer.length() > 0) {
                    // Handle split lines
                    buffer += chunk;
                    if (buffer.length() >= 43) {
                        file.println(buffer.substring(0, 43));
                        bytesProcessed += 43;
                        buffer = buffer.substring(43);
                    }
                } else {
                    if (chunk.length() == 43) {
                        file.println(chunk);
                        bytesProcessed += 43;
                    } else {
                        buffer = chunk;
                    }
                }
            } else if (buffer.length() > 0) {
                buffer += chunk;
                if (buffer.length() >= 43) {
                    file.println(buffer.substring(0, 43));
                    bytesProcessed += 43;
                    buffer = buffer.substring(43);
                }
            }
        }
    }

    // Handle remaining buffer after EOF
    if (buffer.length() > 0 && !eofFound) {
        if (buffer.startsWith(":00000001FF")) {
            file.println(buffer);
            eofFound = true;
        } else if (buffer.length() == 43) {
            file.println(buffer);
            bytesProcessed += 43;
        }
    }

    if (!eofFound) {
        Serial.println("Warning: EOF marker not found!");
        file.println(":00000001FF"); // Add missing EOF
    }

    file.close();
    sendATCommand("AT+HTTPTERM", 500);
    Serial.println("File saved successfully");
}
/************************************************************************************/
/*************************************************************************************/

 // 2 change  downloadAndSaveHexFile(filename,dataSize);
    // **Reading and saving data in chunks**
    // Serial.println("Reading HTTP data in chunks...");
    // uint32_t bytesRead = 0;
    // while (bytesRead < dataSize) {
    //     uint32_t bytesToRead = min(HTTP_READ_CHUNK, dataSize - bytesRead);  // Read in chunks
    //     Serial.print("bytesToRead: ");
    //     Serial.println(bytesToRead);    
    //     Serial.print("bytesToRead: ");
    //     Serial.println(dataSize);     
    //     String readCommand = "AT+HTTPREAD=" + String(bytesRead) + "," + String(bytesToRead);    
    //     Serial.println("Reading chunk: " + readCommand);
    //     String httpData = sendATCommand(readCommand, 3000);  // Get chunk of data
    //     saveToSD(filename, httpData);  // Save chunk to SD card  
    //     bytesRead += bytesToRead;
    //     Serial.print("Bytes received so far: ");
    //     Serial.println(bytesRead);
    // }
    // Serial.println("Download complete! File saved to SD card.");
    // Serial.print("Reading HTTP data (size: ");
    // Serial.print(dataSize);
    // Serial.println(")...");
    // String httpData = sendATCommand("AT+HTTPREAD=0," + String(dataSize), 5000);
    // Serial.println("Received Data:");
    // Serial.println(httpData);
    // sendATCommand("AT+HTTPTERM", 500);
