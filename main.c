#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "synacor.h"

int main(int argc, const char *argv[])
{
  FILE *fp = NULL;
  uint16_t opcode;

  if (argc < 2) exit(1);
  fp = fopen(argv[1], "rb+");
  if (!fp) exit(1);

  init_stack();
  init_heap_memory();

  while (fread(&opcode, 2, 1, fp) == 1)
  {
    callback_ops[opcode](&fp);
  }
  cleanup(&fp);
  exit(0);
}
