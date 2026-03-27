#include <stdint.h>
#ifndef CHAR_QUEUE_H
#define CHAR_QUEUE_H
#include <stdint.h>
#include <string.h>

#define CHAR_QUEUE_SIZE 128 // Define the size of the queue

typedef struct CharQueue_t {
    volatile uint16_t readCursor;
    volatile uint16_t writeCursor;
    volatile uint8_t data[CHAR_QUEUE_SIZE];
} CharQueue_t;

CharQueue_t CharQueueInit();
void CharQueueAdd(volatile CharQueue_t *, const uint8_t);
uint8_t CharQueueGet(volatile CharQueue_t *, uint16_t);
uint16_t CharQueueGetSize(volatile CharQueue_t *);
uint8_t CharQueueGetOffset(volatile CharQueue_t *, uint16_t);
uint8_t CharQueueNext(volatile CharQueue_t *);
void CharQueueRemoveLast(volatile CharQueue_t *);
void CharQueueReset(volatile CharQueue_t *);
uint16_t CharQueueSeek(volatile CharQueue_t *, const uint8_t);
#endif /* CHAR_QUEUE_H */
