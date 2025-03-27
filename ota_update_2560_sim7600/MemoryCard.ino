bool InitializeSDcard()
{
  Serial.println("Opening SD Card . . .");
  delay(500);
  if(SD.begin())
  {
    Serial.println("SD Card ready to use");
    return 1;
  }else{
    Serial.println("Failed to open SD Card");
    return 0;
  }
}

bool createFile(char filename[])
{
  myfile = SD.open(filename, FILE_WRITE);

  if (myfile)
  {
    Serial.println("File created successfully.");
     Serial.println(filename);
    return 1;
  } else
  {
    Serial.println("Error while creating file.");
    return 0;
  }
}


int OpenFile(char Filename[])
{
  myfile = SD.open(Filename, FILE_READ);
  delay(500);
  if(myfile)
  {
    Serial.println("File is open");
    return 1;
  }else{
    Serial.println("Error opening file");
    return 0;
  }
}


void closeFile()
{
  if (myfile)
  {
    myfile.close();
    Serial.println("File closed");
  }
}



int WriteToFile(char text[])
{
  if (myfile)
  {
    myfile.println(text);
    //Serial.println("the text is written successfully");
    return 1;
  }
  else
  {
    Serial.println("Couldn't write to file");
    return 0;
  }
}

/*Befor Reading from the file either using readfile() function or readline() function you have to use first the command
myfile = SD.open("BBBB.txt", FILE_READ);
then open, read and close
*/
void readFile(void)
{
    while (myfile.available())
    {
        char c = myfile.read(); // read a character from the file
        Serial.print(c);
    } 
}

// Function to write HTTP data to SD card
void saveToSD(const char *filename, String data) {
    Serial.print("Processing data for file: ");
    Serial.println(filename);

    // Remove unwanted lines
    String cleanedData = "";
    int startIndex = 0;

    while (startIndex != -1) {
        // Find the next line that starts with ':'
        startIndex = data.indexOf(':', startIndex);
        if (startIndex != -1) {
            int endIndex = data.indexOf('\n', startIndex);
            if (endIndex == -1) endIndex = data.length();
            cleanedData += data.substring(startIndex, endIndex) + "\n";
            startIndex = endIndex;
        }
    }

    File myfile = SD.open(filename, FILE_WRITE);
    if (!myfile) {
        Serial.println("Error opening file for writing.");
        return;
    }

    myfile.print(cleanedData); // Write cleaned data
    myfile.close();
    Serial.println("HEX file saved to SD card.");
}
