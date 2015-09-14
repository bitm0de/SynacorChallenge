#ifndef __SYNACOR_H__
#define __SYNACOR_H__

#define STACK_ALLOCATION_SIZE 1024
#define HEAP_ALLOCATION_SIZE 65535

unsigned char *virtual_heap;

#include "stack.h"
stack program_stack;

#include "opcodes.h"

#define init_heap_memory() virtual_heap = malloc(HEAP_ALLOCATION_SIZE)
#define init_stack() stack_init(&program_stack, STACK_ALLOCATION_SIZE)

#endif /* __SYNACOR_H__ */
