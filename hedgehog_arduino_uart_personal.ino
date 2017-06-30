/*
 * The circuit:
 * PIN 2 is Arduino's RX -- connect to hedgehog's TX
 * PIN 3 is Arduino's TX -- connect to hedgehog's RX 
 */

#include <SoftwareSerial.h>

#define RX_FROM_HEDGEHOG_PIN 2
#define TX_TO_HEDGEHOG_PIN 3

#define HEDGEHOG_BUF_SIZE 40
#define HEDGEHOG_CM_DATA_SIZE 0x10  // 16
#define HEDGEHOG_MM_DATA_SIZE 0x16  // 22
byte hedgehog_serial_buf[HEDGEHOG_BUF_SIZE];
byte hedgehog_serial_buf_ofs;

#define POSITION_DATAGRAM_CM_ID 0x0001
#define POSITION_DATAGRAM_MM_ID 0x0011
#define FROZEN_STATE_DATAGRAM_CM_ID 0x0002
#define FROZEN_STATE_DATAGRAM_MM_ID 0x0012
bool using_mm_mode;
unsigned int hedgehog_data_id;

typedef union {
  byte b[2];
  unsigned int w;
  int wi;
} uni_8x2_16;

typedef union {
  byte b[4];
  float f;
  unsigned long v32;
  long vi32;
} uni_8x4_32;

long hedgehog_x, hedgehog_y;  // coordinates of hedgehog (X,Y) in mm
long hedgehog_z;              // height of hedgehog (Z) in mm
bool hedgehog_pos_updated;    // flag of new data from hedgehog received

SoftwareSerial Shadow (RX_FROM_HEDGEHOG_PIN, TX_TO_HEDGEHOG_PIN);

//  Marvelmind hedgehog support initialize
void setup_hedgehog() {
  Shadow.begin(9600);  // begin SoftwareSerial transmission at 9.6 kbps
  Serial.print("Waiting to connect to hedgehog... ");
  while(!Shadow.available()) delay(500);
  Serial.println("done");
}

byte incoming_byte;
void loop_hedgehog() {
  while (Shadow.available()) {
    top:
    if (Shadow.read() != 0xFF) goto top;
    if (Shadow.read() != 0x47) goto top;
    
    hedgehog_data_id = Shadow.read() + (Shadow.read() << 8);
    switch (hedgehog_data_id) {
      default: goto top;
      case POSITION_DATAGRAM_CM_ID:
      case POSITION_DATAGRAM_MM_ID:
      case FROZEN_STATE_DATAGRAM_CM_ID:
      case FROZEN_STATE_DATAGRAM_MM_ID:;
    }
    
  }
}

void setup() {
  Serial.begin(57600);
  Serial.print("Establishing serial connection... ");
  while (!Serial) delay(500);
  Serial.println("done");

  setup_hedgehog();
}

void loop() {
  loop_hedgehog();
}
