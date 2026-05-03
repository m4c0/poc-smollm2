#include "sft.h"
#include "tkn.h"

static const char * txt = "What's the capital of France?";

static vlk_buffer_t load_input(const char * txt, unsigned * sz) {
  vlk_buffer_t res = vlk_create_host_buffer(8192, 0);

  uint16_t * ids;
  _(vkMapMemory(vlk_dev, res.mem, 0, VK_WHOLE_SIZE, 0, (void **)&ids));

  *sz = tkn_encode(ids, txt);

  vkUnmapMemory(vlk_dev, res.mem);

  return res;
}
static void dump_output(vlk_buffer_t b, unsigned sz) {
  uint16_t * ids;
  _(vkMapMemory(vlk_dev, b.mem, 0, VK_WHOLE_SIZE, 0, (void **)&ids));

  char * buf = mem_alloc(10240);
  int n = tkn_decode(ids, sz, buf, 10240);
  if (n) puts(buf);

  vkUnmapMemory(vlk_dev, b.mem);
}

int main() {
  byt_init();
  bpe_init();
  sft_init();
  vcb_init();
  vlk_init();

  unsigned tksz = 0;
  vlk_buffer_t b_input = load_input(txt, &tksz);

  dump_output(b_input, tksz);

  vlk_deinit();
  mem_deinit();
}
