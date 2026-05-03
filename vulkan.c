#include "vlk.h"

int main(int argc, char ** argv) {
  vlk_init();

  vlk_deinit();
  mem_deinit();

  (void)utl_wstr_new;
  (void)utl_wstr_printable;
  (void)utl_slurp;
}
