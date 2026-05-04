#include "sft.h"

static void list_tensors() {
  FILE * f = sft_file;

  assert(0 == fseek(f, 8, SEEK_SET));
  fscanf(f, "{\"__metadata__\":{\"format\":\"pt\"}");
  assert(!ferror(f) && !feof(f));

  char c;
  while ((c = fgetc(f)) != '}') {
    assert(3 == fscanf(f,
          "\"%[^\"]\":{\"dtype\":\"BF16\",\"shape\":[%[^]]],\"data_offsets\":[%[^]]]}",
          sft_key_buf, sft_shape_buf, sft_offsets_buf));
    printf("%s %s %s\n", sft_key_buf, sft_shape_buf, sft_offsets_buf);
  }
}

static float bf16_to_f32(uint16_t n) {
  // GLSL equivalent of: f = uintBitsToFloat(uint(n) << 16);
  uint32_t i = (uint32_t)n << 16;
  return *(float *)&i;
}

int main() {
  sft_init();
  vlk_init();

  list_tensors();

  sft_list_t b_embed = sft_load_single("embed_tokens", 49152, 576);

  // Just check if it loads
  sft_load_layers("mlp.up_proj", 1536, 576);

  uint16_t * f;
  _(vkMapMemory(vlk_dev, b_embed.data[0].mem, 0, VK_WHOLE_SIZE, 0, (void **)&f));

  for (int i = 0; i < 3; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));
  for (int i = 576 - 3; i < 576; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));
  for (int i = 49152*576 - 3; i < 49152*576; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));

  vkUnmapMemory(vlk_dev, b_embed.data[0].mem);

  vlk_deinit();
  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
  (void)vlk_allocate_command_buffer;
  (void)vlk_begin_command_buffer;
  (void)vlk_create_pipeline;
  (void)vlk_create_local_buffer;
  (void)vlk_dispatch;
  (void)vlk_end_command_buffer;
  (void)vlk_submit;
  (void)vlk_global_barrier;
}
