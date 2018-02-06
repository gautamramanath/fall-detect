#include <Wire.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(9, 10);

long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
float netForce;

long gyroX, gyroY, gyroZ;
float rotX, rotY, rotZ;
float netRot;

int state;
int buzzer=7;
int flag;
int simflag;
int buttonState = 0; 

void setup() {
  mySerial.begin(9600);   // Setting the baud rate of GSM Module 
  Serial.begin(9600);
  Wire.begin();
  setupMPU();
  pinMode(13,OUTPUT);
  pinMode(buzzer,OUTPUT);
  digitalWrite(buzzer,LOW);
  pinMode(4, INPUT);
}


void loop() {
  recordAccelRegisters();
  recordGyroRegisters();
  printData();
  delay(100);

  state=1;
  flag=0;
  simflag=0;

  if(state==1)
  {
    //if(rotX>50 || rotY>50 || rotZ>50 || rotX<-50 || rotY<-50 || rotZ<-50)
    if(netRot>75)
    {
      Serial.println("FALL SENSED");
      digitalWrite(13,HIGH);
    //  digitalWrite(buzzer,HIGH);
      delay(3000);
      digitalWrite(13,LOW);
      digitalWrite(buzzer,LOW);
      state=2;
      
      delay(1000); 
    }
  }
  if(state==2)
  {
    //if((gForceZ>0.50 && (gForceY>0.50 || gForceY<-0.50)) || (gForceZ>0.50 && (gForceX>0.50 || gForceX<-0.50)))  //or (XZ>0.5 YZ>0.5 XY>0.5) or (XY<-.5 ....) or (X>0.5Y<-0.5 ....) 
    if(netForce>0.55 || netForce<0.3)
    {                                                                                                           //or (X<-0.5Y>0.5 ....)
      Serial.println("LEGITIMATE FALL");
      digitalWrite(13,HIGH);
      digitalWrite(buzzer,HIGH);
      /*delay(200);
      digitalWrite(buzzer,LOW);
      digitalWrite(13,HIGH);*/
      delay(10000);
      state=1;
      simflag=1;

    }
  } 

  buttonState = digitalRead(2);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    flag=1;
    digitalWrite(13,LOW);
  }
  //Here write the button code. And if HIGH set flag=1

  if(flag == 1)
  {
    digitalWrite(buzzer,LOW);
    digitalWrite(13,LOW);
  }
   if(flag==0 && simflag==1)
  {
    if (mySerial.available()>0)
    Serial.write(mySerial.read());

    mySerial.println("ATD9945312861;"); // ATDxxxxxxxxxx; -- watch out here for semicolon at the end!!
    delay(1000);
   
    mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
    delay(1000);  // Delay of 1000 milli seconds or 1 second
    mySerial.println("AT+CMGS=\"9945312861\"\r"); // Replace x with mobile number
    delay(1000);
    mySerial.println("FALL DETECTED");// The SMS text you want to send
    delay(100);
    mySerial.println((char)26);// ASCII code of CTRL+Z
    delay(1000);

    /*mySerial.println("ATD9945312861;"); // ATDxxxxxxxxxx; -- watch out here for semicolon at the end!!
    delay(1000);*/
   }

  

  //digitalWrite(13,LOW);
  //digitalWrite(buzzer,LOW);
  
  delay(1000);    
  
}

void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s 
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00000000); //Setting the accel to +/- 2g
  Wire.endTransmission(); 
}

void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}

void processAccelData(){
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0; 
  gForceZ = accelZ / 16384.0;
  netForce = (gForceX+gForceY+gForceZ)/3;
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processGyroData();
}

void processGyroData() {
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0; 
  rotZ = gyroZ / 131.0;
  netRot = sqrt((rotX*rotX)+(rotY*rotY)+(rotZ*rotZ));
}

void printData() {
  Serial.print("Gyro (deg)");
  Serial.print(" X=");
  Serial.print(rotX);
  Serial.print(" Y=");
  Serial.print(rotY);
  Serial.print(" Z=");
  Serial.print(rotZ);
  Serial.print(" NET=");
  Serial.print(netRot);
  Serial.print(" Accel (g)");
  Serial.print(" X=");
  Serial.print(gForceX);
  Serial.print(" Y=");  
  Serial.print(gForceY);
  Serial.print(" Z=");
  Serial.print(gForceZ);
  Serial.print(" NET=");
  Serial.println(netForce);


  /*Serial.print("AcX = "); Serial.print(accelX);
  Serial.print(" | AcY = "); Serial.print(accelY);
  Serial.print(" | AcZ = "); Serial.print(accelZ);
  //Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);  //equation for temperature in degrees C from datasheet
  Serial.print(" | GyX = "); Serial.print(gyroX);
  Serial.print(" | GyY = "); Serial.print(gyroY);
  Serial.print(" | GyZ = "); Serial.println(gyroZ);*/

}

