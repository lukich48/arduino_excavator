/*
    Name:       TestPSX.ino
    Descr:      Example for the Playstation 2 controller library for Arduino 
    Version:    1.0.1
    Author:     Christoph Honal
    Date:       2020-04-25
*/
#include <PSX.h>
#include <string>
using std::string;

#define _dataPin   19  // Brown wire
#define _cmdPin    23  // Orange wire
#define _attPin    5  // Yellow wire
#define _clockPin  18  // Blue wire

byte _delay = 10;

#define _buttonPin 5

// Static instantiation of the library
// PSX psx;

// Variables to hold the controller state and error code
PSX::PSXDATA PSXdata;
int PSXerror;

void print(string message)
{
	  Serial.println(message.c_str());
}

void sendCommand(byte command, byte &response)
{
    // Write out a byte command, or read a byte from the controller
    // When reading, the "command" is usually IDLE (0), thus no data gets written
    // The response is always the response from the previous command
    response = 0;
    for (int i=0; i<8; i++)
    {
        // Write command bit
        digitalWrite(_cmdPin, (command & (1 << i))? HIGH:LOW);
        digitalWrite(_clockPin, LOW);
        delayMicroseconds(_delay);
        // Read response bit
        if (digitalRead(_dataPin)) response |= 1 << i;
        digitalWrite(_clockPin, HIGH);
        delayMicroseconds(_delay);
    }

    print(string("sendCommand ") + std::to_string(command) + " " + std::to_string(response));
}

void writeCommand(byte command)
{
    // Write out a byte command, or read a byte from the controller
    // When reading, the "command" is usually IDLE (0), thus no data gets written
    // The response is always the response from the previous command
    digitalWrite(_clockPin, HIGH);
    for (int i=0; i<8; i++)
    {
        // Write command bit
        digitalWrite(_cmdPin, (command & (1 << i))? HIGH:LOW);
        delayMicroseconds(_delay);
    }
}

void readCommand(byte &response)
{
    // Write out a byte command, or read a byte from the controller
    // When reading, the "command" is usually IDLE (0), thus no data gets written
    // The response is always the response from the previous command
    digitalWrite(_clockPin, LOW);
    response = 0;
    for (int i=0; i<8; i++)
    {
        // Read response bit
        if (digitalRead(_dataPin)) response |= 1 << i;
        delayMicroseconds(_delay);
    }
}

void config(byte mode)
{
    print(string("config ") + std::to_string(mode));
    // Perform initial handshake with the controller
    // Enter config
    byte response;
    digitalWrite(_attPin, LOW);
    sendCommand(PSXPROT_HANDSHAKE, response);
    sendCommand(PSXPROT_CONFIG, response);
    sendCommand(PSXPROT_IDLE, response);
    sendCommand(PSXPROT_ENTERCONFIG, response);
    sendCommand(PSXPROT_ZERO, response);    
    digitalWrite(_attPin, HIGH);
    delayMicroseconds(_delay);
    // Set mode
    digitalWrite(_attPin, LOW); 
    sendCommand(PSXPROT_HANDSHAKE, response);
    sendCommand(PSXPROT_CONFIGMODE, response);
    sendCommand(PSXPROT_IDLE, response);
    sendCommand(mode, response);
    sendCommand(PSXPROT_MODELOCK, response);
    for(int i=0; i<4; i++) sendCommand(PSXPROT_ZERO, response);  
    digitalWrite(_attPin, HIGH);    
    delayMicroseconds(_delay);
    // Disable vibration motors
    digitalWrite(_attPin, LOW); 
    sendCommand(PSXPROT_HANDSHAKE, response);
    sendCommand(PSXPROT_CONFIGMOTOR, response);
    sendCommand(PSXPROT_IDLE, response);
    sendCommand(PSXPROT_ZERO, response);
    sendCommand(PSXPROT_MOTORMAP, response);
    for(int i=0; i<4; i++) sendCommand(PSXPROT_NONZERO, response);   
    digitalWrite(_attPin, HIGH);    
    delayMicroseconds(_delay);
    // Finish config
    digitalWrite(_attPin, LOW);
    sendCommand(PSXPROT_HANDSHAKE, response);
    sendCommand(PSXPROT_CONFIG, response);
    sendCommand(PSXPROT_IDLE, response);
    sendCommand(PSXPROT_EXITCONFIG, response);
    for(int i=0; i<5; i++) sendCommand(PSXPROT_EXITCFGCNT, response);        
    digitalWrite(_attPin, HIGH);
}

