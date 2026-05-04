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

static float bf16_to_f32(uint16_t n) {
  // GLSL equivalent of: f = uintBitsToFloat(uint(n) << 16);
  uint32_t i = (uint32_t)n << 16;
  return *(float *)&i;
}

int main() {
  byt_init();
  bpe_init();
  sft_init();
  vcb_init();
  vlk_init();

  vlk_buffer_t b_indir = create_indirect_buffer();

  vlk_ppl_t p_embed = vlk_create_pipeline("embed.comp.spv", 4, 0);
  vlk_ppl_t p_inpnr = vlk_create_pipeline("inpnr.comp.spv", 2, 0);

  sft_list_t b_embed = sft_load_single("embed_tokens",    49152, 576);
  sft_list_t b_inpnr = sft_load_layers("input_layernorm",   576,   0);

  vlk_buffer_t b_xinpt = vlk_create_host_buffer(576, 0);

  unsigned tksz = 0;
  vlk_buffer_t b_input = load_input(txt, &tksz);

  VkCommandBuffer cb = vlk_allocate_command_buffer();
  vlk_begin_command_buffer(cb);

  vlk_dispatch(cb, p_embed, 1, 1, 1, b_indir, b_input, b_embed, b_xinpt);
  for (int i = 0; i < 1; i++) {
    vlk_global_barrier(cb);
    vlk_dispatch(cb, p_inpnr, 1, 1, 1, b_xinpt, b_inpnr.data[i]);

    vlk_global_barrier(cb);
    // x @ q.T // x @ k.T // x @ v.T
    // RoPE (once for x@q, once for x@k)
    // -- angle = tksz-1 * pow(rope_theta, -2 * i / 64)
    // -- rope_theta = 100k
    // -- i in [0;31]
    // -- 64 is head size
    // -- x1 = x[0;31], x2 = x[32;63]
    // -- x1_rot = x1 cos(a) - x2 sin(a)
    // -- x2_rot = x1 sin(a) + x2 cos(a)
    // "cache" kv
    // q @ k.T / sqrt(64) (q[012] -> k[0] etc)
    // softmax
    // xinpt + x @ o.T
  }

  vlk_end_command_buffer(cb);
  vlk_submit(cb);

  vkDeviceWaitIdle(vlk_dev);

  dump_output(b_input, tksz);

  uint16_t * f;
  _(vkMapMemory(vlk_dev, b_xinpt.mem, 0, VK_WHOLE_SIZE, 0, (void **)&f));
  for (int i = 0; i < 3; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));
  for (int i = 576 - 3; i < 576; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));
  vkUnmapMemory(vlk_dev, b_xinpt.mem);

  vlk_deinit();
  mem_deinit();
  (void)vlk_create_local_buffer;
}
