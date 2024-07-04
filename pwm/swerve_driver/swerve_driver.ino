// Swerve module driver
//   https://www.adafruit.com/product/4900
// 
// Based on example by Nicholas Zambetti <http://www.zambetti.com>
// pwm servo out on pins 29, 28 - A0, A1
// pwm in on pin 27 - A2
// quadrature encoder on pins 24, 25 - A3, SDA
// I2c address pins 3,4; 5, 6

#include <Wire.h>
#include <Servo.h>
#include "PicoEncoder.h"

#define ADDRESS 4   // IN1/2 high 4, in1 low 5, in2 low 6, in1/2 low 7
#define I2CConnector Wire1

#define PWM_OUTPUT1 29
#define PWM_OUTPUT2 28
#define encoder_a   24
#define encoder_b   25
#define PWM_INPUT   27
#define ADDR_IN1    3
#define ADDR_IN2    5
#define ADDR_OUT1   4
#define ADDR_OUT2   6

PicoEncoder encoder;
Servo Servos[2];

void setup()
{
  int addr = ADDRESS;
  Servos[0].attach(PWM_OUTPUT1, 860, 2135);
  Servos[0].writeMicroseconds(1500);
  Servos[1].attach(PWM_OUTPUT2, 860, 2135);
  Servos[1].writeMicroseconds(1500);
  pinMode(ADDR_IN1, INPUT_PULLUP);
  pinMode(ADDR_IN2, INPUT_PULLUP);
  pinMode(ADDR_OUT1, OUTPUT);
  pinMode(ADDR_OUT2, OUTPUT);
  digitalWrite(ADDR_OUT1, LOW);
  digitalWrite(ADDR_OUT2, LOW);
  if (digitalRead(ADDR_IN1) == LOW)
    addr += 1;
  if (digitalRead(ADDR_IN2) == LOW)
    addr += 2;
  int checkPin = digitalPinToInterrupt(PWM_INPUT);

  if (checkPin == -1) {
    Serial.println("Not a valid interrupt pin!");
  } else {
    Serial.println("Valid interrupt pin.");
  }
  encoder.begin(encoder_a);  
  I2CConnector.begin(addr);                           // join i2c bus with address #4
  I2CConnector.onReceive(receiveEvent);               // register event
  I2CConnector.onRequest(requestEvent);
  //Serial.begin(115200);                             // start serial for output
}

void loop()
{
  encoder.update();
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  int s, x;
  s = I2CConnector.read();                            // receive byte as a character
  while(--howMany > 0) {
    s = s & 1;
    x = I2CConnector.read();                          // receive byte as an integer
    Servos[s++].writeMicroseconds(x*5 + 860);         // range is 860-2135, 0 -> 860, 128 -> 1500, 255 -> 2135
    //Serial.println(x);                              // print the integer
  }
}

// return encoder and pwm pulse width
void requestEvent()
{
  byte val[6];
  uint encodercount = encoder.step;
  val[0] = encodercount & 0xff;
  val[1] = (encodercount>>8) & 0xff;
  val[2] = (encodercount>>16) & 0xff;
  val[3] = (encodercount>>24) & 0xff;
  val[4] = 0;
  val[5] = 0;
  I2CConnector.write(val, 6);
}
