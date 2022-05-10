#include <Braccio.h>
#include <Servo.h>


Servo base;
Servo shoulder;
Servo elbow;
Servo wrist_rot;
Servo wrist_ver;
Servo gripper;

String command;

void setup() {  
  //Initialization functions and set up the initial position for Braccio
  //All the servo motors will be positioned in the "safety" position:
  //Base (M1):90 degrees
  //Shoulder (M2): 45 degrees
  //Elbow (M3): 180 degrees
  //Wrist vertical (M4): 180 degrees
  //Wrist rotation (M5): 90 degrees
  //gripper (M6): 10 degrees
  Braccio.begin();
  Serial.begin(38400);
}

void loop() {
  /*
   Step Delay: a milliseconds delay between the movement of each servo.  Allowed values from 10 to 30 msec.
   M1=base degrees. Allowed values from 0 to 180 degrees
   M2=shoulder degrees. Allowed values from 15 to 165 degrees
   M3=elbow degrees. Allowed values from 0 to 180 degrees
   M4=wrist vertical degrees. Allowed values from 0 to 180 degrees
   M5=wrist rotation degrees. Allowed values from 0 to 180 degrees
   M6=gripper degrees. Allowed values from 10 to 73 degrees. 10: the toungue is open, 73: the gripper is closed.
  */
  
  // the arm is aligned upwards  and the gripper is closed
                     //(step delay, M1, M2, M3, M4, M5, M6);
  // Braccio.ServoMovement(20,         90, 90, 90, 90, 90,  73);

  // Notation: m(xx,xxx,xxx,xxx,xxx,xxx,xx)

  if(Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    
    if(command.equals("neutral")) {
      Braccio.ServoMovement(20, 90, 90, 90, 90, 90, 73);
    } else if (command.startsWith("m(")) {
      int delay_time = command.substring(2,4).toInt();
      int base_deg = command.substring(5, 8).toInt();
      int shoulder_deg = command.substring(9, 12).toInt();
      int elbow_deg = command.substring(13, 16).toInt();
      int wrist_vert_deg = command.substring(17, 20).toInt();
      int wrist_rot_deg = command.substring(21, 24).toInt();
      int grip_deg = command.substring(25, 27).toInt();
      Braccio.ServoMovement(delay_time, base_deg, shoulder_deg, elbow_deg, wrist_vert_deg, wrist_rot_deg, grip_deg);
    } else {
      Serial.println("Can't recognize command");
    }
    Serial.println("Command: " + command);
  }
}
