#pragma once
#include "utl.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct sft_tensor {
  int shape[4];
  uint64_t begin;
  uint64_t sz;
} sft_tensor_t;

static FILE * sft_file;
static void sft_init() {
  sft_file = fopen("model.safetensors", "rb");
  assert(sft_file);
}

static char sft_key_buf[1024];
static char sft_shape_buf[1024];
static char sft_offsets_buf[1024];
static sft_tensor_t sft_find(const char * key) {
  uint64_t hsz;
  assert(0 == fseek(sft_file, 0, SEEK_SET));
  assert(fread(&hsz, 8, 1, sft_file));

  fscanf(sft_file, "{\"__metadata__\":{\"format\":\"pt\"}");
  assert(!ferror(sft_file) && !feof(sft_file));

  char c;
  while ((c = fgetc(sft_file)) != '}') {
    assert(c == ',');

    assert(3 == fscanf(sft_file,
          "\"%[^\"]\":{\"dtype\":\"BF16\",\"shape\":[%[^]]],\"data_offsets\":[%[^]]]}",
          sft_key_buf, sft_shape_buf, sft_offsets_buf));
    if (strcmp(key, sft_key_buf)) continue;

    char * s1 = strchr(sft_shape_buf, ',');
    char * s2 = 0;
    char * s3 = 0;
    if (s1) {
      *s1++ = 0;
      s2 = strchr(s1, ',');
      if (s2) {
        *s2++ = 0;
        s3 = strchr(s2, ',');
        if (s3) *s3++ = 0;
      }
    }

    char * e = strchr(sft_offsets_buf, ',');
    assert(e);
    *e++ = 0;

    return (sft_tensor_t) {
      .shape = {
        atoi(sft_shape_buf),
        s1 ? atoi(s1) : 0,
        s2 ? atoi(s2) : 0,
        s3 ? atoi(s3) : 0,
      },
      .begin = atoll(sft_offsets_buf) + hsz + 8,
      .sz = atoll(e) - atoll(sft_offsets_buf),
    };
  }

  fprintf(stderr, "unknown key [%s]", key);
  exit(1);
}

static void sft_get(const char * tensor, void * data, unsigned s0, unsigned s1, unsigned s2, unsigned s3) {
  sft_tensor_t t = sft_find(tensor);
  assert(s0 == t.shape[0]);
  assert(s1 == t.shape[1]);
  assert(s2 == t.shape[2]);
  assert(s3 == t.shape[3]);

  assert(0 == fseek(sft_file, t.begin, SEEK_SET));
  assert(fread(data, t.sz, 1, sft_file));
}


