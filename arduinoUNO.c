void setup() {
  Serial.begin(9600);  // Set the baud rate to match the NodeMCU
}

void loop() {
  Serial.println(":PID0021223*");  // Send data to NodeMCU
  delay(2000);  // Wait for a second
  Serial.println(":PID0041223*");  // Send data to NodeMCU
  delay(1000);  // Wait for a second
  Serial.println(":PID0071223*");  // Send data to NodeMCU
  delay(2000);  // Wait for a second
}
