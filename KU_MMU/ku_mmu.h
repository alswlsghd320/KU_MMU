#pragma once

#include <stdio.h>
#include <stdlib.h>

#define KU_MMU_PDMASK 192 //11000000
#define KU_MMU_PDSHIFT 6
#define KU_MMU_PMDMASK 48 //00110000
#define KU_MMU_PMDSHIFT 4
#define KU_MMU_PTMASK 12 //00001100
#define KU_MMU_PTSHIFT 2
#define KU_MMU_PMASK 3 //00000011

typedef struct ku_pte {
    unsigned char pte;
}ku_pte;

typedef struct PDBR {
    char pid;
    ku_pte *pdbr;
    struct PDBR *next;
}PDBR;

typedef struct Node {
    char pid;
    char va;
    struct Node *next;
}Node;

typedef struct Queue {
    Node *front;
    Node *rear;
}Queue;

//Delcare variables for SIZE, MEMORY, PDBR, SWAP
unsigned int ku_mmu_mem_size;
unsigned int ku_mmu_swap_size;

ku_pte *ku_mmu_pmem = NULL; //physical memory
char *ku_mmu_s_space = NULL; //swap space
char *ku_mmu_free_p = NULL; //freelist

PDBR *ku_mmu_pdbr = NULL;
Queue *ku_mmu_queue = NULL;

//Setting about PDBR
PDBR *create_node(char pid, ku_pte *pdbr) {
    PDBR *new = (PDBR *)malloc(sizeof(PDBR));
    new->pid = pid;
    new->pdbr = pdbr;
    new->next = NULL;

    return new;
}
void add_PDBR(char pid, ku_pte *pdbr) {
    PDBR *new = create_node(pid, pdbr);
    PDBR *temp = ku_mmu_pdbr;

    if(temp == NULL) ku_mmu_pdbr = new;
    else {
        while(temp->next != NULL) temp = temp->next;
        temp->next = new;
    } 
}

//Setting about Queue
void InitQueue() {
   ku_mmu_queue = (Queue *)malloc(sizeof(Queue));
   ku_mmu_queue->front = ku_mmu_queue->rear = NULL;
}

int Isempty() { return ku_mmu_queue->front == NULL; }

void Enqueue(char pid, char va) {
    Node *new = (Node *)malloc(sizeof(Node));
    new->pid = pid;
    new->va = va;
    new->next = NULL;

    if(Isempty()) ku_mmu_queue->front = new;
    else ku_mmu_queue->rear->next = new;
    
    ku_mmu_queue->rear = new;
}

Node *Dequeue() {
    Node *now = (Node *)malloc(sizeof(Node));
    if(Isempty()) return NULL;
    now = ku_mmu_queue->front;
    ku_mmu_queue->front->next = now->next;
    now->next = NULL;

    return now;
}

//Get pdbr for pid
ku_pte *get_pdbr(char pid) {
    ku_pte *now = NULL;

    PDBR *temp = ku_mmu_pdbr;
    while(temp != NULL) {
        if(temp->pid == pid) {
            now = temp->pdbr;
            break;
        }
        temp = temp->next;
    }
    return now;
}

int check_full() {//check every element of free_p and s_space is 1
    int i;
    for(i = 0; i < ku_mmu_mem_size / 4; i++) {
        if(ku_mmu_free_p[i] == 0) return 0; //there exists 0 in free_p
    }
    for(i = 0; i < ku_mmu_swap_size / 4; i++) {
        if(ku_mmu_s_space == 0) return 0; //there exist 0 in s_space
    }

    return 1;
}

char allocate_pfn() {
    if(check_full()) return 0; //fail
    //1) find index(=pfn) for 0 in free_p
    int i;
    char s_index; //s_index < 2^7
    for (i = 1; i < ku_mmu_mem_size / 4; i++) {
        if(ku_mmu_free_p[i] == 0) {
            ku_mmu_free_p[i] = 1;
            return i;
        }
    }

    //2) There is no 0 in free_p. So need to swap-out
    Node *d = Dequeue();
    ku_pte *pdbr = get_pdbr(d->pid);
    char v0 = (d->va & KU_MMU_PDMASK) >> KU_MMU_PDSHIFT;
    char v1 = (d->va & KU_MMU_PMDMASK) >> KU_MMU_PMDSHIFT;
    char v2 = (d->va & KU_MMU_PTMASK) >> KU_MMU_PTSHIFT;

    for(i = 1; i < ku_mmu_swap_size / 4; i++) { //swap-out
        if(ku_mmu_s_space[i] == 0) {
            s_index = i;
            ku_mmu_s_space[i] = 1;
            break;
        }
    }

    char pmd_pfn = ((pdbr + v0)->pte) >> 2;
    char pt_pfn = ku_mmu_pmem[4 * pmd_pfn + v1].pte >> 2;
    char p_pfn = ku_mmu_pmem[4 * pt_pfn + v2].pte >> 2;
    (&ku_mmu_pmem[4 * pt_pfn + v2])->pte = s_index << 1;
    
    return p_pfn;
}

