#include "tkn.h"
#include "utl.h"

int main() {
  byt_init();
  bpe_init();
  vcb_init();

  tkn_ids_t msg = tkn_encode("The quick brown fox jumped over the lazy dog.");

  for (int i = 0; i < msg.sz; i++) printf("%d ", msg.ids[i]);
  printf("\n");

  char buf[1024];
  int n = tkn_decode(msg, buf, 1024);
  if (n) puts(buf);

  mem_deinit();
}
