#include "Stack.h"

#include <stdlib.h>

typedef struct StackNode StackNode;
struct StackNode {
    StackNode* next;
    void* data;
};

struct Stack {
    StackNode* head;
    unsigned int count;
};

Stack* createStack() {
    Stack* stack = (Stack*)malloc(sizeof(Stack));
    stack->head = NULL;
    stack->count = 0;
}

void freeStack(Stack* stack) {
    while (stack->head != NULL) {
        StackNode* toFree = stack->head;
        stack->head = stack->head->next;
        free(toFree);
    }

    free(stack);
}

void pushOntoStack(Stack* stack, void* data) {
    StackNode* newNode = (StackNode*)malloc(sizeof(StackNode));
    newNode->next = stack->head;
    newNode->data = data;

    stack->head = newNode;
    stack->count++;
}

void* popOffOfStack(Stack* stack) {
    void* output;
    StackNode* toFree;

    if (stack->head == NULL) return NULL;

    output = stack->head->data;
    toFree = stack->head;

    stack->head = stack->head->next;
    stack->count--;

    free(toFree);

    return output;
}

unsigned int getStackCount(Stack* stack) {
    return stack->count;
}
