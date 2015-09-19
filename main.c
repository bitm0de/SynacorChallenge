#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "synacor.h"

int main(int argc, const char *argv[])
{
  int disassemble = 0; /* determines mode */
  size_t mem_alloc = 0;
  uint16_t opcode; /* opcode instruction */
  uint16_t *eip = NULL; /* instruction pointer */

  if (argc < 2) exit(1);
  if (argc > 2) disassemble = !strncmp("-d", argv[2], 2);

  init_stack();
  mem_alloc = init_alloc_program_mem(argv[1]);
  if (!mem_alloc) exit(1);

  eip = virtual_heap;

  if (disassemble)
  {
    while ((size_t)(eip - virtual_heap) < mem_alloc)
    {
      opcode = *eip;
      ++eip;
      write_instruction(opcode, &eip, 0);
    }
  }
  else
  {
    while ((size_t)(eip - virtual_heap) < mem_alloc)
    {
      opcode = *eip;
      ++eip;

      /*
      write_instruction(opcode, &eip, 1);
      */

      /* if (VALID_OPCODE(opcode)) */

      callback_ops[opcode](&eip);
    }
  }

  if (virtual_heap) free(virtual_heap);
  exit(0);
}
