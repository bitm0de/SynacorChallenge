#ifndef __SYNACOR_H__
#define __SYNACOR_H__

#define STACK_ALLOCATION_SIZE 1024
#define HEAP_ALLOCATION_SIZE 65535

uint16_t *virtual_heap;

/* reads file contents into virtual_heap buffer.
 * returns: total bytes read, or -1 in case of an error 
 * ----------------------------------------------------- */
size_t init_alloc_program_mem(const char *file)
{
  FILE *fp = NULL;
  long size;
  size_t bytes_read;

  if (!(fp = fopen(file, "rb+"))) return 0;
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);

  virtual_heap = malloc(size);
  bytes_read = fread(virtual_heap, sizeof(*virtual_heap), size / 2, fp);

  fclose(fp);
  return bytes_read;
}

#include "stack.h"
stack program_stack;

#include "opcodes.h"

#define init_heap_memory() virtual_heap = malloc(HEAP_ALLOCATION_SIZE)
#define init_stack() stack_init(&program_stack, STACK_ALLOCATION_SIZE)

#endif /* __SYNACOR_H__ */