void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size) {
    ku_mmu_mem_size = mem_size;
    ku_mmu_swap_size = swap_size;
    
    ku_mmu_pmem = (ku_pte *)calloc(mem_size, sizeof(ku_pte));
    if (ku_mmu_pmem == NULL) return 0;
    for(int i = 0; i < 4; i++) ku_mmu_pmem[i].pte = 1; //OS

    ku_mmu_s_space = (char *)calloc(swap_size / 4, sizeof(char));
    if (ku_mmu_s_space == NULL) return 0;

    ku_mmu_free_p = (char *)calloc(mem_size / 4, sizeof(char));
    if (ku_mmu_free_p == NULL) return 0;
    ku_mmu_free_p[0] = 1;

    ku_mmu_pdbr = NULL;

    InitQueue();
    if(ku_mmu_queue == NULL) return 0;
    
    return ku_mmu_pmem;
}

int ku_run_proc(char pid, void **ku_cr3) {
    //0. Type Casting
    ku_pte *ku_cr;
    
    //1. Checkfulls
    if(check_full()) return -1;

    //2. get pdbr for pid
    ku_cr = get_pdbr(pid);

    //3.If there is no pdbr for given pid, create new PDBR
    if(ku_cr == NULL) {
        char pfn = allocate_pfn();
        ku_cr = (ku_pte *)malloc(sizeof(ku_pte));
        ku_cr = &ku_mmu_pmem[4*pfn];
                
        add_PDBR(pid, ku_cr);
    }

    *(ku_pte **)ku_cr3 = ku_cr; //Type casting
    
    return 0; //success
}

int ku_page_fault(char pid, char va) {
    if(check_full()) return -1; // fail

    ku_pte *pdbr = get_pdbr(pid); //Solve in case 1)
    char pmd_pfn, pt_pfn, p_pfn;
    char v0 = (va & KU_MMU_PDMASK) >> KU_MMU_PDSHIFT;
    char v1 = (va & KU_MMU_PMDMASK) >> KU_MMU_PMDSHIFT;
    char v2 = (va & KU_MMU_PTMASK) >> KU_MMU_PTSHIFT;

    if ((pdbr + v0)->pte == 0x0) {//If pte == 00000000 when access PMD in PD
        pmd_pfn = allocate_pfn();
        (pdbr + v0)->pte = (pmd_pfn << 2) | 0x1;
    }
    else pmd_pfn = ((pdbr + v0)-> pte) >> 2;

    if (ku_mmu_pmem[4 * pmd_pfn + v1].pte == 0x0) {
        pt_pfn = allocate_pfn();
        ku_mmu_pmem[4 * pmd_pfn + v1].pte = (pt_pfn << 2) | 0x1;
    }
    else pt_pfn = ku_mmu_pmem[4 * pmd_pfn + v1].pte >> 2;

    if (ku_mmu_pmem[4 * pt_pfn + v2].pte == 0x0) {
        p_pfn = allocate_pfn();
        ku_mmu_pmem[4 * pt_pfn + v2].pte = (p_pfn << 2) | 0x1;
        Enqueue(pid, va);
        return 0; //success
    }

    else if (ku_mmu_pmem[4 * pt_pfn + v2].pte & 0x1 == 0) { //page is swapped-out
        char s_index = ku_mmu_pmem[4 * pt_pfn + v2].pte >> 1;
        ku_mmu_s_space[s_index] = 0; // reset for swap space

        p_pfn = allocate_pfn();
        (&ku_mmu_pmem[4 * pt_pfn + v2])->pte = (p_pfn << 2) | 0x1;
        Enqueue(pid, va);
        return 0; //success
    }

    else if (ku_mmu_pmem[4 * pt_pfn + v2].pte & 0x1 == 1) {
        return -1;//fail
        //since we don't allocate given page, we call ku_page_fault
        //But, page is allocated successfully in this case
        //So, there is an error; Therefore fail
    }
    return -1;
}
