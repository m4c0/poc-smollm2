#pragma once
#include "byt.h"
#include "utl.h"

#include <string.h>
#include <wchar.h>

static utl_wstr_t vcb_map[49152] = {0};
static void vcb_init() {
  char * file = utl_slurp("vocab.json");

  assert(*file++ == '{');
  while (1) {
    assert(*file++ == '"');

    int ksz = 0;
    for (char * p = file; *p && *p != '"'; p++, ksz++) {
      if (*p == '\\') p++;
    }
    char * key = mem_alloc((ksz + 1) * sizeof(wchar_t));

    char * k = key;
    while (*file != '"') {
      // assuming vocab does not have \u
      if (*file == '\\') {
        file++;
        assert(*file == '\"' || *file == '\\');
      }
      *k++ = *file++;
    }
    *k = 0;

    assert(*file++ == '"');
    assert(*file++ == ':');

    char * n = file;
    while (*file >= '0' && *file <= '9') file++;
    int id = atoi(n);
    assert(id >= 0);
    assert(id < 49152);

    vcb_map[id].str = bpe_utf8_to_wchar(key, k);
    vcb_map[id].sz = wcslen(vcb_map[id].str);

    if (*file == '}') break;
    assert(*file++ == ',');
  }

  assert(0 == wcscmp(vcb_map[0].str, L"<|endoftext|>"));
  assert(13 == vcb_map[0].sz);
  assert(0 == wcscmp(vcb_map[76].str, L"\\"));
  assert(1 == vcb_map[76].sz);
  assert(0 == wcscmp(vcb_map[252].str, L"\x120t"));
  assert(2 == vcb_map[252].sz);
  assert(0 == wcscmp(vcb_map[2365].str, L"\x120quick"));
  assert(6 == vcb_map[2365].sz);
}
static int vcb_find_id(utl_wstr_t str) {
  for (int tkn = 0; tkn < 49152; tkn++) {
    if (str.sz != vcb_map[tkn].sz) continue;
    if (wcsncmp(str.str, vcb_map[tkn].str, str.sz)) continue;
    return tkn;
  }
  utl_unreachable("invalid token: %ls", utl_wstr_printable(str));
}
