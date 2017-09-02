/*
 * The circuit:
 * PIN 2 is Arduino's RX -- connect to hedgehog's TX
 * PIN 3 is Arduino's TX -- connect to hedgehog's RX 
 */

#include "metal_sonic.h"
#include <SoftwareSerial.h>
#include "rgb_lcd.h"

// Thresholds
#define RADIUS_THRESHOLD 0.3 // meters
#define ANGLE_THRESHOLD 20 // degrees
#define TIME_TIL_TIMEOUT 800 // milliseconds
#define OBTUSE_ANGLE_IGNORANCE_THRESHOLD 3 // ignores

// Hedgehog communication pins
#define RX_FROM_HEDGEHOG_PIN 2
#define TX_TO_HEDGEHOG_PIN 3

// Steering control pins
#define STEERING_ENABLE_PIN 4
#define LEFT_TURN_INPUT_PIN 5
#define RIGHT_TURN_INPUT_PIN 6

// Motor control pins
#define MOTOR_ENABLE_PIN 7
#define FORWARD_INPUT_PIN 8
#define BACKWARD_INPUT_PIN 9

#define LOGIC_POWER_PIN 10

extern struct hedgehog_state_type hedgehog_state;

coordinate target_coord, old_coord, new_coord;
/* The car's bearing is calculated as the angle of the car's
   new coordinates relative to its old coordinates.
   The target's bearing is calculated as the angle of the
   target coordinates relative to the car's new coordinates.
   Absolute bearings are used here: 0 degrees is "East"
   and 270 degrees is "South". */
float car_bearing, target_bearing,
/* delta_angle = car_bearing - target_bearing
   delta_distance = distance between target_coord and new_coord */
      delta_angle, delta_distance;

bool coords_packet_received = false,
     target_pursuit_mode = false;

long last_transmission_timestamp;

enum mode {
  WHITE_MODE          = 0,
  ERROR_MODE          = 1,
  TARGET_PURSUIT_MODE = 2,
  READY_MODE          = 3,
  IDLE_MODE           = 4,
  FRESH_BOOT_MODE     = 5
} Mode;

bool front_facing = true;
byte obtuse_angle_count = 0;

SoftwareSerial Shadow (RX_FROM_HEDGEHOG_PIN, TX_TO_HEDGEHOG_PIN);
rgb_lcd lcd;

void read_hedgehog_stream() {
  while (Shadow.available()) {
    int incoming_byte = Shadow.read();  // read new byte from buffer
    // Serial.print(incoming_byte, HEX);
    // Serial.print(" ");
    
    switch (hedgehog_state.current_state(incoming_byte)) {
      case START:
        reset();
        if (receiving_1(incoming_byte) != NEXT) break;
      case NEXT:
        hedgehog_state.buf[hedgehog_state.buf_ofs++] = incoming_byte;
        hedgehog_state.crc16_sum = crc16_streaming_advance(hedgehog_state.crc16_sum, incoming_byte);
        break;
      case END:
        if ((hedgehog_state.crc16_sum = crc16_streaming_advance(hedgehog_state.crc16_sum, incoming_byte)) == 0) {
          hedgehog_state.buf[hedgehog_state.buf_ofs++] = incoming_byte;
          hedgehog_state.current_state(0);
        }
        reset();
        goto exit_loop;
    }
  }
  exit_loop:;
}

//  Marvelmind hedgehog support initialize
void setup_hedgehog() {
  Shadow.begin(9600);
  while(!Shadow.available()) delay(500);
}

void setup() {
  lcd.begin(16, 2);
  update_Mode(FRESH_BOOT_MODE);

  setup_hedgehog();
  
  update_Mode(IDLE_MODE);
  
  pinMode(STEERING_ENABLE_PIN , OUTPUT);
  pinMode(LEFT_TURN_INPUT_PIN , OUTPUT);
  pinMode(RIGHT_TURN_INPUT_PIN, OUTPUT);
  pinMode(MOTOR_ENABLE_PIN    , OUTPUT);
  pinMode(FORWARD_INPUT_PIN   , OUTPUT);
  pinMode(BACKWARD_INPUT_PIN  , OUTPUT);
  pinMode(LOGIC_POWER_PIN     , OUTPUT);
  
  digitalWrite(LOGIC_POWER_PIN, HIGH);
}

void update_Mode(mode new_mode) {
  if (Mode != new_mode) {
    Mode = new_mode;
    lcd.clear();
    
    switch (Mode) {
    case WHITE_MODE:
    case ERROR_MODE:
    case TARGET_PURSUIT_MODE:
      lcd.setColor(Mode);
      break;
    case READY_MODE:
      lcd.setColor(3);
      lcd.print(F("X="));
      lcd.setCursor(0, 1);
      lcd.print(F("Y="));
      break;
    case IDLE_MODE:
      motor_off();
      turn_neutral();
      
      lcd.setColorAll();
      lcd.print(F("Listening..."));
      break;
    case FRESH_BOOT_MODE:
      lcd.setColorAll();
      lcd.print(F("No signal."));
    }
  }
}

