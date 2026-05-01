#pragma once
#include <stdlib.h>

// Alloc-and-forget routines. Mostly to keep track at the end of the process.

typedef struct mem_ptr {
  struct mem_ptr * next;
  int sz;
} mem_ptr_t;
static mem_ptr_t * mem_list;
static void * mem_alloc(unsigned sz) {
  sz += sizeof(mem_ptr_t);
  mem_ptr_t * p = calloc(sz, 1);
  p->next = mem_list;
  p->sz = sz;
  mem_list = p;
  return p + 1;
}

static void mem_deinit() {
  int sz = 0;
  while (mem_list) {
    mem_ptr_t * n = mem_list->next;
    sz += mem_list->sz;
    free(mem_list);
    mem_list = n;
  }
  fprintf(stderr, "Total allocated memory: %dMB\n", sz / (1024 * 1024));
}

