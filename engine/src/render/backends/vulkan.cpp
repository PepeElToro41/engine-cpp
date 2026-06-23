#include <SDL3/SDL.h>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <SDL3/SDL_Vulkan.h>
#include <iterator>
#include <iostream>
#include <cstdio>
#include <shaderc/shaderc.hpp>
 
#include "utils.hpp"
#include "vulkan.hpp"
#include "defines.hpp"
#include "core/logging.hpp"
#include "core/memory/allocators/temporal_allocator.hpp"

using ShaderKind = shaderc_shader_kind;

VKAPI_ATTR VkBool32 VKAPI_CALL VALIDATION_DEBUG_CALLBACK(
	const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	const VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
	void *user_data
) {
	if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
	    if (message_severity >= VK_DEBUG_REPORT_ERROR_BIT_EXT) {
	        LOG_ERROR("[VULKAN VALIDATION] %s", callback_data->pMessage)
	    } else {
	        LOG_WARN("[VULKAN VALIDATION] %s", callback_data->pMessage)
	    }
	}

	return VK_FALSE;
}

static bool create_vulkan_instance(VulkanBackend* backend) {
    if (volkInitialize() != VK_SUCCESS) {
        LOG_FATAL("[VULKAN]: Failed to initialize Volk")
		return false;
	}
    TemporalAllocator temp = TemporalAllocator::create();
     
	VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = backend->title,
		.apiVersion = VULKAN_BACKEND::VULKAN_VERSION,
	};

    u32 needed_extension_count = 0;
    const char* const* needed_extensions = SDL_Vulkan_GetInstanceExtensions(&needed_extension_count);
    const char* extra_extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    const u32 total_extensions_count = needed_extension_count + std::size(extra_extensions);
    DynamicArray<const char*> request_extensions(&temp);

    for (u32 i = 0; i < std::size(extra_extensions); i++) {
        request_extensions.push(extra_extensions[i]);
    }
    for (u32 i = 0; i < needed_extension_count; i++) {
        request_extensions.push(needed_extensions[i]);
    }

    const char* requested_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = VALIDATION_DEBUG_CALLBACK,
        .pUserData = nullptr,
    };

    const VkInstanceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &debug_create_info,
        .pApplicationInfo = &app_info,
		.enabledLayerCount = (u32)std::size(requested_layers),
		.ppEnabledLayerNames = requested_layers,
		.enabledExtensionCount = total_extensions_count,
		.ppEnabledExtensionNames = request_extensions.array()
    };

    if(vkCreateInstance(&create_info, nullptr, &backend->instance) != VK_SUCCESS) {
        LOG_FATAL("[VULKAN]: Failed to create Vulkan instance")
        return false;
    }

    volkLoadInstance(backend->instance);
    return true;
}

static bool create_vulkan_allocator(VulkanBackend* backend) {
    VmaVulkanFunctions vma_func_info{};
	const VmaAllocatorCreateInfo allocator_create_info {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = backend->physical_device,
		.device = backend->device,
		.pVulkanFunctions = &vma_func_info,
		.instance = backend->instance,
		.vulkanApiVersion = VULKAN_BACKEND::VULKAN_VERSION,
	};
	vmaImportVulkanFunctionsFromVolk(&allocator_create_info, &vma_func_info);

	if (vmaCreateAllocator(&allocator_create_info, &backend->vma_allocator) != VK_SUCCESS) {
		return false;
	}
	return true;
}

static bool create_window_surface(VulkanBackend* backend) {
    if(!SDL_Vulkan_CreateSurface(backend->window->sdl_window, backend->instance, nullptr, &backend->surface)) {
        LOG_FATAL("[VULKAN]: Failed to create Vulkan surface")
        return false;
    }
    return true;
}

