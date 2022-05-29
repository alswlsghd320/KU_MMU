#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ku_cfs.h"

#define DeltaExecTime 1

const float weight_value[5] = {0.64, 0.8, 1, 1.25, 1.5625};
short use_scheduling = 0;//Whether to use or not scheduling (0 : don't use, others : use)

int main(int argc, char *argv[]) {
    //0. Initialize Values
    struct sigaction sa;
    struct itimerval timer;
    int ts = atoi(argv[6]);
    int ts_count = 0, sum = 0, num_process, i, j;
    char ch = 'A';
    //num_process : the number of processes with same nice value(i.e. argv values)
    //sum = n1 + n2 + n3 + n4 + n5
    readyQueue *queue = (readyQueue *)malloc(sizeof(queue));
    Node *dequeuing_node = (Node *)malloc(sizeof(Node));
    init(queue);

    //1. fork and exec children process
    for(i = 0; i < 5; i++) {
        num_process = atoi(argv[i+1]);
        sum += num_process;

        for(j = 0; j < num_process; j++) {
            enqueue(queue, CreateNode(i-2, &ch));
            ch++;
        }
    }
    sleep(5);//parent process intentionlly fall asleep for children process to be executed sufficiently

    //2. ITimer Setting
    sa.sa_handler = &timer_interrupt;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;

    //3. START SETITIMER
    setitimer(ITIMER_REAL, &timer, NULL);

    kill(queue->tail->pid, SIGCONT);

    while(1){
        if(use_scheduling == 1) {
            ts_count++;
            use_scheduling--;
            if (ts_count >= ts) break;
            KU_Scheduler(queue);
        }
    }

    return 0;
}

Node* CreateNode(short input_nice, const char *ch) {
    Node *new_Node = (Node*)malloc(sizeof(Node)); // create Node
    memset(new_Node, 0x0, sizeof(Node));

    new_Node->pid = fork();
    new_Node->nice = input_nice;
    new_Node->vrun_t = 0;
    new_Node->next = NULL;

    if(new_Node->pid < 0) exit(1);
    else if(new_Node->pid == 0) execl("./ku_app", "-v", ch, (char *)NULL);

    return new_Node;
}

void enqueue(readyQueue *queue, Node *new_Node) { //vruntime of root node is the smallest!!
    Node *temp = (Node *)malloc(sizeof(Node));
    Node *temp_prev = (Node *)malloc(sizeof(Node));

    if(new_Node == NULL) return;
    if(Isempty(queue)) queue->head = queue->tail = new_Node;
    else if(new_Node->vrun_t < queue->tail->vrun_t) {
        queue->tail->next = new_Node;
        queue->tail = new_Node;
    }
    else {
        temp_prev = temp = queue->head;
        while(temp != NULL) {
            if(new_Node->vrun_t >= temp->vrun_t) {
                break;
            }
            temp_prev = temp;
            temp = temp->next;
        }

        new_Node->next = temp;
        if (temp == temp_prev) queue->head = new_Node;
        else temp_prev->next = new_Node;
    }
}

void dequeue(readyQueue *queue, Node **saving_node) {
    Node *temp = queue->head;
    *saving_node = queue->tail;

    if(Isempty(queue)) exit(1);
    else if(temp->next == NULL) queue->head = queue->tail = NULL;  //has only one queue

    else {
        while(temp->next != queue->tail) temp = temp->next; //move temp before tail
        queue->tail = temp;
        temp->next = NULL;
    }
}

void timer_interrupt(int signum) {
    use_scheduling++;
}

void KU_Scheduler(readyQueue *queue) { //wc
    Node* dequeuing_node = (Node *)malloc(sizeof(Node));
    kill(queue->tail->pid, SIGSTOP);

    queue->tail->vrun_t += DeltaExecTime * weight_value[queue->tail->nice + 2];

    dequeue(queue, &dequeuing_node);
    enqueue(queue, dequeuing_node);

    kill(queue->tail->pid, SIGCONT); //after finishing scheduling, running process newly
}


