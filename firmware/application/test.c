#include <wiringPi.h>
#include <stdio.h>

int main() {
    wiringPiSetup();
    pinMode(0, OUTPUT); // GPIO 17
    for (int i = 0; i < 10; i++) {
        digitalWrite(0, HIGH); // LED on
        delay(500); // wait 500 ms
        digitalWrite(0, LOW); // LED off
        delay(500); // wait 500 ms
    }
    return 0;
}
