// Vulkan without a loader. The shell loads vulkan_dzn.dll (Mesa Dozen, Vulkan on D3D12)
// from the package with LoadPackagedLibrary and passes its vk_icdGetInstanceProcAddr.
// volk turns that into the global vk* function pointers RPCS3 calls (patch 0003).
#include "stdafx.h"
#include "xbx_internal.h"

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VOLK_IMPLEMENTATION
#include <volk.h>

LOG_CHANNEL(xbx_log, "XBX");

namespace xbx
{
	namespace
	{
		bool s_vulkan_ready = false;
	}

	bool vulkan_init()
	{
		const auto gipa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(host().vk_icd_get_instance_proc_addr);

		if (!gipa)
		{
			return false;
		}

		// Global (instance-less) entry points come straight from the driver.
		// volkLoadInstance runs after vkCreateInstance inside RPCS3 (vkutils/instance.cpp).
		volkInitializeCustom(gipa);

		const u32 version = volkGetInstanceVersion();
		if (!version)
		{
			xbx_log.error("Dozen did not report a Vulkan version");
			return false;
		}

		xbx_log.notice("Dozen driver loaded, Vulkan instance version %u.%u.%u",
			VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));

		s_vulkan_ready = true;
		return true;
	}

	bool vulkan_available()
	{
		return s_vulkan_ready;
	}
}
