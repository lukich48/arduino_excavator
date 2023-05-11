#include <Arduino.h>
#include <Servo.h>
#include <ServoMotorHelper.h>
#include <PS2X_lib.h>

#define PIN_L_SERVO 12
#define PIN_R_SERVO 14
#define PIN_TOWER_X_SERVO 27 // башня
#define PIN_TOWER_Y_SERVO 26 //первое плечо
#define PIN_SEGMENT2_SERVO 25 // второе плечо
#define PIN_GRAB_SERVO 33 // ковш


#define PS2_DAT        19  //MISO
#define PS2_CMD        23  //MOSI
#define PS2_SEL         5  //SS
#define PS2_CLK        18  //SLK

Servo servo_l;
Servo servo_r;
Servo servo_tower_x;
Servo servo_tower_y;
Servo servo_segment2;
Servo servo_grab;

byte inc_angle = 30;

int cur_drive_angle_l = 90;
int cur_drive_angle_r = 90;
int cur_tower_angle_x = 90;
int cur_tower_angle_y = 90;
int cur_segment2_angle = 90;
int cur_grab_angle = 90;

ServoMotorHelper motor_helper_l(72, 51, 111, 132);
ServoMotorHelper motor_helper_r(111, 132, 72, 51);

PS2X ps2x;
byte type = 0;

void connect_ps2x()
{
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

// плавный разгон
void static write_angle_old(Servo& servo, int angle, int& cur_angle)
{
    if (angle > cur_angle)
        for (int i = cur_angle; i < angle; i += 30)
        {
            servo.write(constrain(i, cur_angle, 180));
            delay(200);
        }
    else
        for (int i = cur_angle; i > angle; i -= 30)
        {
            servo.write(constrain(i, 0, cur_angle));
            delay(200);
        }

    cur_angle = angle;
}

void static write_angle(Servo& servo, int angle, int& cur_angle)
{
    if (angle > 90)
        servo.write(constrain(angle, 90, 180));
    else
        servo.write(constrain(angle, 0, 180));

    // Serial.printf("angle: %d cur_angle: %d\n", angle, cur_angle);
    cur_angle = constrain(angle, 0, 180);
}

void do_drive()
{
    byte LY = ps2x.Analog(PSS_LY);
    byte LX = ps2x.Analog(PSS_LX);

    // Serial.printf("LY: %d, LX: %d\n", LY, LX);
    if (LY == 128 && LX == 128)
    {
        servo_l.write(90);
        servo_r.write(90);

        cur_drive_angle_l = 90;
        cur_drive_angle_r = 90;
        return;
    }

    int y = -map(LY, 0, 255, -100, 100);
    int x = map(LX, 0, 255, -100, 100);

    int speed_l = map(y + x, -100, 100, -100, 100);
    int speed_r = map(y - x, -100, 100, -100, 100);

    byte angle_l = motor_helper_l.get_angle(speed_l);
    byte angle_r = motor_helper_r.get_angle(speed_r);

    servo_l.write(angle_l);
    servo_r.write(angle_r);

    byte _angle_l = (angle_l > cur_drive_angle_l) ? _min(cur_drive_angle_l + inc_angle, angle_l) : _max(cur_drive_angle_l - inc_angle, angle_l);
    byte _angle_r = (angle_r > cur_drive_angle_r) ? _min(cur_drive_angle_r + inc_angle, angle_r) : _max(cur_drive_angle_r - inc_angle, angle_r);

    write_angle(servo_l, _angle_l, cur_drive_angle_l);
    write_angle(servo_r, _angle_r, cur_drive_angle_r);

    Serial.printf("_angle_l: %d, _angle_r: %d\n", _angle_l, _angle_r);
}

void do_tower()
{
    byte RY = ps2x.Analog(PSS_RY);
    byte RX = ps2x.Analog(PSS_RX);

    // Serial.printf("LY: %d, LX: %d\n", RY, RX);
    if (RY == 128 && RX == 128)
    {
        servo_tower_y.write(90);
        servo_tower_x.write(90);

        cur_tower_angle_y = 90;
        cur_tower_angle_x = 90;

        return;
    }

    byte y = map(RY, 0, 255, 180, 0); // прямое управление
    byte x = map(RX, 0, 255, 180, 0);

    byte _y = (y > cur_tower_angle_y) ? _min(cur_tower_angle_y + inc_angle, y) : _max(cur_tower_angle_y - inc_angle, y);
    byte _x = (x > cur_tower_angle_x) ? _min(cur_tower_angle_x + inc_angle, x) : _max(cur_tower_angle_x - inc_angle, x);

    write_angle(servo_tower_y, _y, cur_tower_angle_y);
    write_angle(servo_tower_x, _x, cur_tower_angle_x);

    Serial.printf("_y: %d, _x: %d\n", _y, _x);
}

void do_segment2()
{
    if (ps2x.Button(PSB_L1))
    {
        byte angle = constrain(cur_segment2_angle + inc_angle, 0, 180);
        write_angle(servo_segment2, angle, cur_segment2_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else if(ps2x.Button(PSB_R1))
    {
        byte angle = constrain(cur_segment2_angle - inc_angle, 0, 180);
        write_angle(servo_segment2, angle, cur_segment2_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else
    {
        servo_segment2.write(90);
        cur_segment2_angle = 90;
        return;
    }
}

void do_grab()
{
    if (ps2x.Button(PSB_L2))
    {
        byte angle = constrain(cur_grab_angle - inc_angle, 0, 180);
        write_angle(servo_grab, angle, cur_grab_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else if(ps2x.Button(PSB_R2))
    {
        byte angle = constrain(cur_grab_angle + inc_angle, 0, 180);
        write_angle(servo_grab, angle, cur_grab_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else
    {
        servo_grab.write(90);
        cur_grab_angle = 90;
        return;
    }
}

void stop_all_motors()
{
    servo_l.write(90);
    servo_r.write(90);
    servo_tower_y.write(90);
    servo_tower_x.write(90);
    servo_segment2.write(90);
    servo_grab.write(90);

    cur_drive_angle_l = 90;
    cur_drive_angle_r = 90;
    cur_tower_angle_x = 90;
    cur_tower_angle_y = 90;
    cur_segment2_angle = 90;
    cur_grab_angle = 90;
}

void setup(){
    Serial.begin(115200);

    servo_l.attach(PIN_L_SERVO);
    servo_r.attach(PIN_R_SERVO);
    servo_tower_y.attach(PIN_TOWER_Y_SERVO);
    servo_tower_x.attach(PIN_TOWER_X_SERVO);
    servo_segment2.attach(PIN_SEGMENT2_SERVO);
    servo_grab.attach(PIN_GRAB_SERVO);
    //servo_grab.attach(PIN_GRAB_SERVO, -1, 0, 180, 544, 2350); // EMAX ES08MA II

    connect_ps2x();
}

void loop(){
    static uint32_t tmr;
    if (millis() - tmr >= 150)
    {
        tmr = millis();
        if (ps2x.read_gamepad(false, 0))
        {
            do_drive();
            do_tower();
            do_segment2();
            do_grab();
        }
        else
        {
            // Serial.println("false");
            stop_all_motors();
        }
    }
}