void turn_neutral() {
  digitalWrite(STEERING_ENABLE_PIN, LOW);
  digitalWrite(LEFT_TURN_INPUT_PIN, LOW);
  digitalWrite(RIGHT_TURN_INPUT_PIN, LOW);
}

void steer_wheel_CCW() {
  digitalWrite(STEERING_ENABLE_PIN, HIGH);
  digitalWrite(LEFT_TURN_INPUT_PIN, HIGH);
  digitalWrite(RIGHT_TURN_INPUT_PIN, LOW);
}

void steer_wheel_CW() {
  digitalWrite(STEERING_ENABLE_PIN, HIGH);
  digitalWrite(RIGHT_TURN_INPUT_PIN, HIGH);
  digitalWrite(LEFT_TURN_INPUT_PIN, LOW);
}

void motor_off() {
  digitalWrite(MOTOR_ENABLE_PIN, LOW);
  digitalWrite(FORWARD_INPUT_PIN, LOW);
  digitalWrite(BACKWARD_INPUT_PIN, LOW);
}

void motor_front() {
  digitalWrite(MOTOR_ENABLE_PIN, HIGH);
  digitalWrite(FORWARD_INPUT_PIN, HIGH);
  digitalWrite(BACKWARD_INPUT_PIN, LOW);
}

void motor_rear() {
  digitalWrite(MOTOR_ENABLE_PIN, HIGH);
  digitalWrite(BACKWARD_INPUT_PIN, HIGH);
  digitalWrite(FORWARD_INPUT_PIN, LOW);
}

void (*turn_left)() = steer_wheel_CCW;
void (*turn_right)() = steer_wheel_CW;
void (*move_forward)() = motor_front;
void (*move_backward)() = motor_rear;

void flip_direction() {
  front_facing = !front_facing;
  if (front_facing) {
    turn_left = steer_wheel_CCW;
    turn_right = steer_wheel_CW;
    move_forward = motor_front;
    move_backward = motor_rear;
  }
  else {
    turn_left = steer_wheel_CW;
    turn_right = steer_wheel_CCW;
    move_forward = motor_rear;
    move_backward = motor_front;
  }
}
  

void loop() {
  start:
  read_hedgehog_stream();
  
  if (coords_packet_received) {
    // Rest flag
    coords_packet_received = false;
    // Renew timetamp
    last_transmission_timestamp = millis();
    // Gets car bearing in degrees
    car_bearing = getAngle(new_coord, old_coord);
    // Updates coord
    old_coord = new_coord;
    
    if (!target_pursuit_mode) {
      if (!isEmpty()) {
        // Dequeue path
        path_command path = dequeue();
        target_coord.x = path.param1 / 100.0;
        target_coord.y = path.param2 / 100.0;
        
        target_pursuit_mode = true;
      }
      else {
        // Set ready and return to start
        update_Mode(READY_MODE);
        lcd.setCursor(2, 0);
        lcd.print(new_coord.x);
        if (new_coord.x >= 0) lcd.print(' ');
        lcd.setCursor(2, 1);
        lcd.print(new_coord.y);
        if (new_coord.y >= 0) lcd.print(' ');
        
        goto start;
      }
    }
    
    update_Mode(TARGET_PURSUIT_MODE);
    
    target_bearing = getAngle(target_coord, new_coord);
    
    delta_angle = getDeltaAngle(target_bearing, car_bearing);
    delta_distance = getDistance(new_coord, target_coord);
    
    if (delta_distance <= RADIUS_THRESHOLD) {
      update_Mode(ERROR_MODE);
      
      target_pursuit_mode = false;
      
      motor_off();
      turn_neutral();
    }
    else {
      // Begin corrective actions here
      if (fabs(delta_angle) < ANGLE_THRESHOLD) {
        update_Mode(WHITE_MODE);
        turn_neutral();
        motor_off();
      }
      else {
        // If reported angle is obtuse, ignore it unless ignore count
        // reaches threshold. In that case, flip direction.
        if (fabs(delta_angle) >= 90) {
          ++obtuse_angle_count;
          if (obtuse_angle_count >= OBTUSE_ANGLE_IGNORANCE_THRESHOLD) {
            flip_direction();
          }
        }
        else {
          obtuse_angle_count = 0;
        }
        
        if (delta_angle > 0) {
          turn_left();
        }
        else {
          turn_right();
        }
        move_forward();
      }
      // TODO: Implement 90 degree case
    }
  }
  else {
    // Check for timeout
    if (millis() - last_transmission_timestamp >= TIME_TIL_TIMEOUT) {
      update_Mode(IDLE_MODE);
    }
  }
}
