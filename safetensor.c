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

  list_tensors();

  uint16_t * f = malloc(49152 * 576 * sizeof(float));
  sft_get("model.embed_tokens.weight", f, 49152, 576, 0, 0);

  for (int i = 0; i < 3; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));
  for (int i = 576 - 3; i < 576; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));
  for (int i = 49152*576 - 3; i < 49152*576; i++) printf("f[%d] = %f\n", i, bf16_to_f32(f[i]));

  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
}
