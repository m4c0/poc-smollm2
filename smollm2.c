#include "sft.h"
#include "tkn.h"

static const char * txt = "What's the capital of France?";

typedef enum di_e {
  di_1,
  di_max,
} di_et;
static vlk_buffer_t create_indirect_buffer() {
  vlk_buffer_t buf = vlk_create_host_buffer(di_max * sizeof(VkDispatchIndirectCommand), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

  VkDispatchIndirectCommand * t;
  _(vkMapMemory(vlk_dev, buf.mem, 0, VK_WHOLE_SIZE, 0, (void **)&t));
  t[di_1] = (VkDispatchIndirectCommand) { 1, 1, 1 };
  vkUnmapMemory(vlk_dev, buf.mem);

  return buf;
}

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

  vlk_buffer_t b_indir = create_indirect_buffer();

  vlk_ppl_t p_embed = vlk_create_pipeline("embed.comp.spv", 4, 0);

  sft_list_t b_embed = sft_load_single("embed_tokens", 49152, 576);

  vlk_buffer_t b_xinpt = vlk_create_local_buffer(576, 0);

  unsigned tksz = 0;
  vlk_buffer_t b_input = load_input(txt, &tksz);

  VkCommandBuffer cb = vlk_allocate_command_buffer();
  vlk_begin_command_buffer(cb);
  vlk_dispatch(cb, p_embed, 1, 1, 1, b_indir, b_input, b_embed, b_xinpt);
  vlk_end_command_buffer(cb);
  vlk_submit(cb);

  vkDeviceWaitIdle(vlk_dev);

  dump_output(b_input, tksz);

  vlk_deinit();
  mem_deinit();
}
