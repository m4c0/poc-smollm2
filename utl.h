#pragma once
#include <stdio.h>
#include "mem.h"

static void utl_assert(size_t n, const char * msg, const char * file, int line) {
  if (n) return;
  fprintf(stderr, "%s:%d: Assertion failed: %s\n", file, line, msg);
  exit(1);
}
#define assert(X) utl_assert((size_t)(X), #X, __FILE__, __LINE__)

static char * utl_slurp(const char * file) {
  FILE * f = fopen(file, "rb");
  assert(f);

  assert(0 == fseek(f, 0, SEEK_END));
  long sz = ftell(f);
  assert(sz);
  assert(0 == fseek(f, 0, SEEK_SET));

  char * data = mem_alloc(sz + 1);
  assert(1 == fread(data, sz, 1, f));
  data[sz] = 0;

  fclose(f);
  return data;
}

