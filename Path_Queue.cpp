#include "Path_Queue.h"
// Adapted from https://www.tutorialspoint.com/data_structures_algorithms/queue_program_in_c.htm

#define QUEUE_MAX_SIZE 32

path_command pathArray[QUEUE_MAX_SIZE];
uint8_t front = 0;
uint8_t back = 0;
uint8_t itemCount = 0;

path_command getFront() {
  return pathArray[front];
}

bool isEmpty() {
  return itemCount == 0;
}

bool isFull() {
  return itemCount == QUEUE_MAX_SIZE;
}

uint8_t getSize() {
  return itemCount;
}

void enqueue(path_command pc) {
  if (!isFull()) {
    pathArray[++back %= QUEUE_MAX_SIZE] = pc;
    ++itemCount;
  }
}

path_command dequeue() {
  uint8_t old_front = front;
  ++front %= QUEUE_MAX_SIZE;
  --itemCount;
  return pathArray[old_front];
}
