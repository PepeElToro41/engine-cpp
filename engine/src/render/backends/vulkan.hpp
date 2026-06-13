#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>

#include "export.h"
#include "defines.hpp"
#include "templates/dynamic_array.hpp"
#include "render/backend.hpp"
#include "core/memory/allocators/heap_allocator.hpp"

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace VULKAN_BACKEND {
    constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

    constexpr u32 VULKAN_VERSION = VK_API_VERSION_1_3;
    constexpr VkFormat SWAPCHAIN_FORMAT = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;
    constexpr VkFormat DEPTH_FORMAT = VkFormat::VK_FORMAT_D32_SFLOAT;
}

struct ENGINE_API FrameResources {
    VkCommandPool command_pool = VK_NULL_HANDLE;
	VkCommandBuffer command_buffer = VK_NULL_HANDLE;

	VkSemaphore swapchain_available_semaphore = VK_NULL_HANDLE;
    VkFence render_complete_fence = VK_NULL_HANDLE;
    u32 swapchain_image_index = 0;
};

struct ENGINE_API VulkanBackend : RendererBackend {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VmaAllocator vma_allocator = nullptr;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    
    u32 graphics_queue_family_index = 0;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    u32 swapchain_width = 0;
    u32 swapchain_height = 0;
    DynamicArray<VkImage> swapchain_images{&HEAP_ALLOCATOR};
    DynamicArray<VkImageView> swapchain_image_views{&HEAP_ALLOCATOR};
    DynamicArray<VkSemaphore> swapchain_render_complete_semaphores{&HEAP_ALLOCATOR};
    DynamicArray<VkFramebuffer> swapchain_framebuffers{&HEAP_ALLOCATOR};
 
    VkImage depth_image = VK_NULL_HANDLE;
    VkImageView depth_image_view = VK_NULL_HANDLE;
    VmaAllocation depth_image_allocation = nullptr;

    VkShaderModule vert_shader = VK_NULL_HANDLE;
    VkShaderModule frag_shader = VK_NULL_HANDLE;

    u64 frame_index = 0;
    u64 next_wait_frame = VULKAN_BACKEND::MAX_FRAMES_IN_FLIGHT + 1;

    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkRenderPass render_pass = VK_NULL_HANDLE;

    FrameResources frame_resources[VULKAN_BACKEND::MAX_FRAMES_IN_FLIGHT] = {};
    FrameResources* current_frame_resources = nullptr;

    bool should_recreate_swapchain = false;
    const char* title;
    
    explicit VulkanBackend(const char* title) : title(title) {};
    bool initialize() override;
    void shutdown() override;

    BEGIN_FRAME_RESULT begin_frame() override;
    void render_frame() override;
    void end_frame() override;
};
