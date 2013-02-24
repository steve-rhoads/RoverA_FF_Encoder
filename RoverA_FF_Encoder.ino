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
// this is a test to see if I can change on a different computer...

#include <AFMotor.h>
void InitSerial(int); 
void PrintEncoders();

// FORWARD, BACKWARD, BRAKE, RELEASE
int DirectionLeft;   // used to determine whether to increment or decrement the encoder
int DirectionRight;  // used to determine whether to increment or decrement the encoder
int Drive(int directionLeft, int speedLeft, int encLeft, int directionRight, int speedRight, int encRight);
int SetSpeed(int motor, int speed);
void StopMotors();
void ResetEncoders();

const int ENC_DIST_SLOW   = 3;   // point at which we slow down to hit encoder target exactly
const int ENC_SPEED_SLOW  = 50;  // speed percent used to approach target encoder
const int LED_PIN         = 13;

AF_DCMotor Motor_Left(4, MOTOR34_1KHZ);   // both left motors on port 4 
AF_DCMotor Motor_Right(3, MOTOR34_1KHZ);  // both right motors on port 3
//AF_DCMotor Motor_Left(1, MOTOR12_1KHZ);   // both left motors on port 4 
//AF_DCMotor Motor_Left(2, MOTOR12_1KHZ);   // both left motors on port 4 
const int speed = 100;
int pwm;
 
#define RIGHT  0
#define LEFT   1

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
  
  attachInterrupt(LEFT_ENCODER_INT, LwheelSpeed, CHANGE);    //init the interrupt mode for the digital pin 2
  attachInterrupt(RIGHT_ENCODER_INT, RwheelSpeed, CHANGE);   //init the interrupt mode for the digital pin 3

  // initialize the motors to stop...(Left,Right,Speed[0..255],waitMsec)
  PrintEncoders();
  // Left:direction, speed%, target encoder;Right:direction,speed%,target encoder 
  Drive(FORWARD,100,60, FORWARD,100,60);
  PrintEncoders();
  
  StopMotors();
  delay(5000);
  
  Drive(BACKWARD,100,0,BACKWARD,100,0);
  PrintEncoders();
  
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
void LwheelSpeed()
{
  if (DirectionLeft == FORWARD){
    encoder[LEFT] ++;  //count the left wheel encoder interrupts
  }
  else {
    encoder[LEFT] --;  //count the left wheel encoder interrupts
  }
}
 
void RwheelSpeed()
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

int Drive(int directionLeft, int speedLeft, int encLeft, int directionRight, int speedRight, int encRight){
  // Set basic speed, unless we're close on the encoder, then override with a slow speed.
  // Overall, we'll move until the encoders hit the desired value - use a big while loop
  //while ((encoder[LEFT] != encLeft) && (encoder[RIGHT] != encRight)){
    while ((encoder[LEFT] != encLeft)){
    PrintEncoders();
    // Set the direction *****************************************************
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
    Serial3.print("EncRemaining:");
    Serial3.print(String(abs(encLeft - encoder[LEFT])));
    Serial3.println("");
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
} 

void PrintSerial(String message){
  Serial.println(message);
  Serial3.println(message);
}

void PrintEncoders(){
  String message = "L:" + String(encoder[LEFT]) + "-R:" + String(encoder[RIGHT]); 
  PrintSerial(message);    
}

int SetSpeed(int motor, int speed){
 // scale motor percent(0-100) into pwm range (0-255) 
 pwm  = map(speed, 0,100, 0,255);  
 pwm  = constrain(pwm,0,255);

 if (motor == RIGHT){
   Motor_Right.setSpeed(pwm);    
 }
 else {
   Motor_Left.setSpeed(pwm);
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
