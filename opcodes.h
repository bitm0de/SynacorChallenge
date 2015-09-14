#ifndef __OPCODES_H__
#define __OPCODES_H__

#include <stdio.h>
#include <stdint.h>

/* 32768-32775 (registers 0-7), >32775 (invalid number ranges)
 * <32768 (literals) */
#define NUMERIC_LIMIT 32768
#define INVALID_RANGE(v) ((v) > 32775)

uint16_t reg[8]; /* general purpose registers */
#define GET_REGISTER_PTR(v) &reg[((v) % NUMERIC_LIMIT)]

#define read_params(fp, params, num_params) fread(params, sizeof(*params), num_params, fp)

/* changes ptr to a register address if within the valid range, otherwise,
 * leaves the address as is
 * returns: non-zero if register, 0 otherwise */
int assign_ptr_register(uint16_t **ptr)
{
  if (**ptr < NUMERIC_LIMIT
      || INVALID_RANGE(**ptr)) return 0;
  *ptr = GET_REGISTER_PTR(**ptr);
  return 1;
}

void cleanup(FILE **fp)
{
  stack_free(&program_stack);
  program_stack.base = NULL;

  fclose(*fp);
  *fp = NULL;
}

/* stop execution and terminate program */
void halt_callback(FILE **fp)
{
  if (*fp) cleanup(fp);
  exit(0);
}

/* set: 1 a b
 * set register <a> to the value of <b> */
void set_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&ptr);
  *ptr = params[1];
}

/* push: 2 a
 * push <a> onto the stack */
void push_callback(FILE **fp)
{
  uint16_t a;
  read_params(*fp, &a, 1);
  stack_push(&program_stack, a);
}

/* pop: 3 a
 * remove the top element from the stack and write it into <a>; empty stack = error */
void pop_callback(FILE **fp)
{
  uint16_t a;
  uint16_t *ptr = &a;

  if (stack_empty(&program_stack))
  {
    fputs("ERROR: pop_callback() - stack is empty!\n", stderr);
    exit(1);
  }

  read_params(*fp, &a, 1);
  assign_ptr_register(&ptr);
  *ptr = stack_pop(&program_stack);
}

/* eq: 4 a b c
 * set <a> to 1 if <b> is equal to <c>; set it to 0 otherwise */
void eq_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = params[1] == params[2] ? 1 : 0;
}

/* gt: 5 a b c
 * set <a> to 1 if <b> is greater than <c>; set it to 0 otherwise */
void gt_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = params[1] > params[2] ? 1 : 0;
}

/* jmp: 6 a
 * jump to <a> */
void jmp_callback(FILE **fp)
{
  uint16_t a;
  read_params(*fp, &a, 1);
  fseek(*fp, a, SEEK_SET);
}

/* jt: 7 a b
 * if <a> is nonzero, jump to <b> */
void jt_callback(FILE **fp)
{
  uint16_t params[2];
  read_params(*fp, params, 2);
  if (params[0]) fseek(*fp, params[1], SEEK_SET);
}

/* jf: 8 a b
 * if <a> is zero, jump to <b> */
void jf_callback(FILE **fp)
{
  uint16_t params[2];
  read_params(*fp, params, 2);
  if (!params[0]) fseek(*fp, params[1], SEEK_SET);
}

/* add: 9 a b c
 * assign into <a> the sum of <b> and <c> (mod 32768) */
void add_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = (params[1] + params[2]) % NUMERIC_LIMIT;
}

/* mult: 10 a b c
 * store into <a> the product of <b> and <c> (mod 32768) */
void mult_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = (params[1] * params[2]) % NUMERIC_LIMIT;
}

/* mod: 11 a b c
 * store into <a> the remainder of <b> divided by <c> */
void mod_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = params[1] % params[2];
}

/* and: 12 a b c
 * stores into <a> the bitwise and of <b> and <c> */
void and_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = params[1] & params[2];
}

/* or: 13 a b c
 * stores into <a> the bitwise or of <b> and <c> */
void or_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&ptr);
  *ptr = params[1] | params[2];
}

/* not: 14 a b
 * stores 15-bit bitwise inverse of <b> in <a> */
void not_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&ptr);
  *ptr = ~(params[1] >> 1);
}

/* rmem: 15 a b
 * read memory at address <b> and write it to <a> */
void rmem_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&ptr);
  *ptr = *((uint16_t *)(virtual_heap + params[1]));
}

/* wmem: 16 a b
 * write the value from <b> into memory at address <a> */
void wmem_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *ptr = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&ptr);
  *((uint16_t *)(virtual_heap + params[1])) = *ptr;
}

/* call: 17 a
 * write the address of the next instruction to the stack and jump to <a> */
void call_callback(FILE **fp)
{
  uint16_t a;
  read_params(*fp, &a, 1);
  stack_push(&program_stack, ftell(*fp) + 2);
  fseek(*fp, a, SEEK_SET);
}

/* ret: 18
 * remove the top element from the stack and jump to it; empty stack = halt */
void ret_callback(FILE **fp)
{
  uint16_t ret_addr;
  if (stack_empty(&program_stack)) halt_callback(fp);
  ret_addr = stack_pop(&program_stack);
  fseek(*fp, ret_addr, SEEK_SET);
}

/* out: 19 a
 * write the character represented by ascii code <a> to the terminal */
void out_callback(FILE **fp)
{
  uint16_t a;
  read_params(*fp, &a, 1);
  putc(a, stdout);
}

/* in: 20 a
 * read a character from the terminal and write its ascii code to <a>;
 * it can be assumed that once input starts, it will continue until a
 * newline is encountered; this means that you can safely read whole
 * lines from the keyboard and trust that they will be fully read */
void in_callback(FILE **fp)
{
  uint16_t a;
  uint16_t *ptr = &a;
  read_params(*fp, &a, 1);
  assign_ptr_register((uint16_t **)&ptr);
  *ptr = a;
}

/* noop: 21
 * no operation */
void noop_callback(FILE **fp) {
  (void)fp; /* supress compiler warnings */
  return;
}

/* NOTE: 'FILE *' argument is only to retrieve the necessary opcode parameters from
 * the input file. Opcode itself should already be read as this determines which callback
 * function pointer is called initially */
void (*callback_ops[])(FILE **) = { halt_callback, set_callback, push_callback, pop_callback, eq_callback,
                                   gt_callback, jmp_callback, jt_callback, jf_callback, add_callback,
                                   mult_callback, mod_callback, and_callback, or_callback, not_callback,
                                   rmem_callback, wmem_callback, call_callback, ret_callback, out_callback,
                                   in_callback, noop_callback };

#endif /* __OPCODES_H__ */