int read(PSX::PSXDATA &psxdata)
{
    print(string("read"));
    // Send data request
    digitalWrite(_attPin, LOW);
    byte response;
    sendCommand(PSXPROT_HANDSHAKE, response);
    sendCommand(PSXPROT_GETDATA, response);
    sendCommand(PSXPROT_IDLE, response);
    // Check response
    if(response == PSXPROT_STARTDATA)
    {
        print(string("Decode response"));
        // Decode response
        byte data[6];
        for(int i=0; i<6; i++) sendCommand(PSXPROT_IDLE, data[i]);
        digitalWrite(_attPin, HIGH);
        psxdata.buttons = ~(data[1] + (data[0] << 8));
        psxdata.JoyRightX = data[2];
        psxdata.JoyRightY = data[3];
        psxdata.JoyLeftX = data[4];
        psxdata.JoyLeftY = data[5];
        return PSXERROR_SUCCESS;
    }
    else
    {
        digitalWrite(_attPin, HIGH);
        return PSXERROR_NODATA;
    }
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup done");
  //Setup the PSX library
  // psx.setupPins(_dataPin, _cmdPin, _attPin, _clockPin, 10);
  // psx.config(PSXMODE_ANALOG);
  // Setup serial communication

  // Setup pins and pin states
  pinMode(_dataPin, INPUT_PULLUP);
  // digitalWrite(_dataPin, HIGH);
  pinMode(_cmdPin, OUTPUT);
  digitalWrite(_cmdPin, HIGH);
  pinMode(_attPin, OUTPUT);
  // digitalWrite(_attPin, HIGH);
  pinMode(_clockPin, OUTPUT);
  digitalWrite(_clockPin, HIGH);

  config(PSXMODE_ANALOG);

  pinMode(_buttonPin, INPUT);
}

void loop() {
  // Read controller state
//   if(digitalRead(_buttonPin) == 0)
//   {
    delay(1000);
  PSXerror = read(PSXdata);

  // Check if the read was successful
  if(PSXerror == PSXERROR_SUCCESS) {
      // Print the joystick states
      Serial.print("JoyLeft: X: ");
      Serial.print(PSXdata.JoyLeftX);
      Serial.print(", Y: ");
      Serial.print(PSXdata.JoyLeftY);
      Serial.print(", JoyRight: X: ");
      Serial.print(PSXdata.JoyRightX);
      Serial.print(", Y: ");
      Serial.print(PSXdata.JoyRightY);

      //Print the button states
      Serial.print(", Buttons: ");
      if(PSXdata.buttons & PSXBTN_LEFT) {
        Serial.print("Left, ");
      }
      if(PSXdata.buttons & PSXBTN_DOWN) {
        Serial.print("Down, ");
      }
      if(PSXdata.buttons & PSXBTN_RIGHT) {
        Serial.print("Right, ");
      }
      if(PSXdata.buttons & PSXBTN_UP) {
        Serial.print("Up, ");
      }
      if(PSXdata.buttons & PSXBTN_START) {
        Serial.print("Start, ");
      }
      if(PSXdata.buttons & PSXBTN_SELECT) {
        Serial.print("Select, ");
      }
      if(PSXdata.buttons & PSXBTN_SQUARE) {
        Serial.print("Square, ");
      }
      if(PSXdata.buttons & PSXBTN_CROSS) {
        Serial.print("Cross, ");
      }
      if(PSXdata.buttons & PSXBTN_CIRCLE) {
        Serial.print("Circle, ");
      }
      if(PSXdata.buttons & PSXBTN_TRIANGLE) {
        Serial.print("Triangle, ");
      }
      if(PSXdata.buttons & PSXBTN_R1) {
        Serial.print("R1, ");
      }
      if(PSXdata.buttons & PSXBTN_L1) {
        Serial.print("L1, ");
      }
      if(PSXdata.buttons & PSXBTN_R2) {
        Serial.print("R2, ");
      }
      if(PSXdata.buttons & PSXBTN_L2) {
        Serial.print("L2, ");
      }

      Serial.println("");
  } else {
    Serial.println("No success reading data");
  }
//   }
}