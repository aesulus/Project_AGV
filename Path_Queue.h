#ifndef PATH_QUEUE_H
#define PATH_QUEUE_H
#include <stdint.h>

struct path_command {
  uint8_t movement_type;
  int16_t param1;
  int16_t param2;
};

path_command getFront();
bool isEmpty();
bool isFull();
uint8_t getSize();
void enqueue(path_command pc);
path_command dequeue();
#endif
