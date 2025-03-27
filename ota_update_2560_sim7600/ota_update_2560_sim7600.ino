#include "sim7600_alg.hpp"
/** PinOut of differrent modules ***/
// const char URL[] =  "https://shehabhassan.github.io/FOTA_TEST/Test_Report.txt";
// const char URL[] =  "https://shehabhassan.github.io/FOTA_TEST/firm.txt";
const char URL2[] =  "https://shehabhassan.github.io/FOTA_TEST/firmware_2560_1_0_0.hex";
// const char URL[] =  "https://shehabhassan.github.io/FOTA_TEST/firmware_2560_2_.hex";
// const char URL[] =  "https://shehabhassan.github.io/FOTA_TEST/2560_1_0_1.hex";
const char URL[] =  "https://shehabhassan.github.io/FOTA_TEST/2560_1_0_0.hex";


/***************** objects and classes ***********************/
File                       myfile; //create a sd card object
/**************************variables*************************/
// String filename = "firmwarefirm.txt";
char filename[] = "566.hex"; // fixed code 
char filename2[] = "111.hex"; // fixed code 
char filename_PATH[] = "/fhx.hex";
void setup() {
 
  Serial.begin(115200);    // Serial monitor
  while(!Serial);
  SerialAT.begin(9600);  // SIM7600 serial communication
  while(!SerialAT);

 if(!InitializeSDcard())
  {return;}
 
//  if(!createFile(filename)){Serial.println("Error to create file .");}

 SIM7600_check_signal_quality();

 checkNetworkStatus();

 setup_HTTP();

//  GSM7600_download_file_from_Server_url(URL);
//  GSM7600_download_file_from_Server(URL,filename);
//  GSM7600_download_file_from_Server_deep(URL,filename);
//  SIM7600_download_file_from_Server(URL,filename);
    // sim7600_download_file_from_Server(URL,filename); // working perfect 

    // sim7600_download_file_from_Server_test(URL,filename); // test line code
    sim7600_download_file_from_Server_test_2(URL2,filename); 
    Serial.println("************************************************************************************************");
    // download_file_from_server(URL2,"");
    // sim7600_download_file_from_Server(URL2,filename2); // working perfect 
    // sim7600_download_file_from_github(URL,filename);
    // sim7600_download_file_from_Server_fixed_version(URL,filename);
}

void loop() 
{
 Serial.println("SIM7600: Done "); 
while(1);
}
