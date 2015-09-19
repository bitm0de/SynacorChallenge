#ifndef __OPCODES_H__
#define __OPCODES_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* #define SWAP_BYTES(x) (((x) << 8) | ((x) >> 8)) */
#define SWAP_BYTES(x) x

/* 32768-32775 (registers 0-7), >32775 (invalid number ranges)
 * <32768 (literals) */
#define NUMERIC_LIMIT 32768
#define INVALID_RANGE(v) ((v) > 32775)

/* register macros */
#define NUM_REGISTERS 8
#define IS_REGISTER(v) ((v) >= NUMERIC_LIMIT && ((v) % NUMERIC_LIMIT) < NUM_REGISTERS)
#define GET_REGISTER_INDEX(v) ((v) % NUMERIC_LIMIT)
#define GET_REGISTER_PTR(v) &reg[GET_REGISTER_INDEX(v)]

#define MAX_PARAMS 3 /* max opcode param count */

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

uint16_t reg[NUM_REGISTERS]; /* general purpose registers */

/* note: params not pushed onto stack but rather read at the next address
 * in memory. params are read based on num_params, and eip is incremented
 * accordingly after a parameter is read
 * ------------------------------------------------------------------------ */
void read_params(uint16_t **eip, uint16_t *params, size_t num_params)
{
  while (num_params-- > 0)
  {
    *params = SWAP_BYTES(**eip);
    ++*eip;
    ++params;
  }
}

/* retrieve a pointer to a register, or return original pointer if value
 * does not match the numeric register range
 * ---------------------------------------------------------------------- */
uint16_t *get_memory_ptr(uint16_t *p)
{
  if (*p < NUMERIC_LIMIT || INVALID_RANGE(*p)) return p;
  return GET_REGISTER_PTR(*p);
}

/* free dynamically allocated memory for stack structure */
void stack_cleanup()
{
  stack_free(&program_stack);
  program_stack.base = NULL;
}

const char *opcode_alias[NUM_OPCODES] = {
  "halt", "set", "push", "pop", "eq", "gt", "jmp",
  "jt", "jf", "add", "mult", "mod", "and", "or", "not",
  "rmem", "wmem", "call", "ret", "out", "in", "noop"
};

const int opcode_param_count[NUM_OPCODES] = {
  0, 2, 1, 1, 3, 3, 1, 2, 2, 3, 3,
  3, 3, 3, 2, 2, 2, 1, 0, 1, 1, 0
};

/* write out human readable instruction and params. restore_eip determines whether
 * function is meant to restore the original eip address after the instruction is
 * written to stdout.
 * -------------------------------------------------------------------------------- */
void write_instruction(uint16_t opcode, uint16_t **eip, int restore_eip)
{
  int i = 0, param_count;
  uint16_t params[MAX_PARAMS];
  uint16_t *original_eip = NULL;

  if (!VALID_OPCODE(opcode))
  {
    printf("-- [%u] --\n", opcode);
    return;
  }

  printf("%s\t\t", opcode_alias[opcode]);
  if (!(param_count = opcode_param_count[opcode]))
  {
    putc('\n', stdout);
    return;
  }
  
  original_eip = *eip;
  read_params(eip, params, param_count);
  if (restore_eip) *eip = original_eip;

  if (IS_REGISTER(params[0]))
  {
    printf("reg%u ", GET_REGISTER_INDEX(params[i]));
  }
  else
  {
    printf("%u", params[i]);
    if (opcode == OPCODE_OUT)
      printf("[%c]", params[i]);
    putc(' ', stdout);
  }

  ++i;

  for (;i < param_count; ++i)
  {
    if (IS_REGISTER(params[i]))
      printf("reg%u ", GET_REGISTER_INDEX(params[i]));
    else
      printf("%u ", params[i]);
  }

  putc('\n', stdout);
}

