#include "swap_chain.h"
#include <vulkan/vulkan.h>
#include <engine/log.h>
#include <string.h>
#include <engine/macros.h>
#include "debug.h"
#include <engine/renderer/vulkan/vulkan.h>

#include <engine/renderer/vulkan/images_view.h>

#include <engine/window_manager.h>

typedef struct PE_VK_SWCH_SupportDetails{
  VkSurfaceCapabilitiesKHR capabilities;
  Array formats;
  Array present_modes;
}PE_VK_SWCH_SupportDetails;

PE_VK_SWCH_SupportDetails 
pe_vk_query_swap_chain_support(VkPhysicalDevice device){
  PE_VK_SWCH_SupportDetails details;
  ZERO(details);
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device,vk_surface,&details.capabilities);

  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device,vk_surface,&format_count,NULL);
  
  array_init(&details.formats,sizeof(VkSurfaceFormatKHR),format_count);
  details.formats.count = format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device,vk_surface,&format_count,details.formats.data);


  uint32_t modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device,vk_surface,&modes_count,NULL);

  array_init(&details.present_modes,sizeof(VkPresentModeKHR),modes_count);
  details.present_modes.count = modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device,vk_surface,&modes_count,details.present_modes.data);


  return details;  
}

VkSurfaceFormatKHR 
pe_vk_swch_choose_surface_format(Array* formats){
  for(u8 i = 0; i < formats->count ; i++ ){
    VkSurfaceFormatKHR* format = array_get(formats,i);
    if(format->format == VK_FORMAT_B8G8R8A8_SRGB && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
      return *(format);
    }

  }
  VkSurfaceFormatKHR* format = array_get(formats,0);
  return *(format);
}

VkPresentModeKHR pe_vk_swch_choose_present_mode(Array* present_modes){
  for(u8 i = 0; i < present_modes->count ; i++ ){
    VkPresentModeKHR* mode = array_get(present_modes,i);
    if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return VK_PRESENT_MODE_MAILBOX_KHR;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pe_vk_swch_choose_extent(const VkSurfaceCapabilitiesKHR* capabilities){
  if(capabilities->currentExtent.width != UINT32_MAX)
    return capabilities->currentExtent;
  else{
    VkExtent2D actual = {INIT_WINDOW_SIZE_X,INIT_WINDOW_SIZE_Y};
    return actual;
  }
  
}

void pe_vk_swch_create(){

  PE_VK_SWCH_SupportDetails support = pe_vk_query_swap_chain_support(vk_physical_device);
  VkSurfaceFormatKHR format = pe_vk_swch_choose_surface_format(&support.formats);
  VkPresentModeKHR mode = pe_vk_swch_choose_present_mode(&support.present_modes);
  VkExtent2D extent = pe_vk_swch_choose_extent(&support.capabilities);

  uint32_t image_count = support.capabilities.minImageCount + 1;


  VkSwapchainCreateInfoKHR info;
  ZERO(info);
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = vk_surface;
  info.minImageCount = image_count;
  info.imageFormat = format.format;
  info.presentMode = mode;
  info.imageExtent = extent;
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.preTransform = support.capabilities.currentTransform;
  //info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

  info.clipped = VK_TRUE;
  info.oldSwapchain = VK_NULL_HANDLE;
  
  VKVALID(vkCreateSwapchainKHR(vk_device,&info,NULL,&pe_vk_swap_chain),"Can't create a swap schain");



  pe_vk_swch_extent = extent;
  pe_vk_swch_format = format.format;
  
//  LOG("Swap chain extent %i, %i", pe_vk_swch_extent.width, pe_vk_swch_extent.height);
  
  vkGetSwapchainImagesKHR(vk_device,pe_vk_swap_chain,&image_count,NULL);
 
  VKVALID(vkGetSwapchainImagesKHR(vk_device,pe_vk_swap_chain,&image_count,pe_vk_swch_images),"Cant't create images view");
  
 
}
