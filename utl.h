#pragma once
#include <stdio.h>
#include "mem.h"

typedef struct utl_wstr {
  const wchar_t * str;
  int sz;
} utl_wstr_t;
static wchar_t * utl_wstr_printable(utl_wstr_t str) {
  wchar_t * dup = mem_alloc((str.sz + 1) * sizeof(wchar_t));
  for (int i = 0; i < str.sz; i++) dup[i] = str.str[i] < 0x80 ? str.str[i] : '?';
  dup[str.sz] = 0;
  return dup;
}

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