#if DEBUG
#define DEBUG_INTERPRET(eip, o, p, c) interpret(eip, o, p, c)
void interpret(uint16_t *eip, uint16_t opcode, uint16_t *params, int param_count)
{
  int i = 0;
  printf("%u\t - %s ", eip - virtual_heap, opcode_alias[opcode]);
  for (;i < param_count; ++i)
  {
    if (IS_REGISTER(params[i])) printf("reg%u:", GET_REGISTER_INDEX(params[i]));
    printf("%u ", *get_memory_ptr(&params[i]));
    if (opcode == OPCODE_OUT) printf("(%c) ", *get_memory_ptr(&params[i]));
    else if (opcode == OPCODE_CALL) printf("(return address: %u) ", stack_top(&program_stack));
    else if (opcode == OPCODE_POP) printf("(pop: %u) ", stack_top(&program_stack));
  }
  putc('\n', stdout);
}
#else
#define DEBUG_INTERPRET(eip, o, p, c) ((void)0)
#endif

/* stop execution and terminate program */
void halt_callback(uint16_t **eip)
{
  DEBUG_INTERPRET(*eip, OPCODE_HALT, NULL, 0);
  (void)eip; /* supress compiler warnings */
  stack_cleanup();
  if (virtual_heap) free(virtual_heap);
  exit(0);
}

/* set: 1 a b
 * set register <a> to the value of <b> */
void set_callback(uint16_t **eip)
{
  uint16_t params[2];
  read_params(eip, params, 2);
  DEBUG_INTERPRET(*eip, OPCODE_SET, params, 2);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]);
}

/* push: 2 a
 * push <a> onto the stack */
void push_callback(uint16_t **eip)
{
  uint16_t param;
  read_params(eip, &param, 1);
  DEBUG_INTERPRET(*eip, OPCODE_PUSH, &param, 1);
  stack_push(&program_stack, *get_memory_ptr(&param));
}

/* pop: 3 a
 * remove the top element from the stack and write it into <a>; empty stack = error */
void pop_callback(uint16_t **eip)
{
  uint16_t param;

  if (stack_empty(&program_stack))
  {
    fputs("ERROR: pop_callback() - stack is empty!\n", stderr);
    exit(1);
  }

  read_params(eip, &param, 1);
  DEBUG_INTERPRET(*eip, OPCODE_POP, &param, 1);
  *get_memory_ptr(&param) = stack_pop(&program_stack);
}

/* eq: 4 a b c
 * set <a> to 1 if <b> is equal to <c>; set it to 0 otherwise */
void eq_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_EQ, params, 3);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]) == *get_memory_ptr(&params[2]) ? 1 : 0;
}

/* gt: 5 a b c
 * set <a> to 1 if <b> is greater than <c>; set it to 0 otherwise */
void gt_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_GT, params, 3);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]) > *get_memory_ptr(&params[2]) ? 1 : 0;
}

/* jmp: 6 a
 * jump to <a> */
void jmp_callback(uint16_t **eip)
{
  uint16_t param;
  read_params(eip, &param, 1);
  DEBUG_INTERPRET(*eip, OPCODE_GT, &param, 3);
  *eip = virtual_heap + *get_memory_ptr(&param);
}

/* jt: 7 a b
 * if <a> is nonzero, jump to <b> */
void jt_callback(uint16_t **eip)
{
  uint16_t params[2];
  read_params(eip, params, 2);
  DEBUG_INTERPRET(*eip, OPCODE_JT, params, 2);
  if (*get_memory_ptr(&params[0])) *eip = virtual_heap + *get_memory_ptr(&params[1]);
}

/* jf: 8 a b
 * if <a> is zero, jump to <b> */
void jf_callback(uint16_t **eip)
{
  uint16_t params[2];
  read_params(eip, params, 2);
  DEBUG_INTERPRET(*eip, OPCODE_JF, params, 2);
  if (!*get_memory_ptr(&params[0])) *eip = virtual_heap + *get_memory_ptr(&params[1]);
}

/* add: 9 a b c
 * assign into <a> the sum of <b> and <c> (mod 32768) */
void add_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_ADD, params, 3);
  *get_memory_ptr(&params[0]) = (*get_memory_ptr(&params[1]) + *get_memory_ptr(&params[2])) % NUMERIC_LIMIT;
}

/* mult: 10 a b c
 * store into <a> the product of <b> and <c> (mod 32768) */
void mult_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_MULT, params, 3);
  *get_memory_ptr(&params[0]) = (*get_memory_ptr(&params[1]) * *get_memory_ptr(&params[2])) % NUMERIC_LIMIT;
}

/* mod: 11 a b c
 * store into <a> the remainder of <b> divided by <c> */
void mod_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_MOD, params, 3);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]) % *get_memory_ptr(&params[2]);
}

