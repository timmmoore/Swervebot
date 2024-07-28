// SPDX-FileCopyrightText: 2023 Carter Nelson for Adafruit Industries
//
// SPDX-License-Identifier: MIT
// --------------------------------------
// i2c_scanner
//
// Modified from https://playground.arduino.cc/Main/I2cScanner/
// --------------------------------------

#include <Wire.h>

// Set I2C bus to use: Wire, Wire1, etc.
#define WIRE Wire1

void setup() {
  WIRE.setSCL(15);
  WIRE.setSDA(14);
  WIRE.begin();

  Serial.begin(115200);
  while (!Serial)
     delay(10);
  Serial.println("\nI2C Scanner");
}

uint net2host(byte *ptr){
  int u1 = ptr[0];
  u1 = u1&0x000000ff;
  int u2 = ptr[1];
  u2 = (u2<<8)&0x0000ff00;
  int u3 = ptr[2];
  u3 = (u3<<16)&0x00ff0000;
  int u4 = ptr[3];
  u4 = (u4<<24)&0xff000000;
  return u1+u2+u3+u4;
}

uint m[4] = {0,0,0,0};
void loop() {
  byte error, address;
  int nDevices;
  unsigned char buf[20];
  int i;
  uint v;

  memset(buf, 0, 20);
  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    WIRE.beginTransmission(address);
    error = WIRE.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      if( address == 16)
        m[0] = address;
      if( address == 17)
        m[1] = address;
      if( address == 18)
        m[2] = address;
      if( address == 19)
        m[3] = address;
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
  for(int ind = 0; ind< 4; ind++){
    if(m[ind] != 0){
      i = 0;
      Serial.print("checking: ");Serial.println(m[ind]);
      WIRE.requestFrom(m[ind], 20, true);
      while(WIRE.available()) {
        buf[i] = WIRE.read();
        if(i < 19)
          i++;
      }
      Serial.print(" ");
      v = net2host(&buf[0]);
      Serial.print(v);         // Print the byte
      Serial.print(" ");
      v = net2host(&buf[4]);
      Serial.print(v);         // Print the byte
      Serial.print(" ");
      v = net2host(&buf[8]);
      Serial.print(v);         // Print the byte
      Serial.print(" ");
      v = net2host(&buf[12]);
      Serial.print(v);         // Print the byte
      Serial.print(" ");
      v = net2host(&buf[16]);
      Serial.print(v);         // Print the byte
      Serial.println(" done");

      Serial.print("drive motor ");Serial.println(ind*2);
      setmotor(2*ind, 0.2);
      delay(1000);
      setmotor(2*ind, 0.0);
      delay(500);
      Serial.println("turn motor");
      setmotor(2*ind+1, 0.2);
      delay(500);
      setmotor(2*ind+1, 0.0);
      Serial.println("motor done");
    }
  }
  delay(5000);           // wait 5 seconds for next scan
}

float speeds[8];

// front left, front right, back left, back right
// drive: 0, 2, 4, 6
// turn: 1, 3, 5, 7
void setmotor(int motor, float speed) {
  uint addr;
  float m1, m2;

  switch(motor){
  case 0:
  case 1:
    speeds[motor] = speed;
    m1 = speeds[0];
    m2 = speeds[1];
    addr = 16;
    break;
  case 2:
  case 3:
    speeds[motor] = speed;
    m1 = speeds[2];
    m2 = speeds[3];
    addr = 17;
    break;
  case 4:
  case 5:
    speeds[motor] = speed;
    m1 = speeds[4];
    m2 = speeds[5];
    addr = 18;
    break;
  case 6:
  case 7:
    speeds[motor] = speed;
    m1 = speeds[6];
    m2 = speeds[7];
    addr = 19;
    break;
  }
  sendmotor(addr, m1, m2);
}

// drive: m1, turn: m2
void sendmotor(uint addr, float m1, float m2) {
  // -1->0; 1->255
  byte v[2];
  v[0] = ((m1 + 1.0)/2.0)*255;
  v[1] = ((m2 + 1.0)/2.0)*255;
  WIRE.beginTransmission(addr);
  WIRE.write(v, 2);
  WIRE.endTransmission();
}