static bool select_physical_device(VulkanBackend* backend) {
    TemporalAllocator temp = TemporalAllocator::create();

    u32 physical_device_count = 0;
    vkEnumeratePhysicalDevices(backend->instance, &physical_device_count, nullptr);
    VkPhysicalDevice* physical_devices = MEMORY::alloc<VkPhysicalDevice>(&temp, physical_device_count);
    vkEnumeratePhysicalDevices(backend->instance, &physical_device_count, physical_devices);

    VkPhysicalDevice selected_device = VK_NULL_HANDLE;
    
    if(physical_device_count) {	
		for (u32 i = 0; i < physical_device_count; i++) {
            VkPhysicalDevice check_device = physical_devices[i];

            VkPhysicalDeviceVulkan14Features supported_v14{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = nullptr };
            VkPhysicalDeviceVulkan13Features supported_v13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &supported_v14 };
	        VkPhysicalDeviceVulkan12Features supported_v12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, .pNext = &supported_v13 };
            VkPhysicalDeviceFeatures2 supported_features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &supported_v12 };

	        vkGetPhysicalDeviceFeatures2(check_device, &supported_features);

            // TODO: do not rely on this if possible. Target vulkan 1.1
            if (!supported_v13.synchronization2) {
                continue;
            }
            
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(check_device, &properties);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                selected_device = check_device;
				break;
			}
		}
    }

    if (selected_device == VK_NULL_HANDLE) {
        LOG_FATAL("[VULKAN]: Could not find a compatible device")
        return false;
    }

	u32 format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(selected_device, backend->surface, &format_count, nullptr);
	VkSurfaceFormatKHR* surface_formats = MEMORY::alloc<VkSurfaceFormatKHR>(&temp, format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(selected_device, backend->surface, &format_count, surface_formats);

    bool supported = false;

    for (u32 i = 0; i < format_count; i++) {
        const VkSurfaceFormatKHR format = surface_formats[i];
		if (format.format == VULKAN_BACKEND::SWAPCHAIN_FORMAT) {
			supported = true;
			break;
		}
	}
	if (!supported) {
        LOG_FATAL("[VULKAN]: Swapchain format not supported")
		return false;
	}

    backend->physical_device = selected_device;
    return true;
}

static bool select_graphics_queue(VulkanBackend* backend) {
    TemporalAllocator temp = TemporalAllocator::create();
    const VkPhysicalDevice physical_device = backend->physical_device;

    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_families_count, nullptr);
    VkQueueFamilyProperties2* queue_families = MEMORY::alloc<VkQueueFamilyProperties2>(&temp, queue_families_count);
    for (u32 i = 0; i < queue_families_count; i++) {
        queue_families[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
    }

    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_families_count, queue_families);

    u32 selected_family_index = UINT32_MAX;
    
    for (u32 i = 0; i < queue_families_count; i++) {
        VkBool32 surface_supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, backend->surface, &surface_supported);
        if (!surface_supported) {
            continue;
        }
        
        VkQueueFamilyProperties2 queue_family = queue_families[i];
        if (queue_family.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            selected_family_index = i;
            break;
        }
    }
    if (selected_family_index == UINT32_MAX) {
        LOG_FATAL("[VULKAN]: Could not find a compatible graphics queue family")
        return false;
    }

    backend->graphics_queue_family_index = selected_family_index;
    return true;
}
static bool create_vulkan_device(VulkanBackend* backend) {
    float queue_priority = 1.0f;
    
    VkPhysicalDeviceVulkan14Features features_v14 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
        .pNext = nullptr,
    };
    VkPhysicalDeviceVulkan13Features features_v13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features_v14,
        .synchronization2 = VK_TRUE,
    };
    VkPhysicalDeviceVulkan12Features features_v12 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features_v13,
        .timelineSemaphore = VK_TRUE
    };
    VkPhysicalDeviceFeatures2 features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &features_v12 };
    
    VkDeviceQueueCreateInfo queue_create_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = backend->graphics_queue_family_index,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority
	};

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo device_create_info {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_create_info,
		.enabledExtensionCount = (u32)std::size(device_extensions),
		.ppEnabledExtensionNames = device_extensions,
		.pEnabledFeatures = nullptr // features struct chain is set in pNext
	};

    if(vkCreateDevice(backend->physical_device, &device_create_info, nullptr, &backend->device) != VK_SUCCESS) {
        LOG_FATAL("[VULKAN]: Failed to create Vulkan device")
        return false;
    }

    vkGetDeviceQueue(backend->device, backend->graphics_queue_family_index, 0, &backend->graphics_queue);
	if (!backend->graphics_queue) {
		LOG_FATAL("[VULKAN]: Couldn't get the graphics queue")
		return false;
	}

    return true;
}