/* and: 12 a b c
 * stores into <a> the bitwise and of <b> and <c> */
void and_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_AND, params, 3);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]) & *get_memory_ptr(&params[2]);
}

/* or: 13 a b c
 * stores into <a> the bitwise or of <b> and <c> */
void or_callback(uint16_t **eip)
{
  uint16_t params[3];
  read_params(eip, params, 3);
  DEBUG_INTERPRET(*eip, OPCODE_OR, params, 3);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]) | *get_memory_ptr(&params[2]);
}

/* not: 14 a b
 * stores 15-bit bitwise inverse of <b> in <a> */
void not_callback(uint16_t **eip)
{
  uint16_t params[2];
  read_params(eip, params, 2);
  DEBUG_INTERPRET(*eip, OPCODE_NOT, params, 2);
  *get_memory_ptr(&params[0]) = *get_memory_ptr(&params[1]) ^ 0x7fff;
}

/* rmem: 15 a b
 * read memory at address <b> and write it to <a> */
void rmem_callback(uint16_t **eip)
{
  uint16_t params[2];
  read_params(eip, params, 2);
  DEBUG_INTERPRET(*eip, OPCODE_RMEM, params, 2);
  *get_memory_ptr(&params[0]) = *(virtual_heap + *get_memory_ptr(&params[1]));
}

/* wmem: 16 a b
 * write the value from <b> into memory at address <a> */
void wmem_callback(uint16_t **eip)
{
  uint16_t params[2];
  read_params(eip, params, 2);
  DEBUG_INTERPRET(*eip, OPCODE_WMEM, params, 2);
  *(virtual_heap + *get_memory_ptr(&params[0])) = *get_memory_ptr(&params[1]);
}

/* call: 17 a
 * write the address of the next instruction to the stack and jump to <a> */
void call_callback(uint16_t **eip)
{
  uint16_t param;
  uint32_t ret;
  read_params(eip, &param, 1);
  ret = (uint32_t)(*eip - virtual_heap + 0);
  stack_push(&program_stack, ret);
  DEBUG_INTERPRET(*eip, OPCODE_CALL, &param, 1);
  *eip = virtual_heap + *get_memory_ptr(&param);
}

/* ret: 18
 * remove the top element from the stack and jump to it; empty stack = halt */
void ret_callback(uint16_t **eip)
{
  uint16_t ret_addr;
  if (stack_empty(&program_stack)) halt_callback(eip);
  ret_addr = stack_pop(&program_stack);
#if DEBUG
  printf("%s (return address: %u)\n", opcode_alias[OPCODE_RET], ret_addr);
#endif
  *eip = virtual_heap + ret_addr;
}

/* out: 19 a
 * write the character represented by ascii code <a> to the terminal */
void out_callback(uint16_t **eip)
{
  uint16_t param;
  read_params(eip, &param, 1);
#if DEBUG
  DEBUG_INTERPRET(*eip, OPCODE_OUT, &param, 1);
#else
  putc(*get_memory_ptr(&param), stdout);
#endif
}

/* in: 20 a
 * read a character from the terminal and write its ascii code to <a>;
 * it can be assumed that once input starts, it will continue until a
 * newline is encountered; this means that you can safely read whole
 * lines from the keyboard and trust that they will be fully read */
void in_callback(uint16_t **eip)
{
  int ch;
  uint16_t param;
  read_params(eip, &param, 1);
  DEBUG_INTERPRET(*eip, OPCODE_IN, &param, 1);
  ch = fgetc(stdin);
  *get_memory_ptr(&param) = ch;
}

/* noop: 21
 * no operation */
void noop_callback(uint16_t **eip)
{
  (void)eip; /* supress compiler warnings */
  DEBUG_INTERPRET(*eip, OPCODE_NOOP, NULL, 0);
  return;
}

void (*callback_ops[])(uint16_t **) = { halt_callback, set_callback, push_callback, pop_callback, eq_callback,
                                        gt_callback, jmp_callback, jt_callback, jf_callback, add_callback,
                                        mult_callback, mod_callback, and_callback, or_callback, not_callback,
                                        rmem_callback, wmem_callback, call_callback, ret_callback, out_callback,
                                        in_callback, noop_callback };

#endif /* __OPCODES_H__ */
