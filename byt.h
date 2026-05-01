#pragma once
#include "mem.h"
#include "utl.h"

#include <wchar.h>

/// Maps bytes of UTF-8 into multibyte chars used in vocab.bpe.
/// Oddly enough, multibyte UTF-8 will be mapped as multiple wchars.
static wchar_t byt_map[256] = {0};
static char byt_rev_map[65536] = {0};

static wchar_t * byt_encode_bytes(const char * b, unsigned bytes) {
  wchar_t * mb = mem_alloc(sizeof(wchar_t) * bytes);
  for (int i = 0; i < bytes; i++) mb[i] = byt_map[(unsigned)b[i]];
  return mb;
}
static int byt_decode_bytes(utl_wstr_t str, char * dst, unsigned dsz) {
  int i;
  for (i = 0; i < str.sz && i < dsz; i++) dst[i] = byt_rev_map[str.str[i]];
  return i;
}
static void byt_init() {
  for (unsigned c = '!'; c <= '~'; c++) byt_map[c] = c;
  for (unsigned c = 161; c <= 172; c++) byt_map[c] = c;
  for (unsigned c = 174; c <= 255; c++) byt_map[c] = c;

  wchar_t mc = 256;
  for (unsigned c = 0; c <= 255; c++) if (!byt_map[c]) byt_map[c] = mc++;

  for (int i = 0; i < 256; i++) byt_rev_map[byt_map[i]] = i;

  assert(byt_map[0] == 256);
  assert(byt_map[33] == 33);
  assert(byt_map[173] == 323);

  wchar_t * mb = byt_encode_bytes("The quick", 9);
  assert(mb[0] == 'T');
  assert(mb[3] == 288);
}

