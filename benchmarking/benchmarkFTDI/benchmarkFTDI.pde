#include <stdio.h>

#define BAUD 1500000L
#define MESSAGE_SIZE_STEP 20

int messageSize = 1;
uint8_t messageBuffer[200];
char alpha[] = "abcdefghijklmnopqrstuvwxyz";

void setup() {
    Serial.begin(BAUD);
}

void loop() {
    if(Serial.available()) {
        messageSize = Serial.read() * MESSAGE_SIZE_STEP;
        delay(100);
        for(int i = 0; i < messageSize; i++) {
            messageBuffer[i] = alpha[i % 26];
        }
    }
    Serial.write(messageBuffer, messageSize);
}
