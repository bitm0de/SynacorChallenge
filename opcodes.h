#ifndef __OPCODES_H__
#define __OPCODES_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* 32768-32775 (registers 0-7), >32775 (invalid number ranges)
 * <32768 (literals) */
#define NUMERIC_LIMIT 32768
#define INVALID_RANGE(v) ((v) > 32775)

uint16_t reg[8]; /* general purpose registers */

#define GET_REGISTER_INDEX(v) ((v) % NUMERIC_LIMIT)
#define GET_REGISTER_PTR(v) &reg[GET_REGISTER_INDEX(v)]

#define MAX_PARAMS 3
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

#define NUM_OPCODES 22
#define VALID_OPCODE(opcode) ((unsigned)(opcode) < NUM_OPCODES)

#define OPCODE_HALT  0
#define OPCODE_SET   1
#define OPCODE_PUSH  2
#define OPCODE_POP   3
#define OPCODE_EQ    4
#define OPCODE_GT    5
#define OPCODE_JMP   6
#define OPCODE_JT    7
#define OPCODE_JF    8
#define OPCODE_ADD   9
#define OPCODE_MULT  10
#define OPCODE_MOD   11
#define OPCODE_AND   12
#define OPCODE_OR    13
#define OPCODE_NOT   14
#define OPCODE_RMEM  15
#define OPCODE_WMEM  16
#define OPCODE_CALL  17
#define OPCODE_RET   18
#define OPCODE_OUT   19
#define OPCODE_IN    20
#define OPCODE_NOOP  21

const char *opcode_alias[NUM_OPCODES] = {
  "halt", "set", "push", "pop", "eq", "gt", "jmp",
  "jt", "jf", "add", "mult", "mod", "and", "or", "not",
  "rmem", "wmem", "call", "ret", "out", "in", "noop"
};

const int opcode_param_count[NUM_OPCODES] = {
  0, 2, 1, 1, 3, 3, 1, 2, 2, 3, 3,
  3, 3, 3, 2, 2, 2, 1, 0, 1, 1, 0
};

void write_instruction(uint16_t opcode, FILE **fp)
{
  int i = 0, param_count;
  uint16_t params[MAX_PARAMS];
  uint16_t *param0 = &params[0];

  if (!VALID_OPCODE(opcode))
  {
    printf("-- INVALID OPCODE [%u] --\n", opcode);
    return;
  }

  printf("%s\t\t", opcode_alias[opcode]);
  if (!(param_count = opcode_param_count[opcode]))
  {
    putc('\n', stdout);
    return;
  }
  
  read_params(*fp, params, param_count);

  if (assign_ptr_register(&param0))
  {
    printf("reg%u ", GET_REGISTER_INDEX(params[i]));
  }
  else
  {
    printf("%u ", params[i]);
    if (opcode == OPCODE_OUT)
      printf("[%c] ", params[i]);
  }

  ++i;

  for (;i < param_count; ++i)
    fprintf(stdout, "%u ", params[i]);

  putc('\n', stdout);
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
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&param0);
  *param0 = params[1];
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
  uint16_t *param0 = &a;

  if (stack_empty(&program_stack))
  {
    fputs("ERROR: pop_callback() - stack is empty!\n", stderr);
    exit(1);
  }

  read_params(*fp, &a, 1);
  assign_ptr_register(&param0);
  *param0 = stack_pop(&program_stack);
}

/* eq: 4 a b c
 * set <a> to 1 if <b> is equal to <c>; set it to 0 otherwise */
void eq_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = params[1] == params[2] ? 1 : 0;
}

/* gt: 5 a b c
 * set <a> to 1 if <b> is greater than <c>; set it to 0 otherwise */
void gt_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = params[1] > params[2] ? 1 : 0;
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
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = (params[1] + params[2]) % NUMERIC_LIMIT;
}

/* mult: 10 a b c
 * store into <a> the product of <b> and <c> (mod 32768) */
void mult_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = (params[1] * params[2]) % NUMERIC_LIMIT;
}

/* mod: 11 a b c
 * store into <a> the remainder of <b> divided by <c> */
void mod_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = params[1] % params[2];
}

/* and: 12 a b c
 * stores into <a> the bitwise and of <b> and <c> */
void and_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = params[1] & params[2];
}

/* or: 13 a b c
 * stores into <a> the bitwise or of <b> and <c> */
void or_callback(FILE **fp)
{
  uint16_t params[3];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 3);
  assign_ptr_register(&param0);
  *param0 = params[1] | params[2];
}

/* not: 14 a b
 * stores 15-bit bitwise inverse of <b> in <a> */
void not_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&param0);
  *param0 = ~(params[1] >> 1);
}

/* rmem: 15 a b
 * read memory at address <b> and write it to <a> */
void rmem_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&param0);
  *param0 = *((uint16_t *)(virtual_heap + params[1]));
}

/* wmem: 16 a b
 * write the value from <b> into memory at address <a> */
void wmem_callback(FILE **fp)
{
  uint16_t params[2];
  uint16_t *param0 = &params[0];
  read_params(*fp, params, 2);
  assign_ptr_register(&param0);
  *((uint16_t *)(virtual_heap + params[1])) = *param0;
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
  uint16_t *param0 = &a;
  read_params(*fp, &a, 1);
  assign_ptr_register((uint16_t **)&param0);
  *param0 = a;
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
