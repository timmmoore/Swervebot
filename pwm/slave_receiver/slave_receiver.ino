// Wire Slave Receiver for supplying 8 pwm output on
//   https://www.seeedstudio.com/Seeeduino-XIAO-Arduino-Microcontroller-SAMD21-Cortex-M0+-p-4426.html?queryID=2927a4497d7d4ec24c09a0fecad8cd9f&objectID=4426&indexName=bazaar_retailer_products
// 
// Based on example by Nicholas Zambetti <http://www.zambetti.com>

#include <Wire.h>
#include <Servo.h>

int spins[] = { 0, 1, 2, 3, 7, 8, 9, 10 };
Servo Servos[8];

void setup()
{
  for(int i = 0; i < 8; i++) {
    Servos[i].attach(spins[i], 860, 2135);
    Servos[i].writeMicroseconds(1500);
  }
  Wire.begin(4);                              // join i2c bus with address #4
  Wire.onReceive(receiveEvent);               // register event
  //Serial.begin(115200);                     // start serial for output
}

void loop()
{
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  int s, x;
  s = Wire.read();                            // receive byte as a character
  while(--howMany > 0) {
    s = s & 7;
    x = Wire.read();                          // receive byte as an integer
    Servos[s++].writeMicroseconds(x*5 + 860); // range is 860-2135, 0 -> 860, 128 -> 1500, 255 -> 2135
    //Serial.println(x);                      // print the integer
  }
}
