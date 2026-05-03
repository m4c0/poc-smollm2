#include "sft.h"
#include "tkn.h"

static const char * txt = "What's the capital of France?";

int main() {
  byt_init();
  bpe_init();
  sft_init();
  vcb_init();
  vlk_init();

  tkn_ids_t msg = tkn_encode(txt);

  char * buf = mem_alloc(10240);
  int n = tkn_decode(msg, buf, 10240);
  if (n) puts(buf);

  vlk_deinit();
  mem_deinit();
}
