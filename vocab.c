#include "tkn.h"
#include "utl.h"

int main() {
  byt_init();
  bpe_init();
  vcb_init();

  uint16_t ids[16];
  unsigned sz = tkn_encode(ids, "The quick brown fox jumped over the lazy dog.");

  for (int i = 0; i < sz; i++) printf("%d ", ids[i]);
  printf("\n");

  char * buf = mem_alloc(10240);
  int n = tkn_decode(ids, sz, buf, 10240);
  if (n) puts(buf);

  mem_deinit();
}
