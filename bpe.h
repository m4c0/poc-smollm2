#pragma once
#include "utl.h"

#include <string.h>
#include <wchar.h>

static utl_wstr_t bpe_map[50000] = {0};
static wchar_t * bpe_mbstowcs(const char * u8, wchar_t * mb) {
  // This is equivalent to mbstowcs but we don't need to rely on
  // changing/restoring the multibyte locale
  for (; *u8; u8++, mb++) {
    if ((u8[0] & 0x80) == 0) {
      *mb = *u8;
      continue;
    }
    assert((u8[0] & 0xE0) == 0xC0 && (u8[1] & 0xC0) == 0x80 && "found unsupported char in merges.txt");
    *mb = ((u8[0] & 0x1F) << 6) | (u8[1] & 0x3F);
    u8++;
  }
  return mb;
}
static wchar_t * bpe_utf8_to_wchar(const char * a, const char * b) {
  // Final string will never be greater than original. Since it can be smaller,
  // we have to clear everything.
  wchar_t * res = mem_alloc((strlen(a) + strlen(b) + 1) * sizeof(wchar_t));
  // We concatenate both because the algo here only uses merges.txt for ranking.
  bpe_mbstowcs(b, bpe_mbstowcs(a, res));
  return res;
}
static void bpe_init() {
  // merges.txt "encodes" a list of "byte pairs", one pair for line, each pair
  // split by space. Each side of the pair is encoded as UTF-8 in the file, but
  // we should use wchar because it aligns with the tokenisation stuff.
  char * bpe = utl_slurp("merges.txt");

  // Skip comment in the first line
  assert(bpe[0] == '#');
  assert(bpe = strchr(bpe, '\n') + 1);

  utl_wstr_t * ptr = bpe_map;
  char * buf = bpe;
  char * nxt;
  while ((nxt = strchr(buf, '\n'))) {
    *nxt = 0;

    char * spc = strchr(buf, ' ');
    assert(spc);
    *spc = 0;

    ptr->str = bpe_utf8_to_wchar(buf, spc + 1);
    ptr->sz = wcslen(ptr->str);

    ptr++;
    buf = nxt + 1;
  }

  assert(0 == wcscmp(bpe_map[8].str, L"\x120the"));
  assert(bpe_map[8].sz == 4);
  assert(0 == wcscmp(bpe_map[48899].str, L"ectable"));
  assert(bpe_map[48899].sz == 7);
}

typedef struct bpe_list {
  utl_wstr_t * list;
  int sz;
} bpe_list_t;
static bpe_list_t bpe_split(const wchar_t * txt, int len) {
  utl_wstr_t * list = mem_alloc(sizeof(utl_wstr_t) * len);
  int lsz = len;
  for (int i = 0; i < lsz; i++) list[i] = utl_wstr_new(txt + i, 1);

  while (lsz > 1) {
    utl_wstr_t best = {0};
    for (int n = 0; n < 50000; n++) {
      utl_wstr_t ns = bpe_map[n];
      for (int i = 0; i < lsz - 1; i++) {
        const wchar_t * t = list[i].str;
        int tsz = list[i].sz + list[i + 1].sz;
        if (tsz != ns.sz) continue;
        if (0 != wcsncmp(t, ns.str, ns.sz)) continue;
        best = ns;
        n = 50000;
        break;
      }
    }
    // Can't compact more
    if (best.sz == 0) break;

    int wr = 0;
    for (int i = 0; i < lsz - 1; i++) {
      const wchar_t * t = list[i].str;
      int tsz = list[i].sz + list[i + 1].sz;
      if (tsz == best.sz && 0 == wcsncmp(t, best.str, best.sz)) {
        list[wr++] = utl_wstr_new(t, best.sz);
        list[i + 1].sz = 0;
        i++;
      } else {
        list[wr++] = list[i];
      }
    }
    if (list[lsz - 1].sz) list[wr++] = list[lsz - 1];
    lsz = wr;
  }

  return (bpe_list_t) { list, lsz };
}
