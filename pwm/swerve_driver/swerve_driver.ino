// Swerve module driver
//   https://www.adafruit.com/product/4900
// 
// Based on example by Nicholas Zambetti <http://www.zambetti.com>
// pwm servo out on pins 29, 28 - A0, A1
// pwm in on pin 27 - A2 : 220-268Hz, pulse width - .9-4506us, pulse width high
//                        .9 -> 12000, 4506 -> 60,000,000
// quadrature encoder on pins 24, 25 - A3, SDA
// I2c address pins 3, 4; 5, 6

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Servo.h>
#include "encoder.h"      // from xrp firmware

#define STARTADDRESS 16   // IN1/2 high 16, in1 low 17, in2 low 18, in1/2 low 19
#define I2CConnector Wire1

#define PWM_OUTPUT1 29    // drive motor
#define PWM_OUTPUT2 28    // turn motor
#define encoder_a   24    // drive encoder a
#define encoder_b   25    // drive encoder b
#define PWM_INPUT   27    // turn dutycycleencoder
#define ADDR_IN1    3
#define ADDR_IN2    5
#define ADDR_OUT1   4
#define ADDR_OUT2   6
#define UPDATE_POS  21    // BOOTSEL button

Servo Servos[2];        // 0 is drive motor, 1 is turn motor
xrp::Encoder encoder;

uint posoffset;
#define VALUEOFFSET   0
#define IVALUEOFFSET  4
uint startupdate;

uint offsetread()
{
  uint value;
  uint valueinvert;
  EEPROM.get(VALUEOFFSET, value);
  EEPROM.get(IVALUEOFFSET, valueinvert);
  if(value == ~valueinvert) {
    Serial.print("EEPROM set, "); Serial.print(value); Serial.println(" used");
    return value;
  }
  else {
    Serial.println("EEPROM not set, 0 used");
    return 0;
  }
}

void offsetwrite(uint value)
{
  if(posoffset != value){
    posoffset = value;
    Serial.print("EEPROM write = "); Serial.println(value);
    EEPROM.put(VALUEOFFSET, value);
    EEPROM.put(IVALUEOFFSET, ~value);
    EEPROM.commit();
  }
}

void setup()
{
  int addr = STARTADDRESS;
  Serial.begin(115200);                             // start serial for output
  //while (!Serial);
  delay(2000);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));
  
  // pwm output, 1 is drive motor, 2 is turn motor
  Servos[0].attach(PWM_OUTPUT1, 860, 2135);
  Servos[0].writeMicroseconds(1500);
  Servos[1].attach(PWM_OUTPUT2, 860, 2135);
  Servos[1].writeMicroseconds(1500);

  // calculate i2c address
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
  Serial.print("address = ");Serial.println(addr);
  
  // quadrature encoder for drive speed
  if(!encoder.init(encoder_a))
    Serial.println("Encoder failed to init");
  else
    encoder.enable();

  // pulse width input for absolute angle encoder
  pinMode(PWM_INPUT, INPUT_PULLUP);
  Serial.print("Started interrupt for digitalPinToInterrupt(pin) = "); Serial.println(digitalPinToInterrupt(PWM_INPUT));
  attachInterrupt(digitalPinToInterrupt(PWM_INPUT), interruptpwm, CHANGE);

  startupdate = 0;
  pinMode(UPDATE_POS, INPUT_PULLUP);
  EEPROM.begin(512);
  posoffset = offsetread();

  // setup i2c device
  I2CConnector.begin(addr);                           // join i2c bus with address #4
  I2CConnector.onReceive(receiveEvent);               // register event
  I2CConnector.onRequest(requestEvent);
}

volatile uint64_t runningwidth;
uint64_t lastrunningwidth;
void loop()
{
  int next = encoder.update();
  if(next >= 8) {
    Serial.print("Encoder Possible PIO RX Buffer Overrun: ");Serial.println(next);
  }
  cli();
  lastrunningwidth = runningwidth;
  sei();
  // if bootsel button is pressed and released, save current PWM width to EEPROM
  if(digitalRead(UPDATE_POS) == LOW){
    if(startupdate == 0)
      startupdate = millis();
  }
  else if(startupdate != 0){
    if((millis()-startupdate) > 1000){
      // button held for more than 1 second and released
      // and current pwm width is different from offset value
      offsetwrite(lastrunningwidth);
    }
    startupdate = 0;
  }
}

uint64_t MINWIDTH =   80LL;
uint64_t MAXWIDTH =   500000LL;
uint64_t DIVIDERCPU = ((uint64_t)F_CPU)/1000000LL;
volatile uint64_t starttime;
void interruptpwm() {
  int in = digitalRead(PWM_INPUT);
  if (in == HIGH)
    starttime = rp2040.getCycleCount64();   // save rising edge time
  else {
    uint64_t pulse_width = (rp2040.getCycleCount64() - starttime)*100LL; // number of cpu clocks * 100, @133Mhz and 0.9-4506us gives 11,970-59,929,800
    pulse_width = pulse_width/DIVIDERCPU;			 	 // 90-450,600
    // > .8us and < 5000us, ignore if outside this range
    if((pulse_width > MINWIDTH) && (pulse_width < MAXWIDTH))
      runningwidth = (runningwidth*7LL + pulse_width)/8LL;
  }
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
  int x;
  if(howMany-- > 0){
    x = I2CConnector.read();                        // receive byte as an integer
    Servos[0].writeMicroseconds(x*5 + 860);         // range is 860-2135, 0 -> 860, 128 -> 1500, 255 -> 2135
    Serial.print("M0:");Serial.println(x);          // print the integer
  }
  if(howMany-- > 0){
    x = I2CConnector.read();                        // receive byte as an integer
    Servos[1].writeMicroseconds(x*5 + 860);         // range is 860-2135, 0 -> 860, 128 -> 1500, 255 -> 2135
    Serial.print("M1:");Serial.println(x);          // print the integer
  }
}

void host2net(uint v, byte *ptr){
  ptr[0] = v & 0xff;
  ptr[1] = (v>>8) & 0xff;
  ptr[2] = (v>>16) & 0xff;
  ptr[3] = (v>>24) & 0xff;
}

// return encoder and pwm pulse width
void requestEvent()
{
  byte val[20];
  int encodercount = encoder.getCount();
  int encoderperiod = encoder.getPeriod();
  int encoderdivisor = F_CPU / encoder2_CYCLES_PER_COUNT;
  int width = lastrunningwidth;
  if(posoffset != 0) {
    width -= posoffset;
    width += 90;
    if(width < 0)
      width += 450600;
    if((width < 90) || (width > 450600))
      width = 450600;
  }
  //Serial.print("E:");Serial.print(encodercount);Serial.print(", P:");Serial.print(width);Serial.print(", ");Serial.println(posoffset);
  host2net(encodercount, &val[0]);
  host2net(encoderperiod, &val[4]);
  host2net(encoderdivisor, &val[8]);
  host2net(width, &val[12]);
  host2net(posoffset, &val[16]);
  I2CConnector.write(val, 20);
}
