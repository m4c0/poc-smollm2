#pragma once
#include "bpe.h"
#include "byt.h"
#include "mem.h"
#include "vcb.h"

#include <ctype.h>
#include <stdint.h>

static unsigned tkn_next_pptoken_len(const char * b) {
  if (!*b) return 0;
  if (*b == '\'') {
    switch (b[1]) {
      case 'l':
        if (b[2] == 'l') return 3;
        break;
      case 'r':
      case 'v':
        if (b[2] == 'e') return 3;
        break;
      case 's':
      case 't':
      case 'm':
      case 'd':
        return 2;
    }
  }

  const char * bs = *b == ' ' ? b + 1 : b;
  if (isalpha(*bs))
    while (*bs && isalpha(*bs)) bs++;
  else if (isdigit(*bs))
    while (*bs && isdigit(*bs)) bs++;
  else if (!isspace(*bs))
    while (*bs && !isalpha(*bs) && !isdigit(*bs) && !isspace(*bs)) bs++;
  else
    while (*bs && isspace(*bs)) bs++;

  return bs - b;
}

typedef struct tkn_ids {
  uint16_t * ids;
  int sz;
} tkn_ids_t;
static tkn_ids_t tkn_encode(const char * txt) {
  uint16_t * ids = mem_alloc(sizeof(uint16_t) * 8192);
  int idx = 0;

  unsigned len;
  while ((len = tkn_next_pptoken_len(txt))) {
    wchar_t * token = byt_encode_bytes(txt, len);
    bpe_list_t list = bpe_split(token, len);

    for (int i = 0; i < list.sz; i++) {
      ids[idx++] = vcb_find_id(list.list[i]);
    }

    txt += len;
  }

  return (tkn_ids_t) { ids, idx };
}
static int tkn_decode(tkn_ids_t ts, char * buf, int bsz) {
  int total = 0;
  for (int i = 0; i < ts.sz && i < bsz; i++) {
    utl_wstr_t tk = vcb_map[ts.ids[i]];
    total += byt_decode_bytes(tk, buf + total, bsz - total);
  }
  if (total < bsz) buf[total] = 0;
  return total;
}


