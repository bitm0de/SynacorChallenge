#ifndef __STACK_H__
#define __STACK_H__

#include <stdlib.h>
#include <stdint.h>

typedef struct stack_
{
  uint16_t *base;
  size_t offset;
  size_t num_elems;
  unsigned char padding[4];
} stack;

void *stack_init(stack *sp, size_t num_elems)
{
  sp->offset = 0;
  sp->num_elems = num_elems;
  return (sp->base = malloc(num_elems * sizeof(*sp->base)));
}

void stack_push(stack *sp, uint16_t elem)
{
  if (!sp->base || sp->offset + 1 >= sp->num_elems)
  {
    /* allocate more memory - realloc() * 2 */
    sp->num_elems *= 2;
    sp->base = realloc(sp->base, sp->num_elems * sizeof(*sp->base));
  }
  *(sp->base + sp->offset) = elem;
  ++sp->offset;
}

uint16_t stack_pop(stack *sp)
{
  if (!sp->base || sp->offset == 0) return -1;
  return *(sp->base + sp->offset-- - 1);
}

uint16_t stack_top(stack *sp)
{
  return *(sp->base + sp->offset - 1);
}

#define stack_free(sp) free((sp)->base)
#define stack_empty(sp) ((sp)->offset == 0)

#endif /* __STACK_H__ */
