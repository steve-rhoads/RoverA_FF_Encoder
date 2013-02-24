// #
// # Editor     : Lauren from DFRobot
// # Date       : 17.01.2012
 
// # Product name: Wheel Encoders for DFRobot 3PA and 4WD Rovers
// # Product SKU : SEN0038
 
// # Description:
// # The sketch for using the encoder on the DFRobot Mobile platform
 
// # Connection:
// #        left wheel encoder  -> Digital pin 2
// #        right wheel encoder -> Digital pin 3
// #
#include "enum.h"
#include <AFMotor.h>
const int RIGHT               = 0;
const int LEFT                = 1;

const int ENC_DIST_SLOW       = 10;   // point at which we slow down to hit encoder target exactly
const int ENC_SPEED_SLOW      = 25;  // speed percent used to approach target encoder

int motor_speed = 100;
int pwm;
const int LED_PIN             = 13;

const int PULSE_PER_ROTATION  = 20;
const float WHEEL_DIAMETER    = 2.6; // inches
//const float PI                = 3.1415927;

// FORWARD, BACKWARD, BRAKE, RELEASE
int DirectionLeft;   // used to determine whether to increment or decrement the encoder
int DirectionRight;  // used to determine whether to increment or decrement the encoder

void InitSerial(int baud_rate); 
int CalculateEncoderTarget(float inches);
int Drive(DriveDirection drive_dir, int drive_speed, float drive_distance); 
void Turn(TurnType turn_type, TurnDirection turn_dir, int drive_speed, float drive_distance); 
int SetSpeed(int motor, int target_speed);
void StopMotors();
void PrintEncoders();
void ResetEncoders();

//AF_DCMotor Motor_Left(4, MOTOR34_1KHZ);   // both left motors on port 4 
//AF_DCMotor Motor_Left(1, MOTOR12_1KHZ);   // both left motors on port 4 
//AF_DCMotor Motor_Left(2, MOTOR12_1KHZ);   // both left motors on port 4 
AF_DCMotor Motor_Left(4);

//AF_DCMotor Motor_Right(3, MOTOR34_1KHZ);  // both right motors on port 3
AF_DCMotor Motor_Right(3);

// on a DUE, this is the pin #
// on a Mega2560, it is the interrupt. INT#(PIN#): 0(2),1(3),2(21),3(20),4(19),5(18)
// on a Leonardo, INT#(PIN#). 0(3),1(2),2(0),3(1)
#define RIGHT_ENCODER_INT (3)
#define LEFT_ENCODER_INT  (2) 
 
long encoder[2]   = {0,0};
int lastSpeed[2]  = {0,0};  

boolean go_enabled = true;  
   
