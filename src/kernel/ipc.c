#include "kernel.h"

MessageQueue* mq_create(int capacity) {
    MessageQueue* mq = (MessageQueue*)malloc(sizeof(MessageQueue));
    if (!mq) return NULL;
    
    mq->capacity = capacity;
    mq->messages = (Message*)malloc(capacity * sizeof(Message));
    if (!mq->messages) {
        free(mq);
        return NULL;
    }
    
    mq->head = 0;
    mq->tail = 0;
    mq->count = 0;
    
    sem_init(&mq->mutex, 0, 1);
    sem_init(&mq->slots, 0, capacity);
    sem_init(&mq->items, 0, 0);
    
    return mq;
}

void mq_destroy(MessageQueue* mq) {
    if (mq) {
        sem_destroy(&mq->mutex);
        sem_destroy(&mq->slots);
        sem_destroy(&mq->items);
        free(mq->messages);
        free(mq);
    }
}

int mq_send(MessageQueue* mq, Message* msg, int timeout) {
    if (sem_wait(&mq->slots) != 0) return -1;
    if (sem_wait(&mq->mutex) != 0) return -1;
    
    // Copy message to queue
    mq->messages[mq->tail] = *msg;
    mq->tail = (mq->tail + 1) % mq->capacity;
    mq->count++;
    
    sem_post(&mq->mutex);
    sem_post(&mq->items);
    
    return 0;
}

int mq_receive(MessageQueue* mq, Message* msg, int timeout) {
    if (sem_wait(&mq->items) != 0) return -1;
    if (sem_wait(&mq->mutex) != 0) return -1;
    
    // Get message from queue
    *msg = mq->messages[mq->head];
    mq->head = (mq->head + 1) % mq->capacity;
    mq->count--;
    
    sem_post(&mq->mutex);
    sem_post(&mq->slots);
    
    return 0;
}