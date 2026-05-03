#pragma once
#include "utl.h"

#define VOLK_IMPLEMENTATION
#include "volk.h"
#include "Vulkan-Headers/include/vulkan/vulkan_core.h"

#include <stdio.h>

static VkInstance vlk_ins;
static VkDevice vlk_dev;
static VkCommandPool vlk_cpool;
static VkDescriptorPool vlk_dpool;
static VkDescriptorSetLayout vlk_dsl;
static VkPhysicalDevice vlk_pd;
static VkQueue vlk_q;
static VkQueryPool vlk_qpool;
static unsigned vlk_qf;

static void vlk_check(VkResult r, const char * msg) {
  if (r == VK_SUCCESS) return;
  fprintf(stderr, "Vulkan call failed (code=%d): %s\n", r, msg);
  exit(1);
}
#define _(X) vlk_check((X), #X)

static void vlk_create_instance() {
  VkApplicationInfo app = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .apiVersion = VK_API_VERSION_1_2,
  };
  VkInstanceCreateInfo info = (VkInstanceCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app,
  };
#ifdef __APPLE__
  const char * ext[] = {
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
  };
  info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  info.enabledExtensionCount = 1;
  info.ppEnabledExtensionNames = ext;
#endif
  _(vkCreateInstance(&info, NULL, &vlk_ins));
  volkLoadInstance(vlk_ins);
}

static void vlk_find_physical_device() {
  VkPhysicalDevice pd[16];
  uint32_t pdsz = 16;
  _(vkEnumeratePhysicalDevices(vlk_ins, &pdsz, pd));
  for (int i = 0; i < pdsz; i++) {
    VkQueueFamilyProperties qp[16];
    uint32_t qpsz = 16;
    vkGetPhysicalDeviceQueueFamilyProperties(pd[i], &qpsz, qp);
    for (vlk_qf = 0; vlk_qf < qpsz; vlk_qf++) {
      if ((qp[vlk_qf].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0) continue;
      if ((qp[vlk_qf].queueFlags & VK_QUEUE_TRANSFER_BIT) == 0) continue;
      vlk_pd = pd[i];
      return;
    }
  }
  utl_unreachable("could not find suitable vulkan device");
}

static void vlk_create_device() {
  const float pri = 1.0f;
  VkDeviceQueueCreateInfo q = (VkDeviceQueueCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueCount = 1,
    .pQueuePriorities = &pri,
    .queueFamilyIndex = vlk_qf,
  };

  VkDeviceCreateInfo info = (VkDeviceCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &q,
    .enabledExtensionCount = 0,
    .ppEnabledExtensionNames = (const char *[]) {
      "VK_KHR_portability_subset"
    },
  };
#ifdef __APPLE__
  info.enabledExtensionCount = 1;
#endif

  _(vkCreateDevice(vlk_pd, &info, NULL, &vlk_dev));
  volkLoadDevice(vlk_dev);

  vkGetDeviceQueue(vlk_dev, vlk_qf, 0, &vlk_q);
}

static VkShaderModule vlk_create_shader_module(const char * name) {
  FILE * f = fopen(name, "rb");
  assert(f && "shader not found");
  assert(0 == fseek(f, 0, SEEK_END));
  long sz = ftell(f);
  assert(sz && (sz % 4 == 0));
  assert(0 == fseek(f, 0, SEEK_SET));
  uint32_t * data = mem_alloc(sz);
  assert(1 == fread(data, sz, 1, f));
  fclose(f);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = sz,
    .pCode = data,
  };

  VkShaderModule mod;
  _(vkCreateShaderModule(vlk_dev, &info, NULL, &mod));

  return mod;
}

static void vlk_create_descriptor_set_layouts() {
  VkDescriptorSetLayoutBinding bis[] = {{
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
  }};
  VkDescriptorSetLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = bis,
  };
  _(vkCreateDescriptorSetLayout(vlk_dev, &info, NULL, &vlk_dsl));
}

static void vlk_create_descriptor_pool() {
  VkDescriptorPoolSize pszs[1] = {{
    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1024,
  }};
  VkDescriptorPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = 1024,
    .poolSizeCount = 1,
    .pPoolSizes = pszs,
  };
  _(vkCreateDescriptorPool(vlk_dev, &info, NULL, &vlk_dpool));
}

typedef struct vlk_ppl {
  VkPipelineLayout pl;
  VkPipeline ppl;
  unsigned sets;
} vlk_ppl_t;
static vlk_ppl_t vlk_ppl_cache[128];
static unsigned vlk_ppl_cache_idx = 0;
static vlk_ppl_t vlk_create_pipeline(const char * name, unsigned set_count, unsigned pcsz) {
  assert(set_count < 8);

  vlk_ppl_t res;
  res.sets = set_count;

  VkDescriptorSetLayout dsls[8];
  for (int i = 0; i < set_count; i++) dsls[i] = vlk_dsl;
  VkPipelineLayoutCreateInfo pli = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = set_count,
    .pSetLayouts = dsls,
  };

  VkPushConstantRange pc = {
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    .size = pcsz,
  };
  if (pcsz) {
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &pc;
  }

  _(vkCreatePipelineLayout(vlk_dev, &pli, NULL, &res.pl));

  VkShaderModule mod = vlk_create_shader_module(name);

  VkComputePipelineCreateInfo infos[] = {{
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .pName = "main",
      .module = mod,
    },
    .layout = res.pl,
  }};

  _(vkCreateComputePipelines(vlk_dev, NULL, 1, infos, NULL, &res.ppl));
  vkDestroyShaderModule(vlk_dev, mod, NULL);
  vlk_ppl_cache[vlk_ppl_cache_idx++] = res;
  return res;
}

