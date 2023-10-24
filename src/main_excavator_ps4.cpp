#include <Arduino.h>
#include <Servo.h>
#include <ServoMotorHelper.h>

#include <PS4Controller.h>
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_err.h"
#include "BLEDevice.h"

#define PIN_L_SERVO 32
#define PIN_R_SERVO 33
#define PIN_TOWER_SERVO 27 // башня
#define PIN_ARM_SERVO 26 //первое плечо
#define PIN_FOREARM_SERVO 25 // второе плечо
#define PIN_GRAB_SERVO 14 // ковш

uint8_t new_mac[] = {0x7c, 0x9e, 0xbd, 0xfa, 0x0b, 0xac};

// если поворачивает в другую сторону
bool invert_x = true;

Servo servo_l;
Servo servo_r;
Servo servo_tower;
Servo servo_arm;
Servo servo_forearm;
Servo servo_grab;

byte inc_angle = 30;

int cur_drive_angle_l = 90;
int cur_drive_angle_r = 90;
int cur_tower_angle = 90;
int cur_arm_angle = 90;
int cur_forearm_angle = 90;
int cur_grab_angle = 90;

ServoMotorHelper motor_helper_l(72, 0, 111, 180);
ServoMotorHelper motor_helper_r(111, 180, 72, 0);

void change_mac()
{
  if (BLEDevice::getInitialized()) BLEDevice::deinit(true);
  esp_base_mac_addr_set(new_mac);
  BLEDevice::init("ESP32");
}

// original code https://github.com/un0038998/PS4Controller_ESP32/blob/main/Remove_Paired_Devices/Remove_Paired_Devices.ino
void removePairedDevices(){
  uint8_t pairedDeviceBtAddr[20][6];  
  int count = esp_bt_gap_get_bond_device_num();
  esp_bt_gap_get_bond_device_list(&count, pairedDeviceBtAddr);
  for(int i = 0; i < count; i++) 
  {
    uint8_t* address = pairedDeviceBtAddr[i];
    Serial.printf("drop device - %02x:%02x:%02x:%02x:%02x:%02x\n", address[0],address[1],address[2],address[3],address[4],address[5]);
    esp_bt_gap_remove_bond_device(pairedDeviceBtAddr[i]);
  }
}

void connect_ps4_controller()
{
    PS4.begin();

    removePairedDevices(); // to avoid re-pairing issues

    const uint8_t* address = esp_bt_dev_get_address();
    char str[100];
    sprintf(str, "ESP32's Bluetooth MAC address is - %02x:%02x:%02x:%02x:%02x:%02x\n", address[0],address[1],address[2],address[3],address[4],address[5]);
    Serial.println(str);
}

void static write_angle(Servo& servo, int angle, int& cur_angle)
{
    servo.write(constrain(angle, 0, 180));

    // Serial.printf("angle: %d cur_angle: %d\n", angle, cur_angle);
    cur_angle = constrain(angle, 0, 180);
}

void do_drive()
{
    byte LY = PS4.LStickY();
    byte LX = PS4.LStickX();

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

    Serial.printf("y: %d, x: %d\n", y, x);

    int speed_l = map(invert_x ? (y - x) : (y + x), -100, 100, -100, 100);
    int speed_r = map(invert_x ? (y + x) : (y - x), -100, 100, -100, 100);

    byte angle_l = motor_helper_l.get_angle(speed_l);
    byte angle_r = motor_helper_r.get_angle(speed_r);

    byte _angle_l = (angle_l > cur_drive_angle_l) ? _min(cur_drive_angle_l + inc_angle, angle_l) : _max(cur_drive_angle_l - inc_angle, angle_l);
    byte _angle_r = (angle_r > cur_drive_angle_r) ? _min(cur_drive_angle_r + inc_angle, angle_r) : _max(cur_drive_angle_r - inc_angle, angle_r);

    write_angle(servo_l, _angle_l, cur_drive_angle_l);
    write_angle(servo_r, _angle_r, cur_drive_angle_r);

    // Serial.printf("_angle_l: %d, _angle_r: %d\n", _angle_l, _angle_r);
}

void do_arm()
{
    byte RY = PS4.RStickY();
    byte RX = PS4.RStickX();

    // Serial.printf("LY: %d, LX: %d\n", RY, RX);
    if (RY == 128)
    {
        write_angle(servo_arm, 90, cur_arm_angle);
    }
    else
    {
        byte y = map(RY, 0, 255, 180, 0);
        byte _y = (y > cur_arm_angle) ? _min(cur_arm_angle + inc_angle, y) : _max(cur_arm_angle - inc_angle, y);
        write_angle(servo_arm, _y, cur_arm_angle);
    }

    if (RX == 128)
    {
        write_angle(servo_forearm, 90, cur_forearm_angle);
    }
    else
    {
        byte x = map(RX, 0, 255, 180, 0);
        byte _x = (x > cur_forearm_angle) ? _min(cur_forearm_angle + inc_angle, x) : _max(cur_forearm_angle - inc_angle, x);
        write_angle(servo_forearm, _x, cur_forearm_angle);
    }
}

void do_tower()
{
    if (PS4.Left() || PS4.UpLeft() || PS4.DownLeft())
    {
        byte angle = constrain(cur_tower_angle + inc_angle, 0, 180);
        write_angle(servo_tower, angle, cur_tower_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else if (PS4.Right() || PS4.UpRight() || PS4.DownRight())
    {
        byte angle = constrain(cur_tower_angle - inc_angle, 0, 180);
        write_angle(servo_tower, angle, cur_tower_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else
    {
        write_angle(servo_tower, 90, cur_tower_angle);
        return;
    }
}

void do_grab()
{
    if (PS4.L2() || PS4.Up() || PS4.UpLeft() || PS4.UpRight()) //todo: use byte value
    {
        byte angle = constrain(cur_grab_angle - inc_angle, 0, 180);
        write_angle(servo_grab, angle, cur_grab_angle);
        Serial.printf("angle: %d\n", angle);
    }
    else if(PS4.R2() || PS4.Down() || PS4.DownLeft() || PS4.DownRight())
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
    servo_arm.write(90);
    servo_tower.write(90);
    servo_forearm.write(90);
    servo_grab.write(90);

    cur_drive_angle_l = 90;
    cur_drive_angle_r = 90;
    cur_tower_angle = 90;
    cur_arm_angle = 90;
    cur_forearm_angle = 90;
    cur_grab_angle = 90;
}

void setup(){
    Serial.begin(115200);

    change_mac();

    servo_l.attach(PIN_L_SERVO);
    servo_r.attach(PIN_R_SERVO);
    servo_arm.attach(PIN_ARM_SERVO);
    servo_tower.attach(PIN_TOWER_SERVO);
    servo_forearm.attach(PIN_FOREARM_SERVO);
    servo_grab.attach(PIN_GRAB_SERVO);

    connect_ps4_controller();
}

void loop(){
    static uint32_t tmr;
    if (millis() - tmr >= 150)
    {
        tmr = millis();
        if (PS4.isConnected())
        {
            do_drive();
            do_tower();
            do_arm();
            do_grab();
        }
        else
        {
            stop_all_motors();
        }
    }
}