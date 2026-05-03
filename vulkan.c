#include "vlk.h"

int main(int argc, char ** argv) {
  vlk_init();

  vlk_buffer_t bin = vlk_create_host_buffer(1, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  vlk_buffer_t bout = vlk_create_host_buffer(1, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  vlk_ppl_t ppl = vlk_create_pipeline("vulkan.comp.spv", 2, 0);

  VkCommandBuffer cb = vlk_allocate_command_buffer();
  vlk_begin_command_buffer(cb);

  float inp = 67;
  vkCmdUpdateBuffer(cb, bin.buf, 0, 4, &inp);

  VkDescriptorSet dsets[2] = { bin.dset, bout.dset };
  vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, ppl.pl, 0, 2, dsets, 0, NULL);

  vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, ppl.ppl);
  vkCmdDispatch(cb, 1, 1, 1);

  vlk_end_command_buffer(cb);
  vlk_submit(cb);

  vkDeviceWaitIdle(vlk_dev);

  float * f;
  _(vkMapMemory(vlk_dev, bout.mem, 0, VK_WHOLE_SIZE, 0, (void **)&f));
  printf("%f\n", f[0]);
  vkUnmapMemory(vlk_dev, bout.mem);

  vlk_deinit();
  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
  (void)vlk_create_local_buffer;
}
