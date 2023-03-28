#include <Arduino.h>
#include <Servo.h>

byte _servoLPin = 27;
byte _servoRPin = 26;
Servo servoL;
Servo servoR;

void setup(){
    Serial.begin(115200);

    servoL.attach(_servoLPin);
    servoL.write(180);
    
    // servoR.attach(_servoRPin, Servo::CHANNEL_NOT_ATTACHED, 0, 180, 500, 2360);
    servoR.attach(_servoRPin);
    // servoR.write(0);
}

void loop(){

    for(int i=90; i>=0; i-=10){
        servoR.write(i);
        Serial.println(i);
        delay(1000);
    }

    for(int i=90; i<=180; i+=10){
        servoR.write(i);
        Serial.println(i);
        delay(1000);
    }
}