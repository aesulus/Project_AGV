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
  Shadow.begin(9600);  // begin SoftwareSerial transmission at 9.6 kbps
  Serial.print(F("Waiting to connect to hedgehog... "));
  while(!Shadow.available()) delay(500);
  Serial.println(F("done"));
}

void setup() {
  Serial.begin(57600);
  Serial.println(F("Serial connection established."));
  CONSOLE("DEBUG MODE: ON\n");

  setup_hedgehog();
}

void loop() {
  read_hedgehog_stream();
}
