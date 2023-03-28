#include <Arduino.h>
#include <Servo.h>
#include <ServoMotorHelper.h>
#include <PS2X_lib.h>

static const byte servoPinL = 12;
static const byte servoPinR = 14;

#define PS2_DAT        19  //MISO
#define PS2_CMD        23  //MOSI
#define PS2_SEL         5  //SS
#define PS2_CLK        18  //SLK

Servo servo_l;
Servo servo_r;

ServoMotorHelper motor_helper_l(72, 51, 111, 132);
ServoMotorHelper motor_helper_r(111, 132, 72, 51);

PS2X ps2x;
byte type = 0;

void test_helpers(int speed, int d){

    byte angle_l = motor_helper_l.get_angle(speed);
    byte angle_r = motor_helper_r.get_angle(speed);

    Serial.print("speed: ");
    Serial.print(speed);
    Serial.print(", ");
    Serial.print(angle_l);
    Serial.print(", ");
    Serial.println(angle_r);

    servo_l.write(angle_l);
    servo_r.write(angle_r);

    delay(d);

    servo_l.write(90);
    servo_r.write(90);

    delay(3000);
}


void setup(){
    Serial.begin(115200);
    servo_l.attach(servoPinL);
    servo_r.attach(servoPinR);

    // test_helpers(1, 10000);
    // test_helpers(-1, 10000);
    // test_helpers(100, 5000);
    // test_helpers(-100, 5000);

    int error = -1;
    int tryNum = 1;
    while (error != 0)
    {
        delay(1000); // 1 second wait
        // setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error //Setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for errors
        error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
        Serial.print("#try config ");
        Serial.println(tryNum);
        tryNum++;
    }
    ps2x.Analog(1);
    type = ps2x.readType();
    switch (type)
    {
    case 0:
        Serial.println(" Unknown Controller type found ");
        break;
    case 1:
        Serial.println(" DualShock Controller found ");
        break;
    case 2:
        Serial.println(" GuitarHero Controller found ");
        break;
    case 3:
        Serial.println(" Wireless Sony DualShock Controller found ");
        break;
    }
}

void loop(){
    delay(50);
    if (ps2x.read_gamepad(false, 0))
    {
        byte LY = ps2x.Analog(PSS_LY);
        byte LX = ps2x.Analog(PSS_LX);

        // Serial.printf("LY: %d, LX: %d\n", LY, LX);
        
        int y = -map(LY, 0, 255, -100, 100);
        int x = map(LX, 0, 255, -100, 100);

        Serial.printf("Y: %d, X: %d\n", y, x);

        int speed_l = map(y + x, -100, 100, -100, 100);
        int speed_r = map(y - x, -100, 100, -100, 100);

        byte angle_l = motor_helper_l.get_angle(speed_l);
        byte angle_r = motor_helper_r.get_angle(speed_r);

        // Serial.printf("angle_r: %d, angle_r: %d\n", angle_l, angle_r);

        servo_l.write(angle_l);
        servo_r.write(angle_r);
    }
    else
    {
        Serial.println("false");
        servo_l.write(90);
        servo_r.write(90);
    }


}