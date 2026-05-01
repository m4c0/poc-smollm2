#include "utl.h"

int main() {
  char * file = utl_slurp("vocab.json");

  assert(*file++ == '{');
  while (1) {
    assert(*file++ == '"');
    char * key = file;

    while (*file != '"') {
      // assuming vocab does not have \u....
      if (*file == '\\') {
        file++;
        assert(*file == '\"' || *file == '\\');
        file++;
        continue;
      }
      file++;
    }
    *file++ = 0;

    assert(*file++ == ':');

    char * n = file;
    while (*file >= '0' && *file <= '9') file++;
    int val = atoi(n);

    printf("%6d %s\n", val, key);

    if (*file == '}') break;
    assert(*file++ == ',');
  }

  mem_deinit();
}
