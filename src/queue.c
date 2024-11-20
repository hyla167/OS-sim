#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        /*
         * Increase the number of Processe(s) in queue by 1
         * Put in the queue the new process
         */
        if (q->size < MAX_QUEUE_SIZE) {
                q->proc[q->size] = proc;
                q->size++;
        }
        else {
                printf("Error: queue_t has already been full\n");
                exit(1);
        }
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (!q || empty(q)) {
                perror("There are no process in this queue");
                return NULL;
        }
        struct pcb_t * front = q->proc[0];
        for (int i = 0; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;
        return front;
}

