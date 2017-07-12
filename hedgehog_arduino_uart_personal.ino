/*
 * The circuit:
 * PIN 2 is Arduino's RX -- connect to hedgehog's TX
 * PIN 3 is Arduino's TX -- connect to hedgehog's RX 
 */

#include <SoftwareSerial.h>

#define RX_FROM_HEDGEHOG_PIN 2
#define TX_TO_HEDGEHOG_PIN 3

#define HEDGEHOG_BUF_SIZE 40        // Must be at least 6
#define HEDGEHOG_CM_DATA_SIZE 0x10  // 16
#define HEDGEHOG_MM_DATA_SIZE 0x16  // 22

struct {
  byte buf[HEDGEHOG_BUF_SIZE];
  byte buf_ofs;
  bool using_mm_mode;
  unsigned int data_id;
  int incoming_byte;
  byte payload_size;
  bool packet_received;
} hedgehog_state;

#define POSITION_DATAGRAM_CM_ID 0x0001
#define POSITION_DATAGRAM_MM_ID 0x0011
#define FROZEN_STATE_DATAGRAM_CM_ID 0x0002
#define FROZEN_STATE_DATAGRAM_MM_ID 0x0012

typedef union { // TODO: remove wi
  byte b[2];
  unsigned int w;
  int wi;
} uni_8x2_16;

typedef union { // TODO: remove f, vi32
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

  // initialize buffer offset
  hedgehog_state.buf_ofs = 0; // TODO: Test if moving this line to typedef works
  hedgehog_state.packet_received = 0;
}

void loop_hedgehog() {
  while (Shadow.available()) {
    hedgehog_state.incoming_byte = Shadow.read();  // read new byte from buffer
    Serial.print(hedgehog_state.incoming_byte, HEX);
    Serial.print(" #");
    Serial.print(hedgehog_state.buf_ofs);
    Serial.print(" ");

    switch (hedgehog_state.buf_ofs) {
      case 0:
        test_if_0xFF:
        hedgehog_state.buf_ofs = 0;
        if (hedgehog_state.incoming_byte != 0xFF) {
          continue;
        }
        break;
      case 1:
        if (hedgehog_state.incoming_byte != 0x47) goto test_if_0xFF;
        break;
      case 2: // We'll need the next byte too, so do nothing for now.
        break;
      case 3: // Concatenate data ID and test if valid
        hedgehog_state.data_id = hedgehog_state.buf[2] + (hedgehog_state.incoming_byte << 8);

        Serial.print("ID:");
        Serial.print(hedgehog_state.data_id, HEX);
        switch (hedgehog_state.data_id) {
          case POSITION_DATAGRAM_CM_ID:
          case POSITION_DATAGRAM_MM_ID:
          case FROZEN_STATE_DATAGRAM_CM_ID:
          case FROZEN_STATE_DATAGRAM_MM_ID:
            break;
          default:
            goto test_if_0xFF;
        }
        
        break;
      case 4: // Set Payload Size and test if valid for position datagrams.
        hedgehog_state.payload_size = hedgehog_state.incoming_byte;
        Serial.print("Payload size: ");
        Serial.println(hedgehog_state.payload_size);
        
        switch (hedgehog_state.data_id) {
          case POSITION_DATAGRAM_CM_ID:
            if (hedgehog_state.payload_size != HEDGEHOG_CM_DATA_SIZE) goto test_if_0xFF;                    // Test for proper size
            hedgehog_state.using_mm_mode = false;
            break;
          case POSITION_DATAGRAM_MM_ID:
            if (hedgehog_state.payload_size != HEDGEHOG_MM_DATA_SIZE) goto test_if_0xFF;                    // Test for proper size
            hedgehog_state.using_mm_mode = true;
            break;
          case FROZEN_STATE_DATAGRAM_CM_ID:
            hedgehog_state.using_mm_mode = false;
            break;
          case FROZEN_STATE_DATAGRAM_MM_ID:
            hedgehog_state.using_mm_mode = true;
        }
        
        break;
      case 5: // Test if Payload Size is valid for frozen state.
      
        switch (hedgehog_state.data_id) {
          case FROZEN_STATE_DATAGRAM_CM_ID:
            if (hedgehog_state.payload_size != 1 + hedgehog_state.incoming_byte * 8) goto test_if_0xFF;     // Test for proper size
            break;
          case FROZEN_STATE_DATAGRAM_MM_ID:
            if (hedgehog_state.payload_size != 1 + hedgehog_state.incoming_byte * 14) goto test_if_0xFF;    // Test for proper size
        }
        
        break;
      case HEDGEHOG_BUF_SIZE: // Buffer overflow handling
        hedgehog_state.buf_ofs = 0;
        goto exit_loop;
      default:
      // The size of a complete packet should be 7 bytes + payload size.
      // The offset of 6 bytes is used here instead of 7 because we are testing if this is the last byte.
        if (hedgehog_state.buf_ofs == 6 + hedgehog_state.payload_size) {  // Complete packet received!
          hedgehog_state.buf[hedgehog_state.buf_ofs] = hedgehog_state.incoming_byte;  // Push in last byte
          hedgehog_state.buf_ofs = 0;
          hedgehog_state.packet_received = true;  // Let's mark that.
          Serial.print("NL\n");
          goto exit_loop;
        }
    }

    hedgehog_state.buf[hedgehog_state.buf_ofs++] = hedgehog_state.incoming_byte;
    Serial.print("\n");
  }

  exit_loop:
  if (hedgehog_state.packet_received) {
    hedgehog_set_crc16(&hedgehog_state.buf[0], 7 + hedgehog_state.payload_size);
    // Reset packet received flag
    hedgehog_state.packet_received = false;

    // TODO: Write if block testing CRC-16
  }
}

// Calculate CRC-16 of hedgehog packet
void hedgehog_set_crc16(byte *buf, byte size)
{uni_8x2_16 sum;
 byte shift_cnt;
 byte byte_cnt;

  sum.w=0xffffU;

  for(byte_cnt=size; byte_cnt>0; byte_cnt--)
   {
   sum.w=(unsigned int) ((sum.w/256U)*256U + ((sum.w%256U)^(buf[size-byte_cnt])));

     for(shift_cnt=0; shift_cnt<8; shift_cnt++)
       {
         if((sum.w&0x1)==1) sum.w=(unsigned int)((sum.w>>1)^0xa001U);
                       else sum.w>>=1;
       }
   }
   
  buf[size]=sum.b[0];
  buf[size+1]=sum.b[1];// little endian
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
