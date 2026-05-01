#include "utl.h"

#include <string.h>
#include <wchar.h>

static utl_wstr_t vcb_map[50000] = {0};
static wchar_t * vcb_mbstowcs(const char * u8, wchar_t * mb) {
  // This is equivalent to mbstowcs but we don't need to rely on
  // changing/restoring the multibyte locale
  for (; *u8; u8++, mb++) {
    if ((u8[0] & 0x80) == 0) {
      *mb = *u8;
      continue;
    }
    assert((u8[0] & 0xE0) == 0xC0 && (u8[1] & 0xC0) == 0x80 && "found unsupported char in vocab.bpe");
    *mb = ((u8[0] & 0x1F) << 6) | (u8[1] & 0x3F);
    u8++;
  }
  return mb;
}
static wchar_t * vcb_utf8_to_wchar(const char * a, const char * b) {
  // Final string will never be greater than original. Since it can be smaller,
  // we have to clear everything.
  wchar_t * res = mem_alloc((strlen(a) + strlen(b) + 1) * sizeof(wchar_t));
  // We concatenate both because the algo here only uses vocab.bpe for ranking.
  vcb_mbstowcs(b, vcb_mbstowcs(a, res));
  return res;
}

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
    *file = 0;
    wchar_t * k = vcb_utf8_to_wchar(key, file++);

    assert(*file++ == ':');

    char * n = file;
    while (*file >= '0' && *file <= '9') file++;
    int val = atoi(n);

    vcb_map[val].str = k;
    vcb_map[val].sz  = wcslen(k);

    printf("%6d %ls\n", val, utl_wstr_printable(vcb_map[val]));

    if (*file == '}') break;
    assert(*file++ == ',');
  }

  mem_deinit();
}
