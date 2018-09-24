#ifndef __STACK_H__
#define __STACK_H__

#include <stdlib.h>

#ifndef STACK_TYPE
#  include <stdint.h>
#  define STACK_TYPE uint32_t
#endif

typedef struct stack_
{
  STACK_TYPE *base;
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

void stack_push(stack *sp, STACK_TYPE elem)
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

STACK_TYPE stack_pop(stack *sp)
{
  if (!sp->base || sp->offset == 0) return -1;
  return *(sp->base + sp->offset-- - 1);
}

STACK_TYPE stack_top(stack *sp)
{
  return *(sp->base + sp->offset - 1);
}

#define stack_free(sp) free((sp)->base)
#define stack_empty(sp) ((sp)->offset == 0)

#endif /* __STACK_H__ */
