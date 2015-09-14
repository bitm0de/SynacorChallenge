#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "synacor.h"

int main(int argc, const char *argv[])
{
  FILE *fp = NULL;
  uint16_t opcode; /* opcode instruction */

  if (argc < 2) exit(1);
  fp = fopen(argv[1], "rb+");
  if (!fp) exit(1);

  init_stack();
  init_heap_memory();

  /* NOTE: "Executing self-test..." stage in bin file makes a jump to
   * a location in the virtual address space where an invalid opcode [4864]
   * is found. This issue is circumvented by a macro; VALID_OPCODE */
  while (fread(&opcode, 2, 1, fp) == 1)
  {
    if (VALID_OPCODE(opcode))
      callback_ops[opcode](&fp);
  }
  
  cleanup(&fp);
  exit(0);
}
