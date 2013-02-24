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

const int ENC_DIST_SLOW       = 17;   // point at which we slow down to hit encoder target exactly
const int ENC_SPEED_SLOW      = 25;  // speed percent used to approach target encoder

const int LED_PIN             = 13;

const int PULSE_PER_ROTATION  = 20;
const float WHEEL_DIAMETER    = 2.4; // inches
const int MIN_RUN_TIME        = 100; // milliseconds after a motor run command...to give time to act

int motor_speed = 100;
int pwm;

//const float PI                = 3.1415927;

// FORWARD, BACKWARD, BRAKE, RELEASE
int DirectionLeft;   // used to determine whether to increment or decrement the encoder
int DirectionRight;  // used to determine whether to increment or decrement the encoder

void InitSerial(int baud_rate); 
int CalculateEncoderTarget(float inches);
int Drive(DriveDirection drive_dir, int drive_speed, float drive_distance); 
void Turn(TurnType turn_type, TurnDirection turn_dir, int drive_speed, float drive_distance); 
int SetSpeed(int encoder_current_value, int encoder_target_value, int target_speed_pct);
int SetDriveDirection(DriveDirection drive_direction, int encoder_current_value, int encoder_target_value);
void StopMotors();
void PrintEncoders();
void ResetEncoders();

//AF_DCMotor Motor_Left(4, MOTOR34_1KHZ);   // both left motors on port 4 
//AF_DCMotor Motor_Left(1, MOTOR12_1KHZ);   // both left motors on port 4 
//AF_DCMotor Motor_Left(2, MOTOR12_1KHZ);   // both left motors on port 4 
AF_DCMotor Motor_Left(4, MOTOR34_64KHZ);

//AF_DCMotor Motor_Right(3, MOTOR34_1KHZ);  // both right motors on port 3
AF_DCMotor Motor_Right(3, MOTOR34_64KHZ);

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
  Drive(DRIVE_DIR_FWD,100,25); // 100% for 10inches
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
    enc_target      *= -1; // make it negative
  }  
  // Set basic speed, unless we're close on the encoder, then override with a slow speed.
  // Overall, we'll move until the encoders hit the desired value - using a while loop
  // Reset both encoders
  ResetEncoders();
  
  // now work the encoders until they both show the target value
  // TODO: right motors aren't working, ... so not including them in this equation.
  while ((encoder[LEFT] != enc_target)){
    //int SetDriveDirection(DriveDirection drive_direction, int encoder_current_value, int encoder_target_value)
    DirectionLeft   = SetDriveDirection(drive_direction, encoder[LEFT], enc_target);
    DirectionRight  = SetDriveDirection(drive_direction, encoder[RIGHT], enc_target);  
    // set speed. If close to target, slow down.
    Motor_Left.setSpeed(SetSpeed(encoder[LEFT], enc_target, drive_speed));
    Motor_Right.setSpeed(SetSpeed(encoder[RIGHT], enc_target, drive_speed));
    // debug
    Serial3.println("ELC:" + String(encoder[LEFT]) + "/ELT:" + String(enc_target) + "-Dir:" + String(encoder[LEFT] < enc_target) + "-DL:" + DirectionLeft + " Speed:" + SetSpeed(encoder[LEFT], enc_target, drive_speed));  
    // Now move
    // Motor_Left.run(DirectionLeft);
    Motor_Left.run(BACKWARD);
    Motor_Right.run(DirectionRight);
    delay(MIN_RUN_TIME); // need a little time so that it can do what it plans    
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

int SetSpeed(int encoder_current_value, int encoder_target_value, int target_speed_pct){
  // calculate how far to go
  int remaining     = abs(abs(encoder_current_value)-abs(encoder_target_value));
  // if we have more than a threshold distance to go, go with the full scaled speed, otherwise slow down.
  if (remaining < ENC_DIST_SLOW){
    target_speed_pct = ENC_SPEED_SLOW; // override with global "slow" setting
  }
  // scale motor percent(0-100) into pwm range (0-255) 
  int scaled_speed  = map(target_speed_pct, 0,100, 0,255);  
  return scaled_speed      = constrain(scaled_speed,0,255);
}

void StopMotors(){
  Motor_Left.setSpeed(0);
  Motor_Right.setSpeed(0);
  // set direction also includes FORWARD|BACKWARD|RELEASE|BRAKE
  Motor_Left.run(RELEASE);
  Motor_Right.run(RELEASE);
}

void ResetEncoders(){
  encoder[LEFT]   = 0;                 //clear the data buffer
  encoder[RIGHT]  = 0;
}

void Turn(TurnType turn_type,TurnDirection turn_dir, int target_speed, int drive_distance){
  // translate inches to encoder target
  int enc_target = CalculateEncoderTarget(drive_distance);
  // swing turns first: to the left
  if ((turn_type == TURN_TYPE_SWING) && (turn_dir == TURN_LEFT)){
    Motor_Left.setSpeed(0);
    Motor_Right.setSpeed(SetSpeed(encoder[RIGHT], enc_target, target_speed));
    Motor_Left.run(RELEASE);
    Motor_Right.run(FORWARD);
  }
  // swing turns first: to the right
  if ((turn_type == TURN_TYPE_SWING) && (turn_dir == TURN_RIGHT)){
    Motor_Left.setSpeed(SetSpeed(encoder[LEFT], enc_target, target_speed));
    Motor_Right.setSpeed(0);
    Motor_Left.run(FORWARD);
    Motor_Right.run(RELEASE);
  }
  // point turns (i.e. Rotation in place): to the left...
  if ((turn_type == TURN_TYPE_POINT) && (turn_dir == TURN_LEFT)){
    Motor_Left.setSpeed(SetSpeed(encoder[LEFT], enc_target, target_speed));
    Motor_Right.setSpeed(SetSpeed(encoder[RIGHT], enc_target, target_speed));
    Motor_Left.run(BACKWARD);
    Motor_Right.run(FORWARD);
  }
  // point turns (i.e. Rotation in place): to the right...
  if ((turn_type == TURN_TYPE_POINT) && (turn_dir == TURN_RIGHT)){
    Motor_Left.setSpeed(SetSpeed(encoder[LEFT], enc_target, target_speed));
    Motor_Right.setSpeed(SetSpeed(encoder[RIGHT], enc_target, target_speed));
    Motor_Left.run(FORWARD);
    Motor_Right.run(BACKWARD);
  } 
}

int CalculateEncoderTarget(float distance_inches){
  // get circumference
  // divide by the PulsePerRotation (PPR)
  // add 1/2 to ensure truncation falls on the right number
  return (int)((distance_inches / (PI * WHEEL_DIAMETER/PULSE_PER_ROTATION))+ 0.5);
}

int SetDriveDirection(DriveDirection drive_direction, int encoder_current_value, int encoder_target_value){
  if (encoder_current_value < encoder_target_value){
    return FORWARD;
  }
  else if (encoder_current_value > encoder_target_value){
    return BACKWARD;
  }
  else {
    return RELEASE; // Shield does not implement "BRAKE"
  }
}
