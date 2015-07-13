#ifndef _STACK_H_
#define _STACK_H_

typedef struct Stack Stack;
struct Stack;

Stack* createStack();

void freeStack(Stack* stack);

void pushOntoStack(Stack* stack, void* data);

void* popOffOfStack(Stack* stack);

unsigned int getStackCount(Stack* stack);

#endif
