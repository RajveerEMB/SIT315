int ledPin = 10;
int motionPin = 2; // Motion sensor digital output

void setup() {
    pinMode(motionPin, INPUT);
    pinMode(ledPin, OUTPUT);
    Serial.begin(9600);
}

void loop() {
    int motionDetected = digitalRead(motionPin);
    
    if (motionDetected == HIGH) {
        digitalWrite(ledPin, HIGH); // Turn LED on
        Serial.println("Motion detected! LED ON");
    } else {
        digitalWrite(ledPin, LOW); // Turn LED off
        Serial.println("No motion detected!. LED OFF");
    }
    
    delay(1500);
}
