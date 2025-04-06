#include "sim7600_alg.hpp"
/** PinOut of differrent modules ***/
const char URL[] =    FILE_HEX_SMALL_2 ;
const char URL2[] =  FILE_HEX_7600;
/***************** objects and classes ***********************/
File                       myfile; //create a sd card object
/**************************variables*************************/
// String filename = "firmwarefirm.txt";
char filename[] = FILE_NAME_1; // fixed code 
char filename2[] = FILE_NAME_2; // fixed code 
char filename_PATH[] = "/fhx.hex";
void setup() {
 
  Serial.begin(115200);    // Serial monitor
  while(!Serial);
  SerialAT.begin(9600);  // SIM7600 serial communication
  while(!SerialAT);

 if(!InitializeSDcard()){return;}
 
 SIM7600_check_signal_quality();
 checkNetworkStatus();
 setup_HTTP();
 sim7600_download_file_from_Server_test_2(URL2,filename); 
 DBG("************************************************************************************************");
 convertHexToBin(filename, "out.bin"); 
}

void loop() 
{
 DBG("SIM7600: Done "); 
while(1);
}
