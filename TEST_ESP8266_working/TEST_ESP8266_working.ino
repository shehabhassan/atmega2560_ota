void setup() {
  Serial.begin(9600);     // To PC
  Serial1.begin(9600);    // To ESP8266
}

void loop() {
  if (Serial.available()) {
    Serial1.write(Serial.read());  // Send from PC to ESP
  }

  if (Serial1.available()) {
    Serial.write(Serial1.read());  // Send from ESP to PC
  }
}
