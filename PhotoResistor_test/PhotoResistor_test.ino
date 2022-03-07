void setup() {
  Serial.begin(9600);
  pinMode(20, INPUT);
  
}


void loop() {
  int value = analogRead(20);
  Serial.println("Analog value : ");
  Serial.println(value);
  delay(250);
}
