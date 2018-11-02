#include "vallocator.h"
#include "vimage.h"

#include "vdevice.h"
#include "vbuffer.h"
#include "exceptions/exception.h"

#include <Tempest/Pixmap>

using namespace Tempest::Detail;

VAllocator::VAllocator() {
  }

void VAllocator::setDevice(VDevice &dev) {
  device          = dev.device;
  provider.device = &dev;
  }

VAllocator::Provider::DeviceMemory VAllocator::Provider::alloc(size_t size,uint32_t typeId) {
  VkDeviceMemory   memory=VK_NULL_HANDLE;

  VkMemoryAllocateInfo vk_memoryAllocateInfo;
  vk_memoryAllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  vk_memoryAllocateInfo.pNext           = nullptr;
  vk_memoryAllocateInfo.allocationSize  = size;
  vk_memoryAllocateInfo.memoryTypeIndex = typeId;

  vkAssert(vkAllocateMemory(device->device,&vk_memoryAllocateInfo,nullptr,&memory));
  return memory;
  }

void VAllocator::Provider::free(VAllocator::Provider::DeviceMemory m) {
  vkFreeMemory(device->device,m,nullptr);
  }

VBuffer VAllocator::alloc(const void *mem, size_t size, MemUsage usage, BufferFlags bufFlg) {
  VBuffer ret;
  ret.alloc = this;

  VkBufferCreateInfo createInfo={};
  createInfo.sType                 = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  createInfo.pNext                 = nullptr;
  createInfo.flags                 = 0;
  createInfo.size                  = size;
  createInfo.sharingMode           = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.pQueueFamilyIndices   = nullptr;

  if(bool(usage & MemUsage::TransferSrc))
    createInfo.usage |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  if(bool(usage & MemUsage::TransferDst))
    createInfo.usage |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  if(bool(usage & MemUsage::UniformBit))
    createInfo.usage |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  if(bool(usage & MemUsage::VertexBuffer))
    createInfo.usage |= VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  vkAssert(vkCreateBuffer(device,&createInfo,nullptr,&ret.impl));

  VkMemoryRequirements memRq;
  vkGetBufferMemoryRequirements(device,ret.impl,&memRq);

  uint32_t props=0;
  if(bool(bufFlg&BufferFlags::Staging))
    props|=VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
  if(bool(bufFlg&BufferFlags::Static))
    props|=VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  uint32_t typeId=provider.device->memoryTypeIndex(memRq.memoryTypeBits,VkMemoryPropertyFlagBits(props));

  ret.page=allocator.alloc(size_t(memRq.size),size_t(memRq.alignment),typeId);
  if(!ret.page.page || !commit(ret.page.page->memory,ret.impl,mem,ret.page.offset,size))
    throw std::system_error(Tempest::GraphicsErrc::OutOfHostMemory);
  return ret;
  }

VImage VAllocator::alloc(const Pixmap& pm,bool mip, Tempest::MemUsage usage, Tempest::BufferFlags bufFlg) {
  VImage ret;
  ret.alloc = this;

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = pm.w();
  imageInfo.extent.height = pm.h();
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.format        = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

  vkAssert(vkCreateImage(device, &imageInfo, nullptr, &ret.impl));

  VkMemoryRequirements memRq;
  vkGetImageMemoryRequirements(device, ret.impl, &memRq);

  uint32_t typeId=provider.device->memoryTypeIndex(memRq.memoryTypeBits,VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  ret.page=allocator.alloc(size_t(memRq.size),size_t(memRq.alignment),typeId);
  if(!ret.page.page)// || !commit(ret.page.page->memory,ret.impl,mem,ret.page.offset,size))
    throw std::system_error(Tempest::GraphicsErrc::OutOfHostMemory);
  return ret;
  }

void VAllocator::free(VBuffer &buf) {
  vkDeviceWaitIdle(device);
  vkDestroyBuffer (device,buf.impl,nullptr);

  allocator.free(buf.page);
  }

bool VAllocator::commit(VkDeviceMemory dev,VkBuffer dest,const void* mem,size_t offset,size_t size) {
  if(mem!=nullptr) {
    void* data=nullptr;
    if(vkMapMemory(device,dev,offset,size,0,&data)!=VkResult::VK_SUCCESS)
      return false;
    memcpy(data,mem,size);
    vkUnmapMemory(device,dev);
    }

  if(vkBindBufferMemory(device,dest,dev,offset)!=VkResult::VK_SUCCESS)
    return false;
  return true;
  }