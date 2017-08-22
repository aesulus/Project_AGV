/*
 * The circuit:
 * PIN 2 is Arduino's RX -- connect to hedgehog's TX
 * PIN 3 is Arduino's TX -- connect to hedgehog's RX 
 */

#include "metal_sonic.h"
#include <SoftwareSerial.h>

#define RX_FROM_HEDGEHOG_PIN 2
#define TX_TO_HEDGEHOG_PIN 3

extern struct hedgehog_state_type hedgehog_state;

SoftwareSerial Shadow (RX_FROM_HEDGEHOG_PIN, TX_TO_HEDGEHOG_PIN);

void read_hedgehog_stream() {
  while (int Shadow_available = Shadow.available()) {
    int incoming_byte = Shadow.read();  // read new byte from buffer
    Serial.print(incoming_byte, HEX);
    Serial.print(" ");
    
    switch (hedgehog_state.current_state(incoming_byte)) {
      case START:
        hedgehog_state.buf_ofs = 0;
        hedgehog_state.crc16_sum = 0xffffU;
        hedgehog_state.current_state = receiving_1;
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
    }
  }
}

//  Marvelmind hedgehog support initialize
void setup_hedgehog() {
  Shadow.begin(9600);  // begin SoftwareSerial transmission at 9.6 kbps
  Serial.print(F("Waiting to connect to hedgehog... "));
  while(!Shadow.available()) delay(500);
  Serial.println(F("done"));

  // initialization stuff (if any) goes here:
  //hedgehog_state.current_state = receiving_1;
}

void setup() {
  Serial.begin(57600);
  Serial.print(F("Serial connection established."));

  setup_hedgehog();
}

void loop() {
  read_hedgehog_stream();

  // if (hedgehog_state.CRC_16_success && !hedgehog_state.frozen_state) {
    // hedgehog_state.CRC_16_success = false; // Reset flag
    
    // if (hedgehog_state.flags.bits.bit0 == 0) { // Coordinate handling
      // Serial.print( "X=");
      // Serial.print(hedgehog_x);
      // Serial.print(" Y=");
      // Serial.print(hedgehog_y);
      // Serial.print(" Z=");
      // Serial.println(hedgehog_z);
    // }

    // // if (hedgehog_state.flags.bits.bit3 == 1) { // Write handling
      // // Serial.println("Writing Start!\nWriting Start!\nWriting Start!\nWriting Start!\nWriting Start!");
      // // byte reply_packet[11];
      // // reply_packet[0] = hedgehog_state.buf[hedgehog_state.payload_size];  // Address of hedgehog
      // // //reply_packet[1] = 0x48; // Type of packet
      // // reply_packet[2] = 0x00;
      // // reply_packet[3] = 0x01;
      // // reply_packet[4] = 0x04;
      // // reply_packet[5] = 0b00000001;
      // // /* reply_packet[6] = 0x00;
         // // reply_packet[7] = 0x00;
         // // reply_packet[8] = 0x00; */
      // // uni_8x2_16 uni16;
      // // uni16.w = calc_crc16(&reply_packet[0], 9);
      // // reply_packet[9] = uni16.b[0];
      // // reply_packet[10] = uni16.b[1];

      // // for (int i = 0; i < 11; ++i) {
        // // Shadow.write(reply_packet[i]);
      // // }
    // // }
  // }
}
