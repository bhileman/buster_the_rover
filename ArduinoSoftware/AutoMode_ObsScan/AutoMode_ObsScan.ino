/* 
Autonomous rover, with ultrasonic crash detection. 
*/
#include <NewPing.h>

#define TRIGGER_PIN  13  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     12  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <Servo.h>

Servo frontServo;

int posDefault = 42;
int scanMax = 75;
int scanMin = posDefault - (scanMax - posDefault);
int driveScanMax = 60;
int driveScanMin = posDefault - (scanMax - posDefault);

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

//Right Motors
Adafruit_DCMotor *Motor1 = AFMS.getMotor(1);
Adafruit_DCMotor *Motor2 = AFMS.getMotor(2);
//Left Motors
Adafruit_DCMotor *Motor3 = AFMS.getMotor(3);
Adafruit_DCMotor *Motor4 = AFMS.getMotor(4);

int mSpd = 100;         // set motor speed
const int startPin = 11;
const int stopPin = 10;
int buttonState1 = 1;
int buttonState2 = 1;
int mode = 1;

const int eStopPin1 = 4;     
const int eStopPin2 = 5;
int eStopState1 = 0;
int eStopState2 = 0;  

void setup() {
  Serial.begin(9600);           // set up Serial library at 9600 bps
  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

  pinMode(startPin, INPUT);
  pinMode(stopPin, INPUT);
  pinMode(eStopPin1, INPUT);
  pinMode(eStopPin2, INPUT);
  frontServo.attach(7);
  frontServo.write(scanMin);
  
  // Set initial values of motors to speed 0
  Motor1->setSpeed(0);
  Motor1->run(FORWARD);
  Motor1->run(RELEASE);
  Motor2->setSpeed(0);
  Motor2->run(FORWARD);
  Motor2->run(RELEASE);
  Motor3->setSpeed(0);
  Motor3->run(FORWARD);
  Motor3->run(RELEASE);
  Motor4->setSpeed(0);
  Motor4->run(FORWARD);
  Motor4->run(RELEASE);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
}

void loop() {
  delay(50);
//Serial.println("the mode is:" + String(mode));
  modeButtonCheck(); 
  if (estopCheck() == true) {
    mode = 3;
  }
  switch(mode){
    case 1:
      mOff();
      break;
    case 2:
      mForward();
      obsScanLeftDrive();
      obsScanRightDrive();
      break;
     case 3:
      mBackward();
      delay(1000);
      mOff();
      ObsScan();
      mode = 2;
      break;
  }
}

/* 
void ObsAvoid(){
  int dist = sonar.ping_cm(); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println(String(dist));
  if (dist < 30 && dist > 2){
    mOff();
    ObsScan();
  }
  else {
    mForward();
  }
}
*/

void modeButtonCheck(){
  buttonState1 = digitalRead(startPin);
  buttonState2 = digitalRead(stopPin);
  if (buttonState2 == LOW){
    Serial.println("mode 1");
    delay(100);
    mode = 1;  //all off standby
  } if (buttonState1 == LOW){
//    Serial.println("mode 2");
//    delay(100);
    mode = 2;  //normal operation
  } 
}

bool estopCheck(){
  eStopState1 = digitalRead(eStopPin1);
  eStopState2 = digitalRead(eStopPin2);
  if (eStopState1 == HIGH || eStopState2 == HIGH){
    return true;  //bumper e-stop escape mode
  } else {
    return false;
  }
}

void obsScanLeftDrive(){
  for (int pos = driveScanMin; pos <= driveScanMax; pos += 5) { // goes from default to full scan (left)
    if(estopCheck() == true){
      mode = 3;
      break;
    }
    frontServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(100);                       // waits 15ms for the servo to reach the position
    int dist = sonar.ping_cm();
    Serial.println(String(dist));
    if (dist < 20 && dist > 2){
      mOff();
      ObsScan();
      break;
    }
  }
}

void obsScanRightDrive(){
  for (int pos = driveScanMax; pos >= driveScanMin; pos -= 5) { // goes from default to full scan (left)
   if(estopCheck() == true){
      mode = 3;
      break;
    }
    frontServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(100);                       // waits 15ms for the servo to reach the position
    int dist = sonar.ping_cm();
    Serial.println(String(dist));
    if (dist < 20 && dist > 1){
      mOff();
      ObsScan();
      break;
    }
  }
}


void ObsScan(){
  //check to see how bad the situation is and if you need to backup
   frontServo.write(posDefault);
   delay(300);
   int distForward = sonar.ping_cm(); //take scan facing left
   delay(100);
   if (distForward <= 10){
    Serial.println("backing it up");
    mBackward();
    delay(700);
    mOff();
    delay(1000);
  }
  
  //check left
  frontServo.write(scanMax);
  delay(500);
  int distLeft = sonar.ping_cm(); //take scan facing left
  delay(500);
  Serial.println("distance left" + String(distLeft));

  //check right
  frontServo.write(scanMin);
  delay(500);
  int distRight = sonar.ping_cm(); //take scan facing right
  delay(500);
  Serial.println("distance right" + String(distRight));  
  
  //compare and take action
  if (distRight == 0){
    Serial.println("I am going right");
    mRight();
    delay(1500);  // turn right for 1.5 sec
    mOff();
  } else if (distLeft == 0){
    Serial.println("I am going left");
    mLeft();
    delay(1500);
    mOff();
  } else if (distRight >= distLeft){
    Serial.println("I am going right");
    mRight();
    delay(1500);  // turn right for 1.5 sec
    mOff();
  }
  else {
    Serial.println("I am going left");
    mLeft();
    delay(1500);
    mOff();
  }
  delay(1000);  
}

void mLeft() {
      Motor1->run(FORWARD);
      Motor1->setSpeed(mSpd);
      Motor2->run(FORWARD);
      Motor2->setSpeed(mSpd);
      Motor3->run(BACKWARD);
      Motor3->setSpeed(mSpd);
      Motor4->run(BACKWARD);
      Motor4->setSpeed(mSpd);
}

void mRight() {
      Motor3->run(FORWARD);
      Motor3->setSpeed(mSpd);
      Motor4->run(FORWARD);
      Motor4->setSpeed(mSpd);
      Motor1->run(BACKWARD);
      Motor1->setSpeed(mSpd);
      Motor2->run(BACKWARD);
      Motor2->setSpeed(mSpd);
}

void mForward() {
      Motor1->run(FORWARD);
      Motor1->setSpeed(mSpd);
      Motor2->run(FORWARD);
      Motor2->setSpeed(mSpd);
      Motor3->run(FORWARD);
      Motor3->setSpeed(mSpd);
      Motor4->run(FORWARD);
      Motor4->setSpeed(mSpd);
}

void mBackward() {
      Motor1->run(BACKWARD);
      Motor1->setSpeed(mSpd);
      Motor2->run(BACKWARD);
      Motor2->setSpeed(mSpd);
      Motor3->run(BACKWARD);
      Motor3->setSpeed(mSpd);
      Motor4->run(BACKWARD);
      Motor4->setSpeed(mSpd);
}

void mOff() {
      Motor1->setSpeed(0);
      Motor1->run(RELEASE);
      Motor2->setSpeed(0);
      Motor2->run(RELEASE);
      Motor3->setSpeed(0);
      Motor3->run(RELEASE);
      Motor4->setSpeed(0);
      Motor4->run(RELEASE);
      delay(100);
}



