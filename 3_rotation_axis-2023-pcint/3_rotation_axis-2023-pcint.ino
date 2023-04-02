/* Modified and updated with PCINT code for QNH knob by Hakan GÖKMEN April2023 
 * 
 *  *  HDG AND CRS Encoders Pins:
 *  Rotary Encoder 1 - (OUTA-OUTB-SW) = Arduino Pins (0,1,15)
 *  Rotary Encoder 2 - (OUTA-OUTB-SW) = Arduino Pins (2,3,6)
 *  
 *  *  QNH Encoder Pins:
 *  Rotary Encoder 3 - (OUTA-OUTB-SW) = Arduino Pins (9,8,10)
 *  
 *   Rotary Library
 *   by Brian Low
 *  https://github.com/brianlow/Rotary
 */

/* Modified HSI Knobs Sketch for Falcon BMS / DCS / FSX
 *  with additional 3rd Rotary Encoder (Z Axis)
 *    *(coming next: a 3-way Toggle Switch add-on)
 *  for Arduino Micro/Leonardo / Sparkfun Pro Micro or equiv. clones
 * by SemlerPDX Aug2019
 *
 * VETERANS-GAMING.COM
 * ( in response to reply at:
 *    http://veterans-gaming.com/index.php?/blogs/entry/32-diy-custom-game-controller-2-dial-hsi-course-and-heading-knobs/ )
 *  
 *  Encoder Library
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 * 
 *  Joystick Library 
 * by Matthew Heironimus
 * https://github.com/MHeironimus/ArduinoJoystickLibrary
 */

#define ENCODER_USE_INTERRUPTS
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Joystick.h>
#include <Rotary.h>


//Rotary Encoder Push Button Pins
int buttonArray[3] = {15, 5, 10};

//Rotary Encoder Interrupt Pins
int EncoderPin0 = 0;
int EncoderPin1 = 1;
int EncoderPin2 = 2;
int EncoderPin3 = 3;
#define EncoderPin4  9
#define EncoderPin5  8

Rotary qnh = Rotary(EncoderPin4, EncoderPin5);

//Tell the Encoder Library which pins have encoders
Encoder axisXRotation(EncoderPin0, EncoderPin1);
Encoder axisYRotation(EncoderPin2, EncoderPin3);
Encoder axisZRotation(EncoderPin4, EncoderPin5);

//Delay Time between loops
int debounceDelay = 260;

//Variables to compare current to old values
int oldX = 0;
int oldY = 0;
int oldZ = 0;
int RxAxis_Value = 1;
int RyAxis_Value = 1;
int RzAxis_Value = 1;

//Intervals for Jump/Warp Speed Rotations
int JumpSpeed = 18;
int WarpSpeed = 30;

//Set generic joystick with id 42 with 3 buttons and 3 axes
Joystick_ Joystick(0x42, 
  0x04, 3, 0,
  false, false, false, true, true, true,
  false, false, false, false, false);  


//Function to set Rotation value adjusted for the turning speed
int speedVal(int dif, int val, int dir){
  int increment = 1;
  if (dif >= WarpSpeed) {
    increment = WarpSpeed;
  }else if (dif >= JumpSpeed) {
    increment = JumpSpeed;
  }
  if (dir == 1) {
    val = val + increment;
  }else{
    val = val - increment;
  }
  //Correct Rotation within 360 deg.
  if (val < 0) {
    val = val + 360;
  }else if (val >= 360) {        
    val = val - 360;
  }
  return val;
}

void setup() { 

  //Set Encoder Pins as Pullups
  pinMode(EncoderPin0, INPUT_PULLUP);
  pinMode(EncoderPin1, INPUT_PULLUP);
  pinMode(EncoderPin2, INPUT_PULLUP);
  pinMode(EncoderPin3, INPUT_PULLUP);
  pinMode(EncoderPin4, INPUT_PULLUP);
  pinMode(EncoderPin5, INPUT_PULLUP);

  //Loop through buttons and set them as Pullups
  for(int x = 0; x < sizeof(buttonArray); x++) {
    pinMode(buttonArray[x], INPUT_PULLUP);
  }

  // Added by Ghost
  qnh.begin();
  PCICR |= (1 << PCIE0);
  PCMSK0 |= (1 << PCINT4) | (1 << PCINT5);
  sei();
  
  
  //Set Range of custom Axes
  Joystick.setRxAxisRange(0, 359);
  Joystick.setRyAxisRange(0, 359);
  Joystick.setRzAxisRange(0, 359);

  // Initialize Joystick Library
  Joystick.begin(false);
}


void loop() {

  // Loop through button pin values & set to Joystick
  for (int x = 0; x < sizeof(buttonArray); x++) {
    byte currentButtonState = !digitalRead(buttonArray[x]);
    Joystick.setButton(x, currentButtonState);
  }


  // Read "Heading" X Axis Rotation Encoder Knob
  int newX = axisXRotation.read();
  if (newX > oldX) {
    //Determine speed of increment & set output
    int difX = newX - oldX;
    RxAxis_Value = speedVal(difX, RxAxis_Value, 1);
    Joystick.setRxAxis(RxAxis_Value);
    axisXRotation.write(newX);
    oldX = newX;

  }else if (newX < oldX) {
    //Determine speed of decrement & set output
    int difX = oldX - newX;
    RxAxis_Value = speedVal(difX, RxAxis_Value, 0);
    Joystick.setRxAxis(RxAxis_Value);
    axisXRotation.write(newX);
    oldX = newX;
  }


  // Read "Course" Y Axis Rotation Encoder Knob
  int newY = axisYRotation.read();
  if (newY > oldY) {
    //Determine speed of increment & set output
    int difY = newY - oldY;
    RyAxis_Value = speedVal(difY, RyAxis_Value, 1);
    Joystick.setRyAxis(RyAxis_Value);
    axisYRotation.write(newY);
    oldY = newY;

  }else if (newY < oldY) {
    //Determine speed of decrement & set output
    int difY = oldY - newY;
    RyAxis_Value = speedVal(difY, RyAxis_Value, 0);
    Joystick.setRyAxis(RyAxis_Value);
    axisYRotation.write(newY);
    oldY = newY;
  }

  //Send Joystick info through USB
  Joystick.sendState();
  delay(debounceDelay);
}

ISR(PCINT0_vect) {
  
  // Read "QNH" Z Axis Rotation Encoder Knob
  // Added by Ghost
  int newZ = axisZRotation.read(); 
  unsigned char result = qnh.process();
  if (result == DIR_NONE) {
    // do nothing
  }
  else if (result == DIR_CW and newZ > oldZ) {
    //Determine speed of increment & set output
    int difZ = newZ - oldZ;
    RzAxis_Value = speedVal(difZ, RzAxis_Value, 1);
    Joystick.setRzAxis(RzAxis_Value);
    axisZRotation.write(newZ);
    oldZ = newZ;
    //Serial.println("ClockWise");
  }
  else if (result == DIR_CCW and newZ < oldZ) {
    //Determine speed of decrement & set output
    int difZ = oldZ - newZ;
    RzAxis_Value = speedVal(difZ, RzAxis_Value, 0);
    Joystick.setRzAxis(RzAxis_Value);
    axisZRotation.write(newZ);
    oldZ = newZ;
    //Serial.println("CounterClockWise");
  }
  
}