void setup(){
  InitSerial(9600);
  ResetEncoders();
  
  attachInterrupt(LEFT_ENCODER_INT, EncoderLFT, CHANGE);    //init the interrupt mode for the digital pin 2
  attachInterrupt(RIGHT_ENCODER_INT, EncoderRT, CHANGE);   //init the interrupt mode for the digital pin 3

  // initialize the motors to stop...(Left,Right,Speed[0..255],waitMsec)
  PrintEncoders();
  //int Drive(DriveDirection, int drive_speed, float drive_distance); 
  Drive(DRIVE_DIR_FWD,100,10); // 100% for 10inches
  StopMotors();
  PrintEncoders();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
void loop(){
/* 
  static unsigned long timer = 0;                //print manager timer
 
  if(millis() - timer > 100){     
    PrintEncoders();
    
    lastSpeed[LEFT]  = encoder[LEFT];   //record the latest speed value
    lastSpeed[RIGHT] = encoder[RIGHT];
    timer = millis();
  }
*/
}
 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void EncoderLFT()
{
  if (DirectionLeft == FORWARD){
    encoder[LEFT] ++;  //count the left wheel encoder interrupts
  }
  else {
    encoder[LEFT] --;  //count the left wheel encoder interrupts
  }
}
 
void EncoderRT()
{
  if (DirectionRight == FORWARD){
    encoder[RIGHT] ++; //count the right wheel encoder interrupts
  }
  else {
    encoder[RIGHT] --; //count the right wheel encoder interrupts  
  }
}

void InitSerial(int baud_rate=9600){
  Serial.begin(baud_rate);
  Serial3.begin(baud_rate);
}

//int Drive(DriveDirection, int drive_speed, float drive_distance); 
int Drive(DriveDirection drive_direction, int drive_speed, float drive_distance){
  // translate inches to encoder target
  int enc_target = CalculateEncoderTarget(drive_distance);
  if (drive_direction == DRIVE_DIR_REV){
    DirectionLeft   = BACKWARD;
    DirectionRight  = BACKWARD;
    enc_target      *= -1; // make it negative
  }
  else {
    DirectionLeft   = FORWARD;
    DirectionRight  = FORWARD;
  }
  // let's see what the computer thinks
  Serial3.println("EncTarget=" + String(enc_target) + " Direction=" + String(DirectionLeft));;
  
  // Set basic speed, unless we're close on the encoder, then override with a slow speed.
  // Overall, we'll move until the encoders hit the desired value - using a while loop
  // Reset both encoders
  ResetEncoders();
  
  // now work the encoders until they both show the target value
  // TODO: right motors aren't working, ... so not including them in this equation.
  while ((encoder[LEFT] != enc_target)){
    Serial3.println("ELC:" + String(encoder[LEFT]) + "/ELT:" + String(enc_target) + "-Dir:" + String(encoder[LEFT] < enc_target) + "-DL:" + DirectionLeft );  
    
    // Set LEFT Direction ---------------------------------------
    if (encoder[LEFT] < enc_target){
      DirectionLeft  = FORWARD;
    }
    else if (encoder[LEFT] > enc_target){
      DirectionLeft  = BACKWARD;
    }
    else {
      DirectionLeft  = BRAKE;  // may need to use RELEASE
    }  
    // Set RIGHT Direction ---------------------------------------
    if (encoder[RIGHT] < enc_target){
      DirectionRight  = FORWARD;
    }
    else if (encoder[RIGHT] > enc_target){
      DirectionRight  = BACKWARD;
    }
    else {
      DirectionRight  = BRAKE;  // may need to use RELEASE
    } 
    
    // Set Speed
    // if we're close, let's slow down. Our encoders are 20 PulsePerRotation (PPR)
    // Set Left, check absolute value
    if (abs(enc_target - encoder[LEFT]) > ENC_DIST_SLOW) {
      SetSpeed(LEFT,drive_speed);
    }
    else {
      SetSpeed(LEFT, ENC_SPEED_SLOW); // speed is in percent (0-100)
    }
    // Set Right, check absolute value
    if (abs(enc_target - encoder[RIGHT]) > ENC_DIST_SLOW) {
      SetSpeed(RIGHT,drive_speed);
    }
    else {
      SetSpeed(RIGHT, ENC_SPEED_SLOW); // speed is in percent (0-100)
    }
    // Now move
    Motor_Left.run(DirectionLeft);
    Motor_Right.run(DirectionRight);
  } // while not encMotor = encoder[Motor]  
  // now reset the encoders! You've reached your target
  ResetEncoders();  
} 

void PrintSerial(String message){
  Serial.println(message);
  Serial3.println(message);
}

void PrintEncoders(){
  String message = "L:" + String(encoder[LEFT]) + "-R:" + String(encoder[RIGHT]); 
  PrintSerial(message);    
}

int SetSpeed(int motor, int target_speed){
  int scaled_speed=0;
  // scale motor percent(0-100) into pwm range (0-255) 
  scaled_speed  = map(target_speed, 0,100, 0,255);  
  scaled_speed  = constrain(scaled_speed,0,255);

 if (motor == RIGHT){
   Motor_Right.setSpeed(scaled_speed);    
 }
 else {
   Motor_Left.setSpeed(scaled_speed);
 }
}

void StopMotors(){
  Motor_Left.setSpeed(0);
  Motor_Right.setSpeed(0);
  // set direction also includes FORWARD|BACKWARD|RELEASE|BRAKE
  Motor_Left.run(BRAKE);
  Motor_Right.run(BRAKE);
}

void ResetEncoders(){
  encoder[LEFT]   = 0;                 //clear the data buffer
  encoder[RIGHT]  = 0;
}

void Turn(TurnType turn_dir, int target_speed, int distance_inches){
  /*
  // Set basic speed, unless we're close on the encoder, then override with a slow speed.
  // Overall, we'll move until the encoders hit the desired value - use a big while loop
  //while ((encoder[LEFT] != encLeft) && (encoder[RIGHT] != encRight)){
    while ((encoder[LEFT] != encLeft)){
        Serial3.println("ELC:" + String(encoder[LEFT]) + "/ELT:" + String(encLeft) + "-Dir:" + String(encoder[LEFT]<encLeft) + "-DL:" + DirectionLeft );  
    //PrintEncoders();
    // Set the direction *****************************************************
    //DirectionLeft   = directionLeft;
    //DirectionRight  = directionRight;
    
    // Set LEFT Direction ---------------------------------------
    if (encoder[LEFT] < encLeft){
      DirectionLeft  = FORWARD;
    }
    else if (encoder[LEFT] > encLeft){
      DirectionLeft  = BACKWARD;
    }
    else {
      DirectionLeft  = BRAKE;  // may need to use RELEASE
    }  
    // Set RIGHT Direction ---------------------------------------
    if (encoder[RIGHT] < encRight){
      DirectionRight  = FORWARD;
    }
    else if (encoder[RIGHT] > encRight){
      DirectionRight  = BACKWARD;
    }
    else {
      DirectionRight  = BRAKE;  // may need to use RELEASE
    } 
    
    // Set Speed
    // if we're close, let's slow down. Our encoders are 20 PulsePerRotation (PPR)
    // Set Left, check absolute value
    if (abs(encLeft - encoder[LEFT]) > ENC_DIST_SLOW) {
      SetSpeed(LEFT,speedLeft);
    }
    else {
      SetSpeed(LEFT, ENC_SPEED_SLOW); // speed is in percent (0-100)
    }
    // Set Right, check absolute value
    if (abs(encRight - encoder[RIGHT]) > ENC_DIST_SLOW) {
      SetSpeed(RIGHT,speedRight);
    }
    else {
      SetSpeed(RIGHT, ENC_SPEED_SLOW); // speed is in percent (0-100)
    }
    // Now move
    Motor_Left.run(DirectionLeft);
    Motor_Right.run(DirectionRight);
  } // while not encMotor = encoder[Motor]  
  // now reset the encoders! You've reached your target
  ResetEncoders();  
  */
}

int CalculateEncoderTarget(float distance_inches){
  // get circumference
  // divide by the PulsePerRotation (PPR)
  // add 1/2 to ensure truncation falls on the right number
  return (int)((distance_inches / (PI * WHEEL_DIAMETER/PULSE_PER_ROTATION))+ 0.5);
}
