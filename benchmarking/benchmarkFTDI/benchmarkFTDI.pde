#include <stdio.h>

int messageSize = 1;
uint8_t messageBuffer[20000];

void setup() {
    Serial.begin(2000000L);
    randomSeed(analogRead(0));
}

void loop() {
    if(Serial.available()) {
        messageSize = Serial.read() * 20;
        delay(100);
        for(int i = 0; i < messageSize; i++) {
            messageBuffer[i] = random(127);
        }
    }
    Serial.write(messageBuffer, messageSize);
}