typedef struct vlk_buffer {
  VkBuffer buf;
  VkDeviceMemory mem;
  VkDescriptorSet dset;
  unsigned size;
} vlk_buffer_t;
static vlk_buffer_t vlk_buf_cache[1024];
static unsigned vlk_buf_cache_idx = 0;
static vlk_buffer_t vlk_create_buffer(VkDeviceSize sz, VkMemoryPropertyFlags mem_flags, VkBufferUsageFlags ex_flags) {
  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(vlk_pd, &props);

  for (int i = 0; i < props.memoryTypeCount; i++) {
    VkMemoryPropertyFlags flags = props.memoryTypes[i].propertyFlags;
    if ((flags & mem_flags) != mem_flags) continue;

    vlk_buffer_t res;
    res.size = sz * sizeof(float);

    VkBufferCreateInfo buf = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sz * sizeof(float),
      .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | ex_flags,
    };
    _(vkCreateBuffer(vlk_dev, &buf, NULL, &res.buf));

    VkMemoryAllocateInfo mem = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = sz * sizeof(float),
      .memoryTypeIndex = i,
    };
    _(vkAllocateMemory(vlk_dev, &mem, NULL, &res.mem));
    _(vkBindBufferMemory(vlk_dev, res.buf, res.mem, 0));

    VkDescriptorSetAllocateInfo ds = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = vlk_dpool,
      .descriptorSetCount = 1,
      .pSetLayouts = &vlk_dsl,
    };
    _(vkAllocateDescriptorSets(vlk_dev, &ds, &res.dset));

    VkDescriptorBufferInfo dbi = { res.buf, 0, VK_WHOLE_SIZE };
    VkWriteDescriptorSet wr[] = {{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = res.dset,
      .dstBinding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &dbi,
    }};
    vkUpdateDescriptorSets(vlk_dev, 1, wr, 0, NULL);

    vlk_buf_cache[vlk_buf_cache_idx++] = res;
    return res;
  }
  utl_unreachable("could not find host memory with Vulkan");
}
static vlk_buffer_t vlk_create_host_buffer(VkDeviceSize sz, VkBufferUsageFlags ex_flags) {
  return vlk_create_buffer(sz,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      ex_flags);
}
static vlk_buffer_t vlk_create_local_buffer(VkDeviceSize sz, VkBufferUsageFlags ex_flags) {
  return vlk_create_buffer(sz, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ex_flags);
}

static void vlk_create_query_pool() {
  VkQueryPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    .queryType = VK_QUERY_TYPE_TIMESTAMP,
    .queryCount = 1024,
  };
  _(vkCreateQueryPool(vlk_dev, &info, NULL, &vlk_qpool));
}

static void vlk_create_command_pool() {
  VkCommandPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
  };
  _(vkCreateCommandPool(vlk_dev, &info, NULL, &vlk_cpool));
}

static VkCommandBuffer vlk_allocate_command_buffer() {
  VkCommandBufferAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = vlk_cpool,
    .commandBufferCount = 1,
  };
  VkCommandBuffer cb;
  _(vkAllocateCommandBuffers(vlk_dev, &info, &cb));
  return cb;
}

static void vlk_begin_command_buffer(VkCommandBuffer cb) {
  VkCommandBufferBeginInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
  };
  vkBeginCommandBuffer(cb, &info);
}
static void vlk_end_command_buffer(VkCommandBuffer cb) {
  vkEndCommandBuffer(cb);
}

static void vlk_submit(VkCommandBuffer cb) {
  VkSubmitInfo info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pCommandBuffers = &cb,
    .commandBufferCount = 1,
  };
  _(vkQueueSubmit(vlk_q, 1, &info, NULL));
}

static void vlk_init() {
  _(volkInitialize());

  vlk_create_instance();
  vlk_find_physical_device();
  vlk_create_device();
  vlk_create_descriptor_pool();
  vlk_create_descriptor_set_layouts();
  vlk_create_command_pool();
  vlk_create_query_pool();
}
static void vlk_deinit() {
  uint64_t n = 0;
  for (int i = 0; i < vlk_buf_cache_idx; i++) n += vlk_buf_cache[i].size;
  fprintf(stderr, "\nTotal buffer size: %lluMB\n", n / (1024 * 1024));

  for (int i = 0; i < vlk_buf_cache_idx; i++) {
    vkDestroyBuffer(vlk_dev, vlk_buf_cache[i].buf, NULL);
    vkFreeMemory(vlk_dev, vlk_buf_cache[i].mem, NULL);
  }
  for (int i = 0; i < vlk_ppl_cache_idx; i++) {
    vkDestroyPipelineLayout(vlk_dev, vlk_ppl_cache[i].pl, NULL);
    vkDestroyPipeline(vlk_dev, vlk_ppl_cache[i].ppl, NULL);
  }
  vkDestroyDescriptorSetLayout(vlk_dev, vlk_dsl, NULL);
  vkDestroyDescriptorPool(vlk_dev, vlk_dpool, NULL);
  vkDestroyQueryPool(vlk_dev, vlk_qpool, NULL);
  vkDestroyCommandPool(vlk_dev, vlk_cpool, NULL);
  vkDestroyDevice(vlk_dev, NULL);
  vkDestroyInstance(vlk_ins, NULL);
}

