#include "vlk.h"

int main(int argc, char ** argv) {
  vlk_init();

  vlk_buffer_t bin = vlk_create_host_buffer(2, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  vlk_buffer_t bout = vlk_create_host_buffer(2, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  vlk_ppl_t ppl = vlk_create_pipeline("vulkan.comp.spv", 2, 0);

  VkCommandBuffer cb = vlk_allocate_command_buffer();
  vlk_begin_command_buffer(cb);

  uint16_t inp[2] = { 0x4286, 0 }; // 67,0 in BF16
  vkCmdUpdateBuffer(cb, bin.buf, 0, 4, &inp);

  vlk_dispatch(cb, ppl, 1, 1, 1, bin, bout);

  vlk_end_command_buffer(cb);
  vlk_submit(cb);

  vkDeviceWaitIdle(vlk_dev);

  uint16_t * s;
  _(vkMapMemory(vlk_dev, bout.mem, 0, VK_WHOLE_SIZE, 0, (void **)&s));
  uint32_t f = (uint32_t)s[0] << 16;
  printf("%.1f\n", *(float *)&f);
  vkUnmapMemory(vlk_dev, bout.mem);

  vlk_deinit();
  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
  (void)vlk_create_local_buffer;
  (void)vlk_global_barrier;
}
