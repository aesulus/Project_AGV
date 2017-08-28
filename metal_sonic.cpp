#include "metal_sonic.h"
struct hedgehog_state_type hedgehog_state;

long hedgehog_x, hedgehog_y;  // coordinates of hedgehog (X,Y) in mm
long hedgehog_z;              // height of hedgehog (Z) in mm

state_result receiving_1(int incoming_byte) {
  CONSOLE("receiving_1|");
  if (incoming_byte == 0xff) {
    hedgehog_state.current_state = receiving_2;
    return NEXT;
  }
  return START;
}

state_result receiving_2(int incoming_byte) {
  switch (incoming_byte) {
    case 0x47:
      hedgehog_state.current_state = streaming_3;
      return NEXT;
    case 0x49:
      hedgehog_state.current_state = reading_3;
      return NEXT;
    case 0x4a:
      hedgehog_state.current_state = writing_3;
      return NEXT;
    default: return START;
  }
}

state_result streaming_3(int incoming_byte) {
  switch (incoming_byte) {
    case 0x11:
      hedgehog_state.current_state = mm_coordinates_4;
      return NEXT;
    case 0x01:  // Don't wanna
    case 0x02:  // deal with these cases
    case 0x12:  // just yet.
    default: return START;
  }
}

state_result cm_coordinates_4(int incoming_byte) {} // TODO

state_result mm_coordinates_4(int incoming_byte) {
  if (incoming_byte == 0x00) {
    hedgehog_state.current_state = mm_coordinates_5;
    return NEXT;
  }
  return START;
}

state_result mm_coordinates_5(int incoming_byte) {
  if (incoming_byte == 0x16) {  // This is the length of the upcoming payload
    hedgehog_state.current_state = mm_coordinates_6;
    return NEXT;
  }
  return START;
}

state_result mm_coordinates_6(int incoming_byte) {
  // Since the payload is of fixed size,
  // we know that the last byte of the payload will be when buf_ofs == 26;
  // thus, only then will transition to the next state be allowed.
  // However, since we know the next 2 inputs are variable CRC-16 values,
  // we can "skip" straight to the END
  // We compensate for this by allowing the transition to occur when buf_ofs == 28
  CONSOLE("mm_coordinates_6|");
  CONSOLE(hedgehog_state.buf_ofs);
  CONSOLE("|");
  if (hedgehog_state.buf_ofs == 28) {
    hedgehog_state.current_state = mm_coordinates_END;
    return END;
  }
  return NEXT;
}

state_result mm_coordinates_END(int incoming_byte) {
  hedgehog_state.flags.uni8 = hedgehog_state.buf[21];
  
  if (hedgehog_state.flags.bits.bit0 == 0) {  // If bit 0 is 1, coordinate data should not be used.
    uni_8x4_32 uni32;

    uni32.b[0] = hedgehog_state.buf[9];
    uni32.b[1] = hedgehog_state.buf[10];
    uni32.b[2] = hedgehog_state.buf[11];
    uni32.b[3] = hedgehog_state.buf[12];
    hedgehog_x = uni32.v32;

    uni32.b[0] = hedgehog_state.buf[13];
    uni32.b[1] = hedgehog_state.buf[14];
    uni32.b[2] = hedgehog_state.buf[15];
    uni32.b[3] = hedgehog_state.buf[16];
    hedgehog_y = uni32.v32;

    uni32.b[0] = hedgehog_state.buf[17];
    uni32.b[1] = hedgehog_state.buf[18];
    uni32.b[2] = hedgehog_state.buf[19];
    uni32.b[3] = hedgehog_state.buf[20];
    hedgehog_z = uni32.v32;
	
    Serial.print( "X=");
    Serial.print(hedgehog_x);
    Serial.print(" Y=");
    Serial.print(hedgehog_y);
    Serial.print(" Z=");
    Serial.println(hedgehog_z);
  }
  return 0;
}

state_result cm_frozen_4(int incoming_byte) {} // TODO

state_result mm_frozen_4(int incoming_byte) {} // TODO
state_result reading_3(int incoming_byte) {} // TODO\

state_result writing_3(int incoming_byte) {
  if (incoming_byte == 0x01) {
    hedgehog_state.current_state = path_4;
    return NEXT;
  }
  return START;
}

state_result path_4(int incoming_byte) {
  if (incoming_byte == 0x02) {
    hedgehog_state.current_state = path_5;
    return NEXT;
  }
  return START;
}

state_result path_5(int incoming_byte) {
  if (incoming_byte == 0x0c) {  // This is the length of the upcoming payload
    hedgehog_state.current_state = path_6;
    return NEXT;
  }
  return START;
}

state_result path_6(int incoming_byte) {
   // This includes the 12 byte payload packet and the two following CRC-16 bytes
  if (hedgehog_state.buf_ofs == 18) {
    hedgehog_state.current_state = path_END;
    return END;
  }
  return NEXT;
}

state_result path_END(int incoming_byte) {
  // We're going to ignore it if it's 5 for now.
  if (hedgehog_state.buf[5] == 5) {
    path_command current_path;
    current_path.movement_type = hedgehog_state.buf[5];
    current_path.param1 = hedgehog_state.buf[8] + hedgehog_state.buf[9] << 8;
    current_path.param2 = hedgehog_state.buf[10] + hedgehog_state.buf[11] << 8;
    enqueue(current_path);
  }
  return 0;
}

void reset() {
  hedgehog_state.buf_ofs = 0;
  hedgehog_state.crc16_sum = 0xffffU;
  hedgehog_state.current_state = receiving_1;
}

uint16_t crc16_streaming_advance(uint16_t sum, byte b) {
    sum ^= b;
    for (byte shift_cnt = 0; shift_cnt < 8; ++shift_cnt) {
        if (sum & 1) sum = (sum >> 1) ^ 0xa001U;
        else sum >>= 1;
    }
    return sum;
}
