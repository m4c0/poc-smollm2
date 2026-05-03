#pragma once
#include "utl.h"
#include "vlk.h"

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

typedef struct sft_list {
  vlk_buffer_t data[30];
} sft_list_t;
static vlk_buffer_t sft_load(const char * key, unsigned s0, unsigned s1) {
  vlk_buffer_t b = vlk_create_host_buffer(s0 * (s1 == 0 ? 1 : s1), 0);

  char name[256]; snprintf(name, 256, "model.%s.weight", key);

  void * ptr;
  _(vkMapMemory(vlk_dev, b.mem, 0, VK_WHOLE_SIZE, 0, &ptr));
  sft_get(name, ptr, s0, s1, 0, 0);
  vkUnmapMemory(vlk_dev, b.mem);

  return b;
}
static sft_list_t sft_load_single(const char * key, unsigned s0, unsigned s1) {
  sft_list_t res = {0};
  res.data[0] = sft_load(key, s0, s1);
  return res;
}
static sft_list_t sft_load_layers(const char * key, unsigned s0, unsigned s1) {
  sft_list_t res = {0};
  for (int i = 0; i < 30; i++) {
    char lkey[256]; snprintf(lkey, 256, "layers.%d.%s", i, key);
    res.data[i] = sft_load(lkey, s0, s1);
  }
  return res;
}
