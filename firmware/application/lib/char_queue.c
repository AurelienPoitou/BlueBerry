#include <stdint.h>
#include "char_queue.h"

CharQueue_t CharQueueInit()
{
    volatile CharQueue_t queue;
    CharQueueReset(&queue);
    return queue;
}

void CharQueueAdd(volatile CharQueue_t *queue, const uint8_t value)
{
    uint16_t size = CharQueueGetSize(queue);
    if (size < CHAR_QUEUE_SIZE) {
        queue->data[queue->writeCursor] = value;
        queue->writeCursor++;
        if (queue->writeCursor >= CHAR_QUEUE_SIZE) {
            queue->writeCursor = 0;
        }
    }
}

uint8_t CharQueueGet(volatile CharQueue_t *queue, const uint16_t idx)
{
    if (idx >= CHAR_QUEUE_SIZE) {
        return 0x00;
    }
    return queue->data[idx];
}

uint8_t CharQueueGetOffset(volatile CharQueue_t *queue, const uint16_t offset)
{
    uint16_t queueSize = CharQueueGetSize(queue);
    if (offset > queueSize) {
        return 0x00;
    }
    uint16_t offsetCursor = queue->readCursor + offset;
    if (offsetCursor >= CHAR_QUEUE_SIZE) {
        offsetCursor = offsetCursor - CHAR_QUEUE_SIZE;
    }
    return queue->data[offsetCursor];
}

uint16_t CharQueueGetSize(volatile CharQueue_t *queue)
{
    uint16_t queueSize = 0;
    uint16_t rCursor = queue->readCursor;
    uint16_t wCursor = queue->writeCursor;
    if (wCursor >= rCursor) {
        queueSize = wCursor - rCursor;
    } else {
        queueSize = (CHAR_QUEUE_SIZE - rCursor) + wCursor;
    }
    return queueSize;
}

uint8_t CharQueueNext(volatile CharQueue_t *queue)
{
    uint16_t size = CharQueueGetSize(queue);
    if (size <= 0) {
        return 0x00;
    }
    uint8_t data = queue->data[queue->readCursor];
    queue->data[queue->readCursor] = 0x00;
    queue->readCursor++;
    if (queue->readCursor >= CHAR_QUEUE_SIZE) {
        queue->readCursor = 0;
    }
    return data;
}

void CharQueueRemoveLast(volatile CharQueue_t *queue)
{
    if (CharQueueGetSize(queue) > 0) {
        queue->data[queue->writeCursor] = 0x00;
        if (queue->writeCursor == 0) {
            queue->writeCursor = CHAR_QUEUE_SIZE - 1;
        } else {
            queue->writeCursor--;
        }
    }
}

void CharQueueReset(volatile CharQueue_t *queue)
{
    queue->readCursor = 0;
    queue->writeCursor = 0;
    memset((void *) queue->data, 0, CHAR_QUEUE_SIZE);
}

uint16_t CharQueueSeek(volatile CharQueue_t *queue, const uint8_t needle)
{
    uint16_t readCursor = queue->readCursor;
    uint16_t size = CharQueueGetSize(queue);
    uint16_t cnt = 1;
    while (size > 0) {
        if (queue->data[readCursor] == needle) {
            return cnt;
        }
        readCursor++;
        if (readCursor >= CHAR_QUEUE_SIZE) {
            readCursor = 0;
        }
        cnt++;
        size--;
    }
    return 0;
}
