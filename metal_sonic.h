#ifndef METAL_SONIC_H
#define METAL_SONIC_H

#include <Arduino.h>
#include "Path_Queue.h"
#include "Coordinates.h"

#define NUMBER_OF_BEACONS 4

//#define DEBUG 0

#ifdef DEBUG
	#define CONSOLE(x) Serial.print(F(x))
#else
	#define CONSOLE(x) 
#endif

enum state_result { START, NEXT, END };
typedef union { byte b[2]; uint16_t w; } uni_8x2_16;
typedef union { byte b[4]; uint32_t v32; } uni_8x4_32;

state_result receiving_1(int incoming_byte);
state_result receiving_2(int incoming_byte);
state_result streaming_3(int incoming_byte);
state_result cm_coordinates_4(int incoming_byte);
state_result mm_coordinates_4(int incoming_byte);
state_result mm_coordinates_5(int incoming_byte);
state_result mm_coordinates_6(int incoming_byte);
state_result mm_coordinates_END(int incoming_byte);
state_result cm_frozen_4(int incoming_byte);
state_result mm_frozen_4(int incoming_byte);
state_result reading_3(int incoming_byte);
state_result writing_3(int incoming_byte);
state_result path_4(int incoming_byte);
state_result path_5(int incoming_byte);
state_result path_6(int incoming_byte);
state_result path_END(int incoming_byte);
void reset();
uint16_t crc16_streaming_advance(uint16_t sum, byte b);

struct hedgehog_state_type {
  byte buf[8 + 14 * NUMBER_OF_BEACONS];
  byte buf_ofs = 0;
  
  state_result (*current_state)(int) = receiving_1;
  
  uint16_t crc16_sum = 0xffffU;
  struct {
    
  } blank;
  
  union {
    byte uni8;
    struct {
      unsigned bit0 : 1,
               bit1 : 1,
               bit2 : 1,
               bit3 : 1,
               bit4 : 1,
               bit5 : 1,
               bit6 : 1,
               bit7 : 1;
    } bits;
  } flags;

  byte payload_size;
};

extern bool coords_packet_received;
#endif
