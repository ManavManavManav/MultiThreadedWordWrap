#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <libgen.h>


typedef struct Node {
    char* file;
    struct Node* next;
} Node;

typedef struct Queue {
    struct Node* head;
    struct Node* tail;
} Queue;

Queue enqueue(Queue queue, char* file) {
    Node* new_node = malloc(sizeof *new_node);
    new_node->file = file;
    new_node->next = NULL;

    if (queue.head == NULL && queue.tail == NULL) {
      queue.head = new_node;
      queue.tail = new_node;
      queue.head->next = queue.tail;
    } else {
      queue.tail->next = new_node;
      queue.tail = new_node;
    }

    return queue;
}

// Remove a QueueNode from the front of the Queue
char* dequeue(Queue* queue) {
    if (queue->head == queue->tail) {
      queue->tail = NULL;
    }

    if (queue->head) {
        Node* to_remove = queue->head;
        char* file = to_remove->file;
        if (queue->head == queue->head->next) {
            queue->head = NULL;
        } else {
            queue->head = queue->head->next;
        }

        free(to_remove);
        return file;
    }

    // puts("Error! dequeue called when there is no head.");
    return NULL;
}

void free_queue(Queue* queue) {
    while (queue && queue->head) {
        dequeue(queue);
    }
}

bool is_empty(Queue queue) {
    return queue.head == NULL;
}

char* peek(Queue queue) {
    if (queue.head) {
        return queue.head->file;
    }

    // puts("Error! peek() called on empty queue.");
    return NULL;
}
