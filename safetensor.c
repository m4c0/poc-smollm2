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

int main() {
  sft_init();

  list_tensors();

  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
}
