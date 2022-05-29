#pragma once

#include <unistd.h>

typedef struct _Node {
    pid_t pid; // PID of Process
    short nice; // Nice value of Process
    double vrun_t; // Vruntime for process
    struct _Node *next;
}Node;

Node* CreateNode(short input_nice, const char *ch);


typedef struct _readyQueue {
    Node *head, *tail;
} readyQueue;

void init(readyQueue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
}

int Isempty(readyQueue *queue) {return queue->head == NULL;}
void enqueue(readyQueue *queue, Node *new_Node);
void dequeue(readyQueue *queue, Node **saving_node);

void timer_interrupt(int signum);
void KU_Scheduler(readyQueue *queue);



