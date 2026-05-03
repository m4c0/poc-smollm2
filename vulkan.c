#include "vlk.h"

int main(int argc, char ** argv) {
  vlk_init();

  vlk_buffer_t buf = vlk_create_host_buffer(2, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  vlk_ppl_t pipe = vlk_create_pipeline("vulkan.comp.spv", 2, 0);

  VkCommandBuffer cb = vlk_allocate_command_buffer();
  vlk_begin_command_buffer(cb);
  vlk_end_command_buffer(cb);
  vlk_submit(cb);

  vkDeviceWaitIdle(vlk_dev);

  vlk_deinit();
  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
  (void)vlk_create_local_buffer;
}