static VkPresentModeKHR choose_swapchain_present_mode(const VulkanBackend* backend) {
    TemporalAllocator temp = TemporalAllocator::create();
    u32 present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(backend->physical_device, backend->surface, &present_mode_count, nullptr);

    if(present_mode_count == 0) {
        LOG_FATAL("[VULKAN]: Failed to get present modes")
        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkPresentModeKHR* present_modes = MEMORY::alloc<VkPresentModeKHR>(&temp, present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(backend->physical_device, backend->surface, &present_mode_count, present_modes);
    
    for (u32 i = 0; i < present_mode_count; i++) {
        const VkPresentModeKHR present_mode = present_modes[i];
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

static bool create_vulkan_swapchain(VulkanBackend* backend) {
    const u32 width = backend->window->width;
    const u32 height = backend->window->height;

    backend->swapchain_width = width;
    backend->swapchain_height = height;

    VkSurfaceCapabilitiesKHR surface_capabilities;
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(backend->physical_device, backend->surface, &surface_capabilities) != VK_SUCCESS) {
		LOG_FATAL("Couldn't get the surface capabilities")
		return false;
	}

    u32 min_image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0) {
        if (min_image_count > surface_capabilities.maxImageCount) {
            min_image_count = surface_capabilities.maxImageCount;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_create_info {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = backend->surface,
		.minImageCount = min_image_count,
		.imageFormat = VULKAN_BACKEND::SWAPCHAIN_FORMAT,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent{.width = width, .height = height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = surface_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = choose_swapchain_present_mode(backend),
        .clipped = VK_TRUE
	};

    // TODO: query present queue family

    if (vkCreateSwapchainKHR(backend->device, &swapchain_create_info, nullptr, &backend->swapchain) != VK_SUCCESS) {
		LOG_FATAL("Error creating swapchain")
		return false;
	}

    u32 image_count = 0;
	vkGetSwapchainImagesKHR(backend->device, backend->swapchain, &image_count, nullptr);

	backend->swapchain_images.fill(image_count);
	backend->swapchain_image_views.fill(image_count);
    backend->swapchain_render_complete_semaphores.fill(image_count);

	vkGetSwapchainImagesKHR(backend->device,  backend->swapchain, &image_count, backend->swapchain_images.array());

    // creating image views
    for (u32 i = 0; i < image_count; i++) {
        VkImage image = backend->swapchain_images.get(i);
        VkImageView* image_view_ref = backend->swapchain_image_views.get_ref(i);

        VkImageViewCreateInfo img_view_info {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
			.image = image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VULKAN_BACKEND::SWAPCHAIN_FORMAT,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
			.subresourceRange {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
				.levelCount = 1,
                .baseArrayLayer = 0,
				.layerCount = 1
			}
		};

        if (vkCreateImageView(backend->device, &img_view_info, nullptr, image_view_ref) != VK_SUCCESS) {
			LOG_FATAL("Error creating swapchain image view")
			return false;
		}
    }

    // creating render complete semaphores
    for (u32 i = 0; i < image_count; i++) {
        VkSemaphore* semaphore_ref = backend->swapchain_render_complete_semaphores.get_ref(i);
        VkSemaphoreCreateInfo semaphore_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        if (vkCreateSemaphore(backend->device, &semaphore_info, nullptr, semaphore_ref) != VK_SUCCESS) {
            LOG_FATAL("Error creating swapchain render complete semaphore")
            return false;
        }
    }
    
    // creating depth buffer
    VkImageCreateInfo depth_create_info {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VULKAN_BACKEND::DEPTH_FORMAT,
		.extent{.width = width, .height = height, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

    VmaAllocationCreateInfo allocation_info {
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO
	};

    if (vmaCreateImage(
        backend->vma_allocator, 
        &depth_create_info, 
        &allocation_info, 
        &backend->depth_image, 
        &backend->depth_image_allocation,
        nullptr
    ) != VK_SUCCESS) {
		LOG_FATAL("Error allocating depth image")
		return false;
	}

    VkImageViewCreateInfo depth_view_create_info {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = backend->depth_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VULKAN_BACKEND::DEPTH_FORMAT,
		.subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .levelCount = 1,
            .layerCount = 1
        }
	};

    if (vkCreateImageView(backend->device, &depth_view_create_info, nullptr, &backend->depth_image_view) != VK_SUCCESS) {
		LOG_FATAL("Error creating depth image view")
		return false;
	}
    return true;
}


static void destroy_swapchain_framebuffers(VulkanBackend* backend) {
    for (VkFramebuffer framebuffer : backend->swapchain_framebuffers) {
        vkDestroyFramebuffer(backend->device, framebuffer, nullptr);
    }
}

static void destroy_vulkan_swapchain(VulkanBackend* backend) {
    for (VkImageView image_view : backend->swapchain_image_views) {
		vkDestroyImageView(backend->device, image_view, nullptr);
	}
	for (VkSemaphore &semaphore : backend->swapchain_render_complete_semaphores) {
        vkDestroySemaphore(backend->device, semaphore, nullptr);
	}
	if (backend->swapchain) {
        vkDestroySwapchainKHR(backend->device, backend->swapchain, nullptr);
	}
	if (backend->depth_image_view) {
        vkDestroyImageView(backend->device, backend->depth_image_view, nullptr);
		vmaDestroyImage(backend->vma_allocator, backend->depth_image, backend->depth_image_allocation);
	}

    backend->swapchain_image_views.clear();
    backend->swapchain_render_complete_semaphores.clear();
    backend->swapchain = VK_NULL_HANDLE;

    backend->depth_image_view = VK_NULL_HANDLE;
    backend->depth_image = VK_NULL_HANDLE;
}

static VkShaderModule create_shader_module(const VulkanBackend* backend, const std::string& filename, const ShaderKind kind) {
    const std::string shader_path = "./shaders/" + filename;
	const std::string source = read_text_file(shader_path);
    const char* filename_c = filename.c_str();
    const char* shader_path_c = shader_path.c_str();

    if (source.empty()) {
		LOG_FATAL("[SHADERC]: Specified shader file doesn't exist or is empty: %s", shader_path_c)
		return nullptr;
	}

    LOG_INFO("[SHADERC]: Compiling shader: %s", shader_path_c)
	const shaderc::Compiler compiler;
	shaderc::CompileOptions compile_options;

	compile_options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
	compile_options.SetTargetSpirv(shaderc_spirv_version_1_6);
	compile_options.SetOptimizationLevel(shaderc_optimization_level_performance);
	shaderc::CompilationResult result = compiler.CompileGlslToSpv(source, kind, filename_c, compile_options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		LOG_FATAL("[SHADERC]: Shader Compilation Error: \n\n %s", result.GetErrorMessage().c_str())
		return nullptr;
	}

    VkShaderModuleCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (result.cend() - result.cbegin()) * sizeof(uint32_t),
        .pCode = result.cbegin(),
    };

    VkShaderModule shader_module = nullptr;
	if (vkCreateShaderModule(backend->device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		LOG_FATAL("[VULKAN]: Error creating shader module")
		return nullptr;
	}

    return shader_module;
}

static bool load_shaders(VulkanBackend* backend) {
    backend->vert_shader = create_shader_module(backend, "shader.vert", ShaderKind::shaderc_vertex_shader);
    if (!backend->vert_shader) {
		return false;
	}
    backend->frag_shader = create_shader_module(backend, "shader.frag", ShaderKind::shaderc_fragment_shader);
	if (!backend->frag_shader) {
		return false;
	}
	return true;
}


static bool create_render_pass(VulkanBackend* backend) {
    VkAttachmentDescription color_attachment {
        .format = VULKAN_BACKEND::SWAPCHAIN_FORMAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference color_attachment_ref {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
     
    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref
    };

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };
    
    if (vkCreateRenderPass(backend->device, &render_pass_info, nullptr, &backend->render_pass) != VK_SUCCESS) {
        LOG_FATAL("[VULKAN]: Error creating render pass")
        return false;
    }
    return true;
}

static bool create_graphics_pipeline(VulkanBackend* backend) {
	VkPipelineLayoutCreateInfo pipeline_layout_info {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 0,
		.pushConstantRangeCount = 0
	};

	if (vkCreatePipelineLayout(
        backend->device, 
        &pipeline_layout_info, 
        nullptr, 
        &backend->pipeline_layout
    ) != VK_SUCCESS) {
		LOG_FATAL("[VULKAN]: Failed to create the pipeline layout")
		return false;
	}

    VkPipelineShaderStageCreateInfo vert_shader_stage_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = backend->vert_shader,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo frag_shader_stage_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = backend->frag_shader,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info,
        frag_shader_stage_info
    };

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (u32)std::size(dynamic_states),
        .pDynamicStates = dynamic_states
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineViewportStateCreateInfo viewport_state_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterization_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState blend_attachment_info {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blending_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment_info,
    };

    VkGraphicsPipelineCreateInfo pipeline_info {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisampling_info,
        .pColorBlendState = &color_blending_info,
        .pDynamicState = &dynamic_state_info,

        .layout = backend->pipeline_layout,
        .renderPass = backend->render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };

    if (vkCreateGraphicsPipelines(backend->device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &backend->pipeline) != VK_SUCCESS) {
        LOG_FATAL("[VULKAN]: Error creating graphics pipeline")
        return false;
    }

    return true;
}

static bool create_frame_buffers(VulkanBackend* backend) {
    backend->swapchain_framebuffers.fill(backend->swapchain_image_views.size);

    for (u64 i = 0; i < backend->swapchain_image_views.size; i++) {
        VkImageView attachment = backend->swapchain_image_views.get(i);

        VkFramebufferCreateInfo framebuffer_info {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = backend->render_pass,
            .attachmentCount = 1,
            .pAttachments = &attachment,
            .width = backend->swapchain_width,
            .height = backend->swapchain_height,
            .layers = 1
        };

        VkFramebuffer* framebuffer_ref = backend->swapchain_framebuffers.get_ref(i);
        if (vkCreateFramebuffer(backend->device, &framebuffer_info, nullptr, framebuffer_ref) != VK_SUCCESS) {
            LOG_FATAL("[VULKAN]: Error creating framebuffer")
            return false;
        }
    }
    return true;
}

static bool create_frame_resources(VulkanBackend* backend) {
    for (u64 i = 0; i < std::size(backend->frame_resources); i++) {
        FrameResources* resources = backend->frame_resources + i;

        VkCommandPoolCreateInfo command_pool_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = backend->graphics_queue_family_index,
        };
        if (vkCreateCommandPool(backend->device, &command_pool_info, nullptr, &resources->command_pool) != VK_SUCCESS) {
            LOG_FATAL("[VULKAN]: Error creating command pool")
            return false;
        }

        VkCommandBufferAllocateInfo command_buffer_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = resources->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(backend->device, &command_buffer_info, &resources->command_buffer) != VK_SUCCESS) {
            LOG_FATAL("[VULKAN]: Error allocating command buffer")
            return false;
        }

    }
    return true;
}

static bool create_frame_sync_resources(VulkanBackend* backend) {
    for (u64 i = 0; i < std::size(backend->frame_resources); i++) {
        FrameResources* resources = backend->frame_resources + i;

        VkSemaphoreCreateInfo semaphore_info {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        if (vkCreateSemaphore(backend->device, &semaphore_info, nullptr, &resources->swapchain_available_semaphore) != VK_SUCCESS) {
            LOG_FATAL("[VULKAN]: Error creating semaphore")
            return false;
        }

        VkFenceCreateInfo fence_info {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };
        if (vkCreateFence(backend->device, &fence_info, nullptr, &resources->render_complete_fence) != VK_SUCCESS) {
            LOG_FATAL("[VULKAN]: Error creating fence")
            return false;
        }
    }
    return true;
}   

bool VulkanBackend::initialize() {
    if(!create_vulkan_instance(this)) {
        LOG_FATAL("Couldn't create Vulkan instance")
        return false;
    }
    LOG_INFO("[VULKAN]: Instance created");

    if (!create_window_surface(this)) {
		LOG_FATAL("Couldn't create window surface")
		return false;
	}
    LOG_INFO("[VULKAN]: Window surface created");

    if (!select_physical_device(this)) {
		LOG_FATAL("Couldn't select physical device")
		return false;
	}
    LOG_INFO("[VULKAN]: Physical device found");

    if (!select_graphics_queue(this)) {
		LOG_FATAL("Couldn't select graphics queue")
		return false;
	}
    LOG_INFO("[VULKAN]: Graphics queue found");

    if (!create_vulkan_device(this)) {
		LOG_FATAL("Couldn't create Vulkan device")
		return false;
	}
    LOG_INFO("[VULKAN]: Device created");

    if (!create_vulkan_allocator(this)) {
		LOG_FATAL("Couldn't create Vulkan allocator")
		return false;
	}
    LOG_INFO("[VULKAN]: VMA allocator created");

    if (!create_vulkan_swapchain(this)) {
		LOG_FATAL("Couldn't create Vulkan swapchain")
		return false;
	}
    LOG_INFO("[VULKAN]: Swapchain created")

    if (!load_shaders(this)) {
		LOG_FATAL("Couldn't load shaders")
		return false;
	}
    LOG_INFO("[VULKAN]: Shaders loaded")

    if (!create_render_pass(this)) {
        LOG_FATAL("Couldn't create render pass")
        return false;
    }
    LOG_INFO("[VULKAN]: Render pass created");

    if (!create_graphics_pipeline(this)) {
        LOG_FATAL("Couldn't create graphics pipeline")
        return false;
    }
    LOG_INFO("[VULKAN]: Graphics pipeline created")

    if (!create_frame_buffers(this)) {
        LOG_FATAL("Couldn't create frame buffers")
        return false;
    }
    LOG_INFO("[VULKAN]: Frame buffers created")

    if (!create_frame_resources(this)) {
        LOG_FATAL("Couldn't create frame resources")
        return false;
    }
    LOG_INFO("[VULKAN]: Frame resources created")

    if (!create_frame_sync_resources(this)) {
        LOG_FATAL("Couldn't create frame sync resources")
        return false;
    }
    LOG_INFO("[VULKAN]: Frame sync resources created")

    return true;
}

void VulkanBackend::shutdown() {
    vkDeviceWaitIdle(this->device);
    
    if (this->pipeline_layout) {
        vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);
    }
    if (this->pipeline) {
        vkDestroyPipeline(this->device, this->pipeline, nullptr);
    }
    for (u64 i = 0; i < std::size(this->frame_resources); i++) {
        FrameResources& resources_ref = this->frame_resources[i];
        vkFreeCommandBuffers(this->device, resources_ref.command_pool, 1, &resources_ref.command_buffer);
        vkDestroyCommandPool(this->device, resources_ref.command_pool, nullptr);
        vkDestroySemaphore(this->device, resources_ref.swapchain_available_semaphore, nullptr);
        vkDestroyFence(this->device, resources_ref.render_complete_fence, nullptr);
    }
    destroy_swapchain_framebuffers(this);
    if (this->render_pass) {
        vkDestroyRenderPass(this->device, this->render_pass, nullptr);
    }

    if (this->vert_shader) {
        vkDestroyShaderModule(this->device, this->vert_shader, nullptr);
    }
    if (this->frag_shader) {
        vkDestroyShaderModule(this->device, this->frag_shader, nullptr);
    }
    destroy_vulkan_swapchain(this);
    
    if (this->vma_allocator) {
        vmaDestroyAllocator(this->vma_allocator);
    }
    if (this->surface) {
        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    }
    if (this->debug_messenger) {
        vkDestroyDebugUtilsMessengerEXT(this->instance, this->debug_messenger, nullptr);
    }
    if (this->device) {
        vkDestroyDevice(this->device, nullptr);
    }
    if (this->instance) {
        vkDestroyInstance(this->instance, nullptr);
    }
    volkFinalize();

    this->swapchain_images.free();
    this->swapchain_image_views.free();
    this->swapchain_render_complete_semaphores.free();
    this->swapchain_framebuffers.free();
}


BEGIN_FRAME_RESULT VulkanBackend::begin_frame() {
    if (this->should_recreate_swapchain) {
        vkDeviceWaitIdle(this->device);
        destroy_swapchain_framebuffers(this);
        destroy_vulkan_swapchain(this);

        create_vulkan_swapchain(this);
        create_frame_buffers(this);
        this->should_recreate_swapchain = false;
    }

    u64 frame_index = this->frame_index;
    u64 frame_resource_index = (u32)(frame_index % VULKAN_BACKEND::MAX_FRAMES_IN_FLIGHT);
    FrameResources& resources = this->frame_resources[frame_resource_index];
    
    vkWaitForFences(this->device, 1,&resources.render_complete_fence, VK_TRUE, UINT64_MAX);
    
    VkSemaphore acquire_semaphore = resources.swapchain_available_semaphore;
    u32 image_index = 0;
    VkResult acquire_result = vkAcquireNextImageKHR(this->device, this->swapchain, UINT64_MAX, acquire_semaphore, VK_NULL_HANDLE, &image_index);
    
    if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
        this->should_recreate_swapchain = true;
        return BEGIN_FRAME_RESULT::SKIP;
    } else if (acquire_result == VK_SUBOPTIMAL_KHR) {
        this->should_recreate_swapchain = true;
    }
    vkResetFences(this->device, 1, &resources.render_complete_fence);
    
    resources.swapchain_image_index = image_index;
    this->frame_index++;
    this->current_frame_resources = &resources;

    vkResetCommandPool(this->device, resources.command_pool, 0);

    VkCommandBufferBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    if (vkBeginCommandBuffer(resources.command_buffer, &begin_info) != VK_SUCCESS) {
        LOG_ERROR("[VULKAN]: Error beginning command buffer");
        return BEGIN_FRAME_RESULT::FAIL;
    }

    return BEGIN_FRAME_RESULT::SUCCESS;
}


void VulkanBackend::render_frame() {
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    FrameResources& resources = *this->current_frame_resources;

    VkRenderPassBeginInfo render_pass_begin_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = this->render_pass,
        .framebuffer = this->swapchain_framebuffers.get(resources.swapchain_image_index),
        .renderArea {
            .offset { 0, 0 },
            .extent {
                .width = this->window->width,
                .height = this->window->height
            }
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor,
    };

    VkCommandBuffer command_buffer = this->current_frame_resources->command_buffer;

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)this->window->width,
        .height = (float)this->window->height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor {
        .offset { 0, 0 },
        .extent {
            .width = this->window->width,
            .height = this->window->height
        }
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);
}

void VulkanBackend::end_frame() {
    FrameResources& resources = *this->current_frame_resources;
    // transition the image from color attachment to presentation so we can show it

    if (vkEndCommandBuffer(resources.command_buffer) != VK_SUCCESS) {
        LOG_ERROR("[VULKAN]: Error ending command buffer");
    }

    VkSemaphore wait_semaphores[] = {
        resources.swapchain_available_semaphore
    };
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkSemaphore signal_semaphores[] = {
        this->swapchain_render_complete_semaphores.get(resources.swapchain_image_index),
    };

    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = wait_semaphores,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &resources.command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signal_semaphores
    };

    if (vkQueueSubmit(this->graphics_queue, 1, &submit_info, resources.render_complete_fence) != VK_SUCCESS) {
        LOG_ERROR("[VULKAN]: Error submitting command buffer");
    }


    VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signal_semaphores,
        .swapchainCount = 1,
        .pSwapchains = &this->swapchain,
        .pImageIndices = &resources.swapchain_image_index,
        .pResults = nullptr
    };

    vkQueuePresentKHR(this->graphics_queue, &present_info);
}