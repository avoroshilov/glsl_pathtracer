#include <fstream>
#include <assert.h>

#include "vulkan/basic.h"

namespace vulkan
{
	using namespace math;

	static std::vector<char> readShaderFile(const std::string & filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			printf("Shader file %s not found!\n", filename.c_str());
			return std::vector<char>();
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	void Wrapper::buildSupportedInstanceExtensionsList(bool printList)
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		m_supportedExtensionsProps.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_supportedExtensionsProps.data());

		if (printList)
		{
			printf("%d instance extensions supported\n", extensionCount);
			for (const VkExtensionProperties & extensionProp : m_supportedExtensionsProps)
			{
				printf("  %s\n", extensionProp.extensionName);
			}
		}
	}

	void Wrapper::buildRequiredInstanceExtensionsList(bool printList)
	{
		const char * requiredExtensions[] =
		{
			VK_KHR_SURFACE_EXTENSION_NAME,			//"VK_KHR_surface",
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME		//"VK_KHR_win32_surface"
		};
		const uint32_t requiredExtensionCount = (uint32_t)(sizeof(requiredExtensions) / sizeof(const char *));

		for (uint32_t i = 0; i < requiredExtensionCount; ++i)
		{
			m_requiredExtensionNamesList.push_back(requiredExtensions[i]);
		}
		m_requiredExtensionNamesList.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

		if (printList)
		{
			printf("Required instance extensions:\n");
			for (uint32_t i = 0; i < requiredExtensionCount; ++i)
			{
				printf("  %s\n", requiredExtensions[i]);
			}
		}
	}

	bool Wrapper::initValidationLayers(bool areValidationLayersEnabled)
	{
		bool enableDebugCallback = false;
		bool enableValidationLayers = false;

		m_requiredInstanceValidationLayerNamesList.resize(0);
		m_requiredLogDevValidationLayerNamesList.resize(0);

		if (areValidationLayersEnabled)
		{
			enableDebugCallback = true;
			enableValidationLayers = true;
			m_requiredInstanceValidationLayerNamesList.push_back("VK_LAYER_LUNARG_standard_validation");
			m_requiredLogDevValidationLayerNamesList.push_back("VK_LAYER_LUNARG_standard_validation");
		}

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char * layerName : m_requiredInstanceValidationLayerNamesList)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				printf("Required instance validation layer %s not found!\n", layerName);
			}
		}

		for (const char * layerName : m_requiredLogDevValidationLayerNamesList)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				printf("Required logical device validation layer %s not found!\n", layerName);
			}
		}

		return enableDebugCallback;
	}

	void Wrapper::initDebugCallback(PFN_vkDebugReportCallbackEXT debugCallback)
	{
		VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo = {};
		debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debugCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debugCallbackCreateInfo.pfnCallback = debugCallback;

		auto vkCreateDebugReportCallbackEXT_pfn = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_vkInstance, "vkCreateDebugReportCallbackEXT");
		if (vkCreateDebugReportCallbackEXT_pfn != nullptr)
		{
			vkCreateDebugReportCallbackEXT_pfn(m_vkInstance, &debugCallbackCreateInfo, nullptr, &m_debugCallbackDesc);
			m_debugCallbackInitialized = true;
		}
		else
		{
			printf("Debug callback extension not present!\n");
		}
	}

	void Wrapper::deinitDebugCallback()
	{
		if (m_debugCallbackInitialized)
		{
			auto vkDestroyDebugReportCallbackEXT_pfn = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_vkInstance, "vkDestroyDebugReportCallbackEXT");
			if (vkDestroyDebugReportCallbackEXT_pfn != nullptr)
			{
				vkDestroyDebugReportCallbackEXT_pfn(m_vkInstance, m_debugCallbackDesc, nullptr);
				m_debugCallbackInitialized = false;
			}
			else
			{
				// TODO: error
				printf("Debug callback extension not present!\n");
			}
		}
	}

	void Wrapper::initInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = (uint32_t)m_requiredExtensionNamesList.size();
		createInfo.ppEnabledExtensionNames = m_requiredExtensionNamesList.data();
		if (m_requiredInstanceValidationLayerNamesList.size() > 0)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(m_requiredInstanceValidationLayerNamesList.size());
			createInfo.ppEnabledLayerNames = m_requiredInstanceValidationLayerNamesList.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.ppEnabledLayerNames = nullptr;
		}

		VkResult status;
		if ((status = vkCreateInstance(&createInfo, nullptr, &m_vkInstance)) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create VkInstance: %d\n", (int)status);
		}
	}
	void Wrapper::deinitInstance()
	{
		vkDestroyInstance(m_vkInstance, nullptr);
	}

	// TODO: implement through getQueueFamilyIndex (request idx if required and check if it's valid)
	// static
	bool Wrapper::checkQueuesPresence(const VkPhysicalDevice & physDev, const VkSurfaceKHR & surface, bool needsGraphics, bool needsPresent, bool needsMemoryTransfer, bool needsCompute)
	{
		bool hasGraphics = false;
		bool hasMemoryTransfer = false;
		bool hasCompute = false;
		bool hasPresent = false;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilies.data());

		for (size_t qIdx = 0, qIdxEnd = queueFamilies.size(); qIdx < qIdxEnd; ++qIdx)
		{
			const VkQueueFamilyProperties & queueFamily = queueFamilies[qIdx];

			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				hasGraphics = true;
			}
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT))
			{
				hasMemoryTransfer = true;
			}
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				hasCompute = true;
			}

			VkBool32 isPresentSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physDev, (uint32_t)qIdx, surface, &isPresentSupported);
			if (queueFamily.queueCount > 0 && isPresentSupported)
			{
				hasPresent = true;
			}
		}

		return
			(!needsGraphics || hasGraphics) && 
			(!needsPresent || hasPresent) && 
			(!needsMemoryTransfer || hasMemoryTransfer) && 
			(!needsCompute || hasCompute);
	};

	// static
	Wrapper::VulkanPhysicalDeviceData::SurfaceInfo Wrapper::queryDeviceSurfaceInfo(const VkPhysicalDevice & physDev, const VkSurfaceKHR & surface)
	{
		VulkanPhysicalDeviceData::SurfaceInfo deviceSurfaceInfo;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &deviceSurfaceInfo.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, nullptr);

		deviceSurfaceInfo.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &formatCount, deviceSurfaceInfo.formats.data());

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, nullptr);

		deviceSurfaceInfo.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, deviceSurfaceInfo.presentModes.data());

		return deviceSurfaceInfo;
	}

	// static
	bool Wrapper::checkPhysicalDevice(const VkPhysicalDevice & physDev, const std::vector<const char *> & requiredExtensionNamesList, const VkSurfaceKHR & surface)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physDev, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physDev, &deviceFeatures);

		// Only allowed to run on physical GPUs
		//	(but maybe allow to run on CPUs as well? not important atm)
		if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			return false;
		}
		if (!deviceFeatures.geometryShader)
		{
			return false;
		}

		// TODO:
		//	* prefer discrete over integrated
		//	* prefer devices with presentable queue == graphics queue (possibly increased performance?)
		if (!checkQueuesPresence(physDev, surface, true, true, false, false))
		{
			return false;
		}

		// Check for required extensions support
		std::vector<VkExtensionProperties> curDeviceSupportedExtensions;
		getGenericSupportedDeviceExtensionsList(physDev, &curDeviceSupportedExtensions);

		bool anyExtensionNotFound = false;
		for (const VkExtensionProperties & curDevProp : curDeviceSupportedExtensions)
		{
			bool extensionFound = false;
			for (const char * reqExtName : requiredExtensionNamesList)
			{
				if (strcmp(curDevProp.extensionName, reqExtName) == 0)
				{
					extensionFound = true;
					break;
				}
			}

			if (!extensionFound)
			{
				anyExtensionNotFound = true;
				break;
			}
		}

		if (!anyExtensionNotFound)
		{
			return false;
		}

		// Check for surface parameters support
		VulkanPhysicalDeviceData::SurfaceInfo deviceSurfaceInfo = queryDeviceSurfaceInfo(physDev, surface);
		if (deviceSurfaceInfo.formats.empty() || deviceSurfaceInfo.presentModes.empty())
		{
			return false;
		}

		return true;
	};

	void Wrapper::selectPhysicalDevice()
	{
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, nullptr);

		if (physicalDeviceCount == 0)
		{
			// TODO: error
			printf("No physical devices with Vulkan support found!\n");
		}

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(m_vkInstance, &physicalDeviceCount, physicalDevices.data());

		buildRequiredDeviceExtensionsList(true);

		for (const auto & physDev : physicalDevices)
		{
			if (checkPhysicalDevice(physDev, m_vkPhysicalDeviceData.requiredExtensionNamesList, m_vkPresentableSurface))
			{
				// We'll use the first device that meets our expectations
				//	TODO: prefer discrete GPU to the integrated GPU
				m_vkPhysicalDeviceData.vkHandle = physDev;
				break;
			}
		}

		if (m_vkPhysicalDeviceData.vkHandle == VK_NULL_HANDLE)
		{
			// TODO: error
			printf("No physical device meets the requirements!\n");
		}

		vkGetPhysicalDeviceFeatures(m_vkPhysicalDeviceData.vkHandle, &m_vkPhysicalDeviceData.deviceFeatures);

		// Fill in the physical device info
		buildSupportedDeviceExtensionsList(true);
	}

	void Wrapper::initLogicalDevice()
	{
		auto getQueueFamilyIndex = [](const VkPhysicalDevice & physDev, const VkQueueFlagBits & queueKindFlag) -> int
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilies.data());

			for (size_t qIdx = 0, qIdxEnd = queueFamilies.size(); qIdx < qIdxEnd; ++qIdx)
			{
				const VkQueueFamilyProperties & queueFamily = queueFamilies[qIdx];

				if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & queueKindFlag))
				{
					return (int)qIdx;
				}
			}

			return -1;
		};

		VkSurfaceKHR createdPresentableSurface = m_vkPresentableSurface;
		auto getPresentingQueueFamilyIndex = [&createdPresentableSurface](const VkPhysicalDevice & physDev) -> int
		{
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physDev, &queueFamilyCount, queueFamilies.data());

			for (size_t qIdx = 0, qIdxEnd = queueFamilies.size(); qIdx < qIdxEnd; ++qIdx)
			{
				const VkQueueFamilyProperties & queueFamily = queueFamilies[qIdx];

				VkBool32 isPresentSupported = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physDev, (uint32_t)qIdx, createdPresentableSurface, &isPresentSupported);
				if (queueFamily.queueCount > 0 && isPresentSupported)
				{
					return (int)qIdx;
				}
			}

			return -1;
		};

		VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
		physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

		float queuePriority = 1.0f;
		int graphicsQueueFamilyIndex = getQueueFamilyIndex(m_vkPhysicalDeviceData.vkHandle, VK_QUEUE_GRAPHICS_BIT);
		int presentingQueueFamilyIndex = getPresentingQueueFamilyIndex(m_vkPhysicalDeviceData.vkHandle);

		std::vector<int> queuesRequired;
		queuesRequired.push_back(graphicsQueueFamilyIndex);
		if (presentingQueueFamilyIndex != graphicsQueueFamilyIndex)
			queuesRequired.push_back(presentingQueueFamilyIndex);

		std::vector<VkDeviceQueueCreateInfo> logicalDeviceQueueCreateInfos;
		for (const auto & qIdx : queuesRequired)
		{
			VkDeviceQueueCreateInfo logicalDeviceQueueCreateInfo = {};
			logicalDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			logicalDeviceQueueCreateInfo.queueFamilyIndex = qIdx;
			logicalDeviceQueueCreateInfo.queueCount = 1;
			logicalDeviceQueueCreateInfo.pQueuePriorities = &queuePriority;
			logicalDeviceQueueCreateInfos.push_back(logicalDeviceQueueCreateInfo);
		}

		VkDeviceCreateInfo logicalDeviceCreateInfo = {};
		logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		logicalDeviceCreateInfo.pQueueCreateInfos = logicalDeviceQueueCreateInfos.data();
		logicalDeviceCreateInfo.queueCreateInfoCount = (uint32_t)logicalDeviceQueueCreateInfos.size();
		logicalDeviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		logicalDeviceCreateInfo.enabledExtensionCount = (uint32_t)m_vkPhysicalDeviceData.requiredExtensionNamesList.size();
		logicalDeviceCreateInfo.ppEnabledExtensionNames = m_vkPhysicalDeviceData.requiredExtensionNamesList.data();
		if (m_requiredLogDevValidationLayerNamesList.size() > 0)
		{
			logicalDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(m_requiredLogDevValidationLayerNamesList.size());
			logicalDeviceCreateInfo.ppEnabledLayerNames = m_requiredLogDevValidationLayerNamesList.data();
		}
		else
		{
			logicalDeviceCreateInfo.enabledLayerCount = 0;
			logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
		}

		if (vkCreateDevice(m_vkPhysicalDeviceData.vkHandle, &logicalDeviceCreateInfo, nullptr, &m_vkLogicalDeviceData.vkHandle) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create logical device!\n");
		}

		m_vkLogicalDeviceData.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
		m_vkLogicalDeviceData.presentingQueueFamilyIndex = presentingQueueFamilyIndex;
		vkGetDeviceQueue(m_vkLogicalDeviceData.vkHandle, m_vkLogicalDeviceData.graphicsQueueFamilyIndex, 0, &m_vkLogicalDeviceData.graphicsQueue);
		vkGetDeviceQueue(m_vkLogicalDeviceData.vkHandle, m_vkLogicalDeviceData.presentingQueueFamilyIndex, 0, &m_vkLogicalDeviceData.presentingQueue);
	}
	void Wrapper::deinitLogicalDevice()
	{
		vkDestroyDevice(m_vkLogicalDeviceData.vkHandle, nullptr);
	}

	void Wrapper::initWindowSurface(HWND hWnd)
	{
		VkWin32SurfaceCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = hWnd;
		createInfo.hinstance = GetModuleHandle(nullptr);

#if 0
		auto vkCreateWin32SurfaceKHR_pfn = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(m_vkInstance, "vkCreateWin32SurfaceKHR");
		if (vkCreateWin32SurfaceKHR_pfn != nullptr)
		{
			if (vkCreateWin32SurfaceKHR_pfn(m_vkInstance, &createInfo, nullptr, &m_vkPresentableSurface) != VK_SUCCESS)
			{
				// TODO: error
				printf("Win32 surface creation failed!\n");
			}
		}
		else
		{
			// TODO: error
			printf("CreateWin32SurfaceKHR function is not found!\n");
		}
#else
		if (vkCreateWin32SurfaceKHR(m_vkInstance, &createInfo, nullptr, &m_vkPresentableSurface) != VK_SUCCESS)
		{
			// TODO: error
			printf("Win32 surface creation failed!\n");
		}
#endif
	}
	void Wrapper::deinitWindowSurface()
	{
		vkDestroySurfaceKHR(m_vkInstance, m_vkPresentableSurface, nullptr);
	}

	// static
	void Wrapper::getGenericSupportedDeviceExtensionsList(const VkPhysicalDevice & physDev, std::vector<VkExtensionProperties> * supportedExtensionsProps)
	{
		if (!supportedExtensionsProps)
			return;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, nullptr);

		supportedExtensionsProps->resize(extensionCount);
		vkEnumerateDeviceExtensionProperties(physDev, nullptr, &extensionCount, supportedExtensionsProps->data());
	}

	void Wrapper::buildSupportedDeviceExtensionsList(bool printList)
	{
		getGenericSupportedDeviceExtensionsList(m_vkPhysicalDeviceData.vkHandle, &m_vkPhysicalDeviceData.supportedExtensionsProps);

		if (printList)
		{
			printf("%d device extensions supported\n", (int)m_vkPhysicalDeviceData.supportedExtensionsProps.size());
			for (const VkExtensionProperties & extensionProp : m_vkPhysicalDeviceData.supportedExtensionsProps)
			{
				printf("  %s\n", extensionProp.extensionName);
			}
		}
	}

	void Wrapper::buildRequiredDeviceExtensionsList(bool printList)
	{
		const char * requiredExtensions[] =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME			//"VK_KHR_swapchain"
		};
		const uint32_t requiredExtensionCount = (uint32_t)(sizeof(requiredExtensions) / sizeof(const char *));

		for (uint32_t i = 0; i < requiredExtensionCount; ++i)
		{
			m_vkPhysicalDeviceData.requiredExtensionNamesList.push_back(requiredExtensions[i]);
		}

		if (printList)
		{
			printf("Required device extensions:\n");
			for (uint32_t i = 0; i < requiredExtensionCount; ++i)
			{
				printf("  %s\n", requiredExtensions[i]);
			}
		}
	}

	// static
	VkExtent2D Wrapper::selectPresentableSurfaceExtents(const VkSurfaceCapabilitiesKHR & capabilities, uint32_t w, uint32_t h)
	{
		// Special case: window manager doesn't care about extent being same as window size
		if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() &&
			capabilities.currentExtent.height == std::numeric_limits<uint32_t>::max())
		{
			VkExtent2D actualExtent = { w, h };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}		

		return capabilities.currentExtent;
	}

	// static
	VkSurfaceFormatKHR Wrapper::selectPresentableSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & availableFormats)
	{
		VkSurfaceFormatKHR desiredFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

		// Special case: device doesn't care about format selection
		if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return desiredFormat;
		}

		for (const VkSurfaceFormatKHR & availableFormat : availableFormats)
		{
			if (availableFormat.format == desiredFormat.format && availableFormat.colorSpace == desiredFormat.colorSpace)
			{
				return desiredFormat;
			}
		}

		return availableFormats[0];
	}

	// static
	VkPresentModeKHR Wrapper::selectPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes)
	{
		for (const VkPresentModeKHR & availablePresentMode : availablePresentModes)
		{
			// Present as fast as possible (with tearing)
			if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				return availablePresentMode;
			}
			else if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
			// We could select VK_PRESENT_MODE_MAILBOX_KHR first,
			//	but it facilitates discarding frames, which means potentially wasted effort
		}

		// VK_PRESENT_MODE_FIFO_KHR is guaranteed to be present by the spec
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void Wrapper::initSwapchain()
	{
		m_vkPhysicalDeviceData.surfaceInfo = queryDeviceSurfaceInfo(m_vkPhysicalDeviceData.vkHandle, m_vkPresentableSurface);

		VkExtent2D presentableSurfaceExtents = selectPresentableSurfaceExtents(m_vkPhysicalDeviceData.surfaceInfo.capabilities, (uint32_t)m_windowWidth, (uint32_t)m_windowHeight);
		VkSurfaceFormatKHR presentableSurfaceFormat = selectPresentableSurfaceFormat(m_vkPhysicalDeviceData.surfaceInfo.formats);
		VkPresentModeKHR presentMode =  selectPresentMode(m_vkPhysicalDeviceData.surfaceInfo.presentModes);

		if (0)
		{
			printf("Initializing swapchain of size %dx%d\n", presentableSurfaceExtents.width, presentableSurfaceExtents.height);
		}

		uint32_t minImageCount = m_vkPhysicalDeviceData.surfaceInfo.capabilities.minImageCount;
		// maxImageCount == 0 means there's no limits other than the available memory
		if (m_vkPhysicalDeviceData.surfaceInfo.capabilities.maxImageCount != 0 && minImageCount > m_vkPhysicalDeviceData.surfaceInfo.capabilities.maxImageCount)
		{
			// TODO: warning
			printf("Swapchain max image count is limiting: requested %d, max is %d\n", minImageCount, m_vkPhysicalDeviceData.surfaceInfo.capabilities.maxImageCount);

			minImageCount = m_vkPhysicalDeviceData.surfaceInfo.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_vkPresentableSurface;
		swapchainCreateInfo.minImageCount = minImageCount;
		swapchainCreateInfo.imageFormat = presentableSurfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = presentableSurfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = presentableSurfaceExtents;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { (uint32_t)m_vkLogicalDeviceData.graphicsQueueFamilyIndex, (uint32_t)m_vkLogicalDeviceData.presentingQueueFamilyIndex};
		if (queueFamilyIndices[0] != queueFamilyIndices[1])
		{
			// If the graphics and presenting queues are different, go down the simplest path to avoid explicit management headache
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// Exclusive mode doesn't require explicit queue family list
			swapchainCreateInfo.queueFamilyIndexCount = 0;
			swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		}

		swapchainCreateInfo.preTransform = m_vkPhysicalDeviceData.surfaceInfo.capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(m_vkLogicalDeviceData.vkHandle, &swapchainCreateInfo, nullptr, &m_vkSwapchainData.vkHandle) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create swap chain!\n");
		}

		m_vkSwapchainData.format = presentableSurfaceFormat.format;
		m_vkSwapchainData.extent = presentableSurfaceExtents;
		m_vkSwapchainData.colorSpace = presentableSurfaceFormat.colorSpace;

		uint32_t imageCount;
		vkGetSwapchainImagesKHR(m_vkLogicalDeviceData.vkHandle, m_vkSwapchainData.vkHandle, &imageCount, nullptr);
		m_vkSwapchainData.images.resize(imageCount);
		vkGetSwapchainImagesKHR(m_vkLogicalDeviceData.vkHandle, m_vkSwapchainData.vkHandle, &imageCount, m_vkSwapchainData.images.data());

		m_vkSwapchainData.imageViews.resize(imageCount);

		for (uint32_t imgIdx = 0; imgIdx < imageCount; ++imgIdx)
		{
			VkImageView imageView;
			imageView = createImageView2D(
							m_vkLogicalDeviceData.vkHandle, 
							m_vkSwapchainData.images[imgIdx],
							m_vkSwapchainData.format
							);
			m_vkSwapchainData.imageViews[imgIdx] = imageView;
		}
	}
	void Wrapper::deinitSwapchain()
	{
		for (uint32_t imgIdx = 0, imgIdxEnd = (uint32_t)m_vkSwapchainData.imageViews.size(); imgIdx < imgIdxEnd; ++imgIdx)
		{
			vkDestroyImageView(m_vkLogicalDeviceData.vkHandle, m_vkSwapchainData.imageViews[imgIdx], nullptr);
		}
		vkDestroySwapchainKHR(m_vkLogicalDeviceData.vkHandle, m_vkSwapchainData.vkHandle, nullptr);
	}

	VkShaderModule Wrapper::initShaderModule(const std::vector<char> & shaderByteCode)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = shaderByteCode.size();
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderByteCode.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(m_vkLogicalDeviceData.vkHandle, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			printf("Failed to create shader module %d!\n", (int)shaderByteCode.size());
			return VK_NULL_HANDLE;
		}

		return shaderModule;
	}
	void Wrapper::deinitShaderModules()
	{
		for (const VkShaderModule & vkShaderModule : m_vkShaderModules)
		{
			vkDestroyShaderModule(m_vkLogicalDeviceData.vkHandle, vkShaderModule, nullptr);
		}
	}

	void Wrapper::initRenderPass()
	{
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.format = m_vkSwapchainData.format;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &attachmentDescription;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pDependencies = &subpassDependency;

		if (vkCreateRenderPass(m_vkLogicalDeviceData.vkHandle, &renderPassCreateInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
		{
			printf("Failed to create render pass!\n");
		}
	}
	void Wrapper::deinitRenderPass()
	{
		vkDestroyRenderPass(m_vkLogicalDeviceData.vkHandle, m_vkRenderPass, nullptr);
	}

	void Wrapper::getVertexInputDescriptions(VkVertexInputBindingDescription * bindingDescr, VkVertexInputAttributeDescription attribsDescr[], int numAttribs)
	{
		if (bindingDescr == nullptr || attribsDescr == nullptr || numAttribs != 3)
		{
			// TODO: error
			printf("Wrong parameters supplied to the getVertexInputDescriptions!\n");
			assert(false && "Wrong parameters supplied to the getVertexInputDescriptions!\n");
		}

		*bindingDescr = {};
		bindingDescr->binding = 0;
		bindingDescr->stride = sizeof(Vertex);
		bindingDescr->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		// Position
		attribsDescr[0].binding = 0;
		attribsDescr[0].location = 0;
		attribsDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attribsDescr[0].offset = offsetof(Vertex, pos);
		// Color
		attribsDescr[1].binding = 0;
		attribsDescr[1].location = 1;
		attribsDescr[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attribsDescr[1].offset = offsetof(Vertex, col);
		// Texture coordinates
		attribsDescr[2].binding = 0;
		attribsDescr[2].location = 2;
		attribsDescr[2].format = VK_FORMAT_R32G32_SFLOAT;
		attribsDescr[2].offset = offsetof(Vertex, tc);
	}

	// Initializes graphics pipeline and render passes
	void Wrapper::initPipelineState()
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_vkShaderModules[0];
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_vkShaderModules[1];
		fragShaderStageInfo.pName = "main";
		fragShaderStageInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Vertex Buffers
		VkVertexInputBindingDescription bindingDescription = {};
		const int numAttribs = 3;
		VkVertexInputAttributeDescription attributeDescriptions[numAttribs];
		getVertexInputDescriptions(&bindingDescription, attributeDescriptions, sizeof(attributeDescriptions)/sizeof(VkVertexInputAttributeDescription));

		VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
		pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
		pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = (uint32_t)numAttribs;
		pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions;

		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
		pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_vkSwapchainData.extent.width;
		viewport.height = (float)m_vkSwapchainData.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_vkSwapchainData.extent;

		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
		pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineViewportStateCreateInfo.viewportCount = 1;
		pipelineViewportStateCreateInfo.pViewports = &viewport;
		pipelineViewportStateCreateInfo.scissorCount = 1;
		pipelineViewportStateCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
		pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
		pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
		pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f;
		pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
		pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		// No depth/stencil usage for now
		//VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo;

		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
		pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
		pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
		pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		pipelineColorBlendStateCreateInfo.attachmentCount = 1;
		pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
		pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &m_vkUBODescriptorSetLayout;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = 0;

		if (vkCreatePipelineLayout(m_vkLogicalDeviceData.vkHandle, &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
		{
			printf("Failed to create pipeline layout!\n");
		}

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.stageCount = 2;
		graphicsPipelineCreateInfo.pStages = shaderStages;
		graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState = nullptr;
		graphicsPipelineCreateInfo.layout = m_vkPipelineLayout;
		graphicsPipelineCreateInfo.renderPass = m_vkRenderPass;
		graphicsPipelineCreateInfo.subpass = 0;
		graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		graphicsPipelineCreateInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(m_vkLogicalDeviceData.vkHandle, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
		{
			printf("Failed to create graphics pipeline!\n");
		}
	}
	void Wrapper::deinitPipelineState()
	{
		vkDestroyPipeline(m_vkLogicalDeviceData.vkHandle, m_vkGraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_vkLogicalDeviceData.vkHandle, m_vkPipelineLayout, nullptr);
	}

	void Wrapper::initSwapchainFramebuffers()
	{
		m_vkSwapchainData.framebuffers.resize(m_vkSwapchainData.imageViews.size());

		for (size_t i = 0, iend = m_vkSwapchainData.framebuffers.size(); i < iend; i++)
		{
			VkImageView attachments[] =
			{
				m_vkSwapchainData.imageViews[i]
			};

			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = m_vkRenderPass;
			framebufferCreateInfo.attachmentCount = 1;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = m_vkSwapchainData.extent.width;
			framebufferCreateInfo.height = m_vkSwapchainData.extent.height;
			framebufferCreateInfo.layers = 1;

			if (vkCreateFramebuffer(m_vkLogicalDeviceData.vkHandle, &framebufferCreateInfo, nullptr, &m_vkSwapchainData.framebuffers[i]) != VK_SUCCESS)
			{
				// TODO: error
				printf("Failed to create swapchain framebuffer for image view %lld!\n", (uint64_t)(attachments[0]));
			}
		}
	}
	void Wrapper::deinitSwapchainFramebuffers()
	{
		for (const VkFramebuffer & swapchainFramebuffer : m_vkSwapchainData.framebuffers)
		{
			vkDestroyFramebuffer(m_vkLogicalDeviceData.vkHandle, swapchainFramebuffer, nullptr);
		}
	}

	void Wrapper::initCommandPool()
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.queueFamilyIndex = m_vkLogicalDeviceData.graphicsQueueFamilyIndex;
		commandPoolCreateInfo.flags = 0;

		if (vkCreateCommandPool(m_vkLogicalDeviceData.vkHandle, &commandPoolCreateInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create graphics command pool!\n");
		}
	}
	void Wrapper::deinitCommandPool()
	{
		// Command buffers deallocation happens automatically on VkCommandPool destruction
		vkDestroyCommandPool(m_vkLogicalDeviceData.vkHandle, m_vkCommandPool, nullptr);
	}

	uint32_t Wrapper::findMemoryType(const VkPhysicalDevice & physDev, uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties)
	{
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physDev, &physicalDeviceMemoryProperties);

		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
		{
			if ( (typeFilter & (1 << i)) &&
				 ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) )
			{
				return i;
			}
		}

		// TODO: warning
		printf("Failed to find suitable memory type!\n");
		assert(false && "Failed to find suitable memory type!\n");
		return 0xFFffFFff;
	}

	VkCommandPool Wrapper::getTransientCommandPool()
	{
		// TODO: here we use m_vkCommandPool which is generalized command pool
		//	it should be better to sue command pool specialized for temporary operations like that,
		//	created with a `VK_COMMAND_POOL_CREATE_TRANSIENT_BIT` flag
		return m_vkCommandPool;
	}

	VkCommandBuffer Wrapper::beginTransientCommandBuffer()
	{
		VkCommandPool transientCommandPool = getTransientCommandPool();

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandPool = transientCommandPool;
		commandBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_vkLogicalDeviceData.vkHandle, &commandBufferAllocateInfo, &commandBuffer);

		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

		return commandBuffer;
	}

	void Wrapper::endTransientCommandBuffer(const VkCommandBuffer & transientCommandBuffer)
	{
		vkEndCommandBuffer(transientCommandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &transientCommandBuffer;

		vkQueueSubmit(m_vkLogicalDeviceData.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_vkLogicalDeviceData.graphicsQueue);

		VkCommandPool transientCommandPool = getTransientCommandPool();
		vkFreeCommandBuffers(m_vkLogicalDeviceData.vkHandle, transientCommandPool, 1, &transientCommandBuffer);
	}

	void Wrapper::createBuffer(
			const VkPhysicalDevice & physDev,
			const VkDevice & logicDev,
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags memoryProperties,
			VkBuffer * buffer,
			VkDeviceMemory * bufferDeviceMemory
			)
	{
		if (buffer == nullptr || bufferDeviceMemory == nullptr)
		{
			// TODO: warning
			printf("Wrong data supplied for createBuffer!\n");
		}

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicDev, &bufferCreateInfo, nullptr, buffer) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create triangle vertex buffer!\n");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(logicDev, *buffer, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(physDev, memoryRequirements.memoryTypeBits, memoryProperties);

		// TODO: vulkan memory allocation
		//	using fine-grained allocations for each buffer is bad, since devices could have very limited amount of
		//	allocations being live simultaneously. Instead, it is better to gather all of the buffers and make single
		//	allocation for them, then splitting the data between them.
		if (vkAllocateMemory(logicDev, &memoryAllocateInfo, nullptr, bufferDeviceMemory) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to allocate triangle vertex buffer memory!");
		}

		// Offset (currently simply 0) needs to be divisible by memoryRequirements.alignment
		vkBindBufferMemory(logicDev, *buffer, *bufferDeviceMemory, 0);
	}

	void Wrapper::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer transientCommandBuffer;

		transientCommandBuffer = beginTransientCommandBuffer();
		{
			VkBufferCopy bufferCopyRegion = {};
			bufferCopyRegion.srcOffset = 0;
			bufferCopyRegion.dstOffset = 0;
			bufferCopyRegion.size = size;
			vkCmdCopyBuffer(transientCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);
		}
		endTransientCommandBuffer(transientCommandBuffer);
	}

	void Wrapper::createImage(
			const VkPhysicalDevice & physDev,
			const VkDevice & logicDev,
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags memoryProperties,
			VkImage * image,
			VkDeviceMemory * imageDeviceMemory
			)
	{
		if (image == nullptr || imageDeviceMemory == nullptr)
		{
			// TODO: warning
			printf("Wrong data supplied for createImage!\n");
		}

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.flags = 0;

		if (vkCreateImage(logicDev, &imageCreateInfo, nullptr, image) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create image!\n");
		}

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(logicDev, *image, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryType(physDev, memoryRequirements.memoryTypeBits, memoryProperties);

		if (vkAllocateMemory(logicDev, &memoryAllocateInfo, nullptr, imageDeviceMemory) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to allocate image memory!");
		}

		vkBindImageMemory(logicDev, *image, *imageDeviceMemory, 0);
	}

	void Wrapper::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
	{
		VkCommandBuffer transientCommandBuffer;

		transientCommandBuffer = beginTransientCommandBuffer();
		{
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			// In case of transitioning queue families, these fields should be used
			//	In case no ownership transition is intended, they should be both VK_QUEUE_FAMILY_IGNORED
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imageMemoryBarrier.subresourceRange.levelCount = 1;
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imageMemoryBarrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags srcSyncStage = 0;
			VkPipelineStageFlags dstSyncStage = 0;

			// Try to infer synchronization logic from the layouts supplied
			if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				srcSyncStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstSyncStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				srcSyncStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstSyncStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else
			{
				// TODO: error
				printf("Transition logic doesn't support this pair of layouts");
				assert(false && "Transition logic doesn't support this pair of layouts");
			}

			const uint32_t memoryBarrierCount = 0;
			const VkMemoryBarrier * pMemoryBarriers = nullptr;

			const uint32_t bufferMemoryBarrierCount = 0;
			const VkBufferMemoryBarrier * pBufferMemoryBarriers = nullptr;
			
			const uint32_t imageMemoryBarrierCount = 1;
			const VkImageMemoryBarrier pImageMemoryBarriers[] = { imageMemoryBarrier };

			vkCmdPipelineBarrier(
				transientCommandBuffer,
				srcSyncStage,
				dstSyncStage,
				0,
				memoryBarrierCount,
				pMemoryBarriers,
				bufferMemoryBarrierCount,
				pBufferMemoryBarriers,
				imageMemoryBarrierCount,
				pImageMemoryBarriers
				);
		}
		endTransientCommandBuffer(transientCommandBuffer);
	}

	void Wrapper::copyBufferToImage(uint32_t width, uint32_t height, VkBuffer buffer, VkImage image)
	{
		VkCommandBuffer transientCommandBuffer;

		transientCommandBuffer = beginTransientCommandBuffer();
		{
			VkBufferImageCopy bufferImageCopyRegion = {};
			bufferImageCopyRegion.bufferOffset = 0;
			bufferImageCopyRegion.bufferRowLength = 0;
			bufferImageCopyRegion.bufferImageHeight = 0;

			bufferImageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferImageCopyRegion.imageSubresource.mipLevel = 0;
			bufferImageCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferImageCopyRegion.imageSubresource.layerCount = 1;

			bufferImageCopyRegion.imageOffset = {0, 0, 0};
			bufferImageCopyRegion.imageExtent = {width, height, 1};

			vkCmdCopyBufferToImage(
				transientCommandBuffer,
				buffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferImageCopyRegion
				);
		}
		endTransientCommandBuffer(transientCommandBuffer);
	}

	void Wrapper::initTextureImage()
	{
		const int imgSizeW = 256;
		const int imgSizeH = 256;
		const int imgSize = imgSizeW*imgSizeH*4;
		const size_t imgBufferSize = imgSizeW*imgSizeH*4*sizeof(unsigned char);
		unsigned char * imgData = new unsigned char[imgSize];

		if (imgData == nullptr)
		{
			// TODO: error
			printf("Failed to allocate memory for texture!\n");
			return;
		}

		int yAdd = 0;
		for (int y = 0; y < imgSizeH; ++y)
		{
			for (int x = 0; x < imgSizeW; ++x)
			{
				int pixelOffset = ((x + yAdd) << 2);
				imgData[pixelOffset  ] = (rand() % 255);
				imgData[pixelOffset+1] = (rand() % 255);
				imgData[pixelOffset+2] = (rand() % 255);
				imgData[pixelOffset+3] = (rand() % 255);
			}
			yAdd += imgSizeW;
		}

		m_vkTextureImageFormat = VK_FORMAT_R8G8B8A8_UNORM;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferDeviceMemory;

		createBuffer(
			m_vkPhysicalDeviceData.vkHandle,
			m_vkLogicalDeviceData.vkHandle,
			(VkDeviceSize)imgBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&stagingBuffer,
			&stagingBufferDeviceMemory
			);

		void * data = nullptr;
		vkMapMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory, 0, (VkDeviceSize)imgBufferSize, 0, &data);
		memcpy(data, imgData, imgBufferSize);
		vkUnmapMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory);

		delete [] imgData;

		createImage(
			m_vkPhysicalDeviceData.vkHandle,
			m_vkLogicalDeviceData.vkHandle,
			(uint32_t)imgSizeW,
			(uint32_t)imgSizeH,
			m_vkTextureImageFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&m_vkTextureImage,
			&m_vkTextureImageDeviceMemory
			);

		/*
		// TODO: move copyBuffer to staging here, and take subresourceLayout.rowPitch into account
		//	actually, seems like it is not needed since staging buffer is used which is linear,
		//	and further layout transition handles the rest
		VkSubresourceLayout subresourceLayout;
		VkImageSubresource imageSubresource;
		imageSubresource.arrayLayer = 0;
		imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageSubresource.mipLevel = 0;
		vkGetImageSubresourceLayout(m_vkLogicalDeviceData.vkHandle, m_vkTextureImage, *imageSubresource, &subresourceLayout);
		//*/

		// Transition image layout for transfer-friendly, not caring about the current image contents
		transitionImageLayout(
			m_vkTextureImage,
			m_vkTextureImageFormat,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			);
		copyBufferToImage(
			(uint32_t)imgSizeW,
			(uint32_t)imgSizeH,
			stagingBuffer,
			m_vkTextureImage
			);
		// Transition image layout for optimal access from the shader
		transitionImageLayout(
			m_vkTextureImage,
			m_vkTextureImageFormat,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

		vkDestroyBuffer(m_vkLogicalDeviceData.vkHandle, stagingBuffer, nullptr);
		vkFreeMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory, nullptr);
	}
	void Wrapper::deinitTextureImage()
	{
		vkDestroyImage(m_vkLogicalDeviceData.vkHandle, m_vkTextureImage, nullptr);
		vkFreeMemory(m_vkLogicalDeviceData.vkHandle, m_vkTextureImageDeviceMemory, nullptr);
	}

	VkImageView Wrapper::createImageView2D(const VkDevice & logicDev, VkImage image, VkFormat format)
	{
		VkImageView imageView;

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = format;

		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicDev, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create image view!\n");
			assert(false && "Failed to create image view!\n");
		}

		return imageView;
	}

	void Wrapper::initTextureImageView()
	{
		m_vkTextureImageView = createImageView2D(m_vkLogicalDeviceData.vkHandle, m_vkTextureImage, m_vkTextureImageFormat);
	}
	void Wrapper::deinitTextureImageView()
	{
		vkDestroyImageView(m_vkLogicalDeviceData.vkHandle, m_vkTextureImageView, nullptr);
	}

	void Wrapper::initTextureSampler()
	{
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		if (m_vkPhysicalDeviceData.deviceFeatures.samplerAnisotropy)
		{
			samplerCreateInfo.anisotropyEnable = VK_TRUE;
			samplerCreateInfo.maxAnisotropy = 16;
		}
		else
		{
			samplerCreateInfo.anisotropyEnable = VK_FALSE;
			samplerCreateInfo.maxAnisotropy = 1;
		}
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 0.0f;

		if (vkCreateSampler(m_vkLogicalDeviceData.vkHandle, &samplerCreateInfo, nullptr, &m_vkTextureSampler) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create texture sampler!\n");
		}
	}
	void Wrapper::deinitTextureSampler()
	{
		vkDestroySampler(m_vkLogicalDeviceData.vkHandle, m_vkTextureSampler, nullptr);
	}

	void Wrapper::initFSQuadBuffers()
	{
		//
		// Vertex buffer
		// 
		{
			const size_t numVertices = 4;
			Vertex vertices[numVertices];
			vertices[0] = { Vec3C(-1.0f, -1.0f, 0.0f), Vec4C(1.0f, 1.0f, 1.0f, 1.0f), Vec2C(0.0f, 1.0f) };
			vertices[1] = { Vec3C( 1.0f, -1.0f, 0.0f), Vec4C(1.0f, 1.0f, 1.0f, 1.0f), Vec2C(1.0f, 1.0f) };
			vertices[2] = { Vec3C(-1.0f,  1.0f, 0.0f), Vec4C(1.0f, 1.0f, 1.0f, 1.0f), Vec2C(0.0f, 0.0f) };
			vertices[3] = { Vec3C( 1.0f,  1.0f, 0.0f), Vec4C(1.0f, 1.0f, 1.0f, 1.0f), Vec2C(1.0f, 0.0f) };

			m_vkTriangleVerticesCount = numVertices;

			size_t triangleVertexBufferSize = sizeof(vertices[0]) * numVertices;

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferDeviceMemory;
			createBuffer(
				m_vkPhysicalDeviceData.vkHandle,
				m_vkLogicalDeviceData.vkHandle,
				(VkDeviceSize)triangleVertexBufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer,
				&stagingBufferDeviceMemory
				);

			// Fill in the Vertex Buffer
		
			/*
			(obsolete warning, we're using staging buffer now)
			// WARNING: to make sure the memory is seen by the device immediately, we're using VK_MEMORY_PROPERTY_HOST_COHERENT_BIT for the allocation
			//	but faster and probably more widespread way of doing so would be to utilize `vkFlushMappedMemoryRanges`/`vkInvalidateMappedMemoryRanges`
			*/

			void * data = nullptr;
			vkMapMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory, 0, (VkDeviceSize)triangleVertexBufferSize, 0, &data);
			memcpy(data, vertices, triangleVertexBufferSize);
			vkUnmapMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory);

			createBuffer(
				m_vkPhysicalDeviceData.vkHandle,
				m_vkLogicalDeviceData.vkHandle,
				(VkDeviceSize)triangleVertexBufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&m_vkTriangleVertexBuffer,
				&m_vkTriangleVertexBufferDeviceMemory
				);

			copyBuffer(stagingBuffer, m_vkTriangleVertexBuffer, (VkDeviceSize)triangleVertexBufferSize);

			vkDestroyBuffer(m_vkLogicalDeviceData.vkHandle, stagingBuffer, nullptr);
			vkFreeMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory, nullptr);
		}

		//
		// Index buffer
		// 
		{
			const size_t numIndices = 6;
			uint16_t indices[numIndices] = 
			{
				0, 1, 2, 2, 1, 3
			};
			
			m_vkTriangleIndexBufferType = VK_INDEX_TYPE_UINT16;
			m_vkTriangleIndicesCount = numIndices;

			size_t triangleIndexBufferSize = sizeof(indices[0]) * numIndices;

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferDeviceMemory;
			createBuffer(
				m_vkPhysicalDeviceData.vkHandle,
				m_vkLogicalDeviceData.vkHandle,
				(VkDeviceSize)triangleIndexBufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&stagingBuffer,
				&stagingBufferDeviceMemory
				);

			// Fill in the buffer

			void * data = nullptr;
			vkMapMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory, 0, (VkDeviceSize)triangleIndexBufferSize, 0, &data);
			memcpy(data, indices, triangleIndexBufferSize);
			vkUnmapMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory);

			createBuffer(
				m_vkPhysicalDeviceData.vkHandle,
				m_vkLogicalDeviceData.vkHandle,
				(VkDeviceSize)triangleIndexBufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				&m_vkTriangleIndexBuffer,
				&m_vkTriangleIndexBufferDeviceMemory
				);

			copyBuffer(stagingBuffer, m_vkTriangleIndexBuffer, (VkDeviceSize)triangleIndexBufferSize);

			vkDestroyBuffer(m_vkLogicalDeviceData.vkHandle, stagingBuffer, nullptr);
			vkFreeMemory(m_vkLogicalDeviceData.vkHandle, stagingBufferDeviceMemory, nullptr);
		}
	}
	void Wrapper::deinitFSQuadBuffers()
	{
		vkDestroyBuffer(m_vkLogicalDeviceData.vkHandle, m_vkTriangleIndexBuffer, nullptr);
		vkFreeMemory(m_vkLogicalDeviceData.vkHandle, m_vkTriangleIndexBufferDeviceMemory, nullptr);

		vkDestroyBuffer(m_vkLogicalDeviceData.vkHandle, m_vkTriangleVertexBuffer, nullptr);
		vkFreeMemory(m_vkLogicalDeviceData.vkHandle, m_vkTriangleVertexBufferDeviceMemory, nullptr);
	}

	void Wrapper::initDescriptorSetLayout()
	{
		const uint32_t numBindings = 2;
		VkDescriptorSetLayoutBinding bindings[numBindings];

		VkDescriptorSetLayoutBinding & uboDescriptorSetLayoutBinding = bindings[0];
		uboDescriptorSetLayoutBinding = { };
		uboDescriptorSetLayoutBinding.binding = 0;
		uboDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboDescriptorSetLayoutBinding.descriptorCount = 1;
		uboDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uboDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding & samplerDescriptorSetLayoutBinding = bindings[1];
		samplerDescriptorSetLayoutBinding = { };
		samplerDescriptorSetLayoutBinding.binding = 1;
		samplerDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerDescriptorSetLayoutBinding.descriptorCount = 1;
		samplerDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerDescriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = numBindings;
		descriptorSetLayoutCreateInfo.pBindings = bindings;

		VkResult result = vkCreateDescriptorSetLayout(
								m_vkLogicalDeviceData.vkHandle,
								&descriptorSetLayoutCreateInfo,
								nullptr,
								&m_vkUBODescriptorSetLayout
								);
		if (result != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create UBO descriptor set layout!\n");
		}
	}
	void Wrapper::deinitDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(m_vkLogicalDeviceData.vkHandle, m_vkUBODescriptorSetLayout, nullptr);
	}

	void Wrapper::initUBO()
	{
		size_t bufferSize = sizeof(UniformBufferObject);
		createBuffer(
			m_vkPhysicalDeviceData.vkHandle,
			m_vkLogicalDeviceData.vkHandle,
			(VkDeviceSize)bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_vkUBOBuffer,
			&m_vkUBOBufferDeviceMemory
			);
	}
	void Wrapper::deinitUBO()
	{
		vkDestroyBuffer(m_vkLogicalDeviceData.vkHandle, m_vkUBOBuffer, nullptr);
		vkFreeMemory(m_vkLogicalDeviceData.vkHandle, m_vkUBOBufferDeviceMemory, nullptr);
	}

	void Wrapper::initDescriptorPool()
	{
		const uint32_t descriptorPoolSizesNum = 2;
		VkDescriptorPoolSize descriptorPoolSizes[descriptorPoolSizesNum];

		VkDescriptorPoolSize & uboDescriptorPoolSize = descriptorPoolSizes[0];
		uboDescriptorPoolSize = { };
		uboDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboDescriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolSize & samplerDescriptorPoolSize = descriptorPoolSizes[1];
		samplerDescriptorPoolSize = { };
		samplerDescriptorPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerDescriptorPoolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.flags = (VkDescriptorPoolCreateFlags)0;
		descriptorPoolCreateInfo.maxSets = 1;
		descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSizesNum;
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

		VkResult result = vkCreateDescriptorPool(
								m_vkLogicalDeviceData.vkHandle,
								&descriptorPoolCreateInfo,
								nullptr,
								&m_vkDescriptorPool
								);
		if (result != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create descriptor pool!\n");
		}
	}
	void Wrapper::deinitDescriptorPool()
	{
		vkDestroyDescriptorPool(m_vkLogicalDeviceData.vkHandle, m_vkDescriptorPool, nullptr);
	}

	void Wrapper::initDescriptorSet()
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.descriptorPool = m_vkDescriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &m_vkUBODescriptorSetLayout;

		VkResult result = vkAllocateDescriptorSets(
								m_vkLogicalDeviceData.vkHandle,
								&descriptorSetAllocateInfo,
								&m_vkDescriptorSet
								);
		if (result != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to allocate descriptor set!\n");
		}

		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = m_vkUBOBuffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = (VkDeviceSize)sizeof(UniformBufferObject);

		VkDescriptorImageInfo descriptorImageInfo = {};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = m_vkTextureImageView;
		descriptorImageInfo.sampler = m_vkTextureSampler;

		const uint32_t writeDescriptorSetsNum = 2;
		VkWriteDescriptorSet writeDescriptorSets[writeDescriptorSetsNum];

		VkWriteDescriptorSet & uboWriteDescriptorSet = writeDescriptorSets[0];
		uboWriteDescriptorSet = { };
		uboWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboWriteDescriptorSet.dstSet = m_vkDescriptorSet;
		uboWriteDescriptorSet.dstBinding = 0;
		uboWriteDescriptorSet.dstArrayElement = 0;
		uboWriteDescriptorSet.descriptorCount = 1;
		uboWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboWriteDescriptorSet.pImageInfo = nullptr;
		uboWriteDescriptorSet.pBufferInfo = &descriptorBufferInfo;
		uboWriteDescriptorSet.pTexelBufferView = nullptr;

		VkWriteDescriptorSet & samplerWriteDescriptorSet = writeDescriptorSets[1];
		samplerWriteDescriptorSet = { };
		samplerWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		samplerWriteDescriptorSet.dstSet = m_vkDescriptorSet;
		samplerWriteDescriptorSet.dstBinding = 1;
		samplerWriteDescriptorSet.dstArrayElement = 0;
		samplerWriteDescriptorSet.descriptorCount = 1;
		samplerWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerWriteDescriptorSet.pImageInfo = &descriptorImageInfo;
		samplerWriteDescriptorSet.pBufferInfo = nullptr;
		samplerWriteDescriptorSet.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(m_vkLogicalDeviceData.vkHandle, writeDescriptorSetsNum, writeDescriptorSets, 0, nullptr);
	}
	void Wrapper::deinitDescriptorSet()
	{
		// No need to explicitly deallocate descriptor set since its lifetime
		//	is equal to lifetime of the descrptor set pool
		//vkFreeDescriptorSets(m_vkLogicalDeviceData.vkHandle, m_vkDescriptorPool, 1, &m_vkDescriptorSet);
	}

	void Wrapper::buildCommandBuffers()
	{
		// Command buffer outputs to a certain image, and since swapchain has several of them - we need several command buffers
		m_vkCommandBuffers.resize(m_vkSwapchainData.framebuffers.size());

		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = m_vkCommandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

		if (vkAllocateCommandBuffers(m_vkLogicalDeviceData.vkHandle, &commandBufferAllocateInfo, m_vkCommandBuffers.data()) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to allocate command buffers!\n");
		}

		for (size_t i = 0, iend = m_vkCommandBuffers.size(); i < iend; ++i)
		{
			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			commandBufferBeginInfo.pInheritanceInfo = nullptr;

			vkBeginCommandBuffer(m_vkCommandBuffers[i], &commandBufferBeginInfo);

			VkClearValue clearColor = { 0.1f, 0.2f, 0.4f, 1.0f };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = m_vkRenderPass;
			renderPassBeginInfo.framebuffer = m_vkSwapchainData.framebuffers[i];
			renderPassBeginInfo.renderArea.offset = { 0, 0 };
			renderPassBeginInfo.renderArea.extent = m_vkSwapchainData.extent;
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(m_vkCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_vkCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

			VkBuffer vertexBuffers[] = { m_vkTriangleVertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_vkCommandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_vkCommandBuffers[i], m_vkTriangleIndexBuffer, 0, m_vkTriangleIndexBufferType);

			vkCmdBindDescriptorSets(
				m_vkCommandBuffers[i],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_vkPipelineLayout,
				0,
				1,
				&m_vkDescriptorSet,
				0,
				nullptr
				);

			vkCmdDrawIndexed(m_vkCommandBuffers[i], (uint32_t)m_vkTriangleIndicesCount, 1, 0, 0, 0);

			vkCmdEndRenderPass(m_vkCommandBuffers[i]);

			if (vkEndCommandBuffer(m_vkCommandBuffers[i]) != VK_SUCCESS)
			{
				// TODO: error
				printf("Failed to record command buffer %zd!\n", i);
			}
		}
	}
	void Wrapper::destroyCommandBuffers()
	{
		vkFreeCommandBuffers(m_vkLogicalDeviceData.vkHandle, m_vkCommandPool, (uint32_t)m_vkCommandBuffers.size(), m_vkCommandBuffers.data());
		m_vkCommandBuffers.resize(0);
	}

	void Wrapper::initSemaphores()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(m_vkLogicalDeviceData.vkHandle, &semaphoreCreateInfo, nullptr, &m_vkSemaphoreImageAvailable) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create \"image available\" semaphore!\n");
		}
		if (vkCreateSemaphore(m_vkLogicalDeviceData.vkHandle, &semaphoreCreateInfo, nullptr, &m_vkSemaphoreRenderFinished) != VK_SUCCESS)
		{
			// TODO: error
			printf("Failed to create \"render finished\" semaphore!\n");
		}
	}
	
	void Wrapper::deinitSemaphores()
	{
		vkDestroySemaphore(m_vkLogicalDeviceData.vkHandle, m_vkSemaphoreRenderFinished, nullptr);
		vkDestroySemaphore(m_vkLogicalDeviceData.vkHandle, m_vkSemaphoreImageAvailable, nullptr);
	}

	void Wrapper::init(HWND hWnd, int width, int height)
	{
		m_hWnd = hWnd;
		m_windowWidth = width;
		m_windowHeight = height;

		buildRequiredInstanceExtensionsList(true);
		buildSupportedInstanceExtensionsList(true);

#ifdef NDEBUG
		m_enableValidationLayers = false;
#else
		m_enableValidationLayers = true;
#endif
		bool requiresDebugCallback = initValidationLayers(m_enableValidationLayers);

		initInstance();

		if (requiresDebugCallback)
		{
			initDebugCallback(m_vkDebugCallback);
		}

		initWindowSurface(hWnd);

		selectPhysicalDevice();

		initLogicalDevice();
		
		initSwapchain();

		std::vector<char> vertShaderByteCode = readShaderFile("shaders/bin/test.vs.spv");
		std::vector<char> fragShaderByteCode = readShaderFile("shaders/bin/pathtracer.fs.spv");

		VkShaderModule vertShaderModule = initShaderModule(vertShaderByteCode);
		VkShaderModule fragShaderModule = initShaderModule(fragShaderByteCode);

		m_vkShaderModules.push_back(vertShaderModule);
		m_vkShaderModules.push_back(fragShaderModule);

		initRenderPass();
		initDescriptorSetLayout();
		initCommandPool();
		initUBO();
		initTextureImage();
		initTextureImageView();
		initTextureSampler();
		initDescriptorPool();
		initDescriptorSet();
		initPipelineState();

		initSwapchainFramebuffers();

		initFSQuadBuffers();
		buildCommandBuffers();

		initSemaphores();
	}

	void Wrapper::deinit()
	{
		// Wait before the last frame is fully rendered
		vkDeviceWaitIdle(m_vkLogicalDeviceData.vkHandle);

		deinitSemaphores();

		// No need to call destroyCommandBuffers as this will be done automatically by Vulkan on command pool deinitialization
		deinitFSQuadBuffers();

		deinitSwapchainFramebuffers();
		deinitPipelineState();
		deinitDescriptorSet();
		deinitDescriptorPool();
		deinitTextureSampler();
		deinitTextureImageView();
		deinitTextureImage();
		deinitUBO();
		deinitCommandPool();
		deinitDescriptorSetLayout();
		deinitRenderPass();
		deinitShaderModules();
		deinitSwapchain();
		deinitLogicalDevice();
		deinitDebugCallback();
		deinitWindowSurface();
		deinitInstance();
	}

	void Wrapper::update(double dtMS)
	{
		m_elapsedTimeMS += dtMS;
		UniformBufferObject ubo = {};
		ubo.time = (float)m_elapsedTimeMS;

		void * data;
		vkMapMemory(m_vkLogicalDeviceData.vkHandle, m_vkUBOBufferDeviceMemory, 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(m_vkLogicalDeviceData.vkHandle, m_vkUBOBufferDeviceMemory);
	}

	void Wrapper::render()
	{
		// Sync to the presenting queue here in order to allow CPU/GPU code to overlap
		vkQueueWaitIdle(m_vkLogicalDeviceData.presentingQueue);

		uint32_t imageIndexInSwapchain;

		{
			VkResult result = vkAcquireNextImageKHR(m_vkLogicalDeviceData.vkHandle, m_vkSwapchainData.vkHandle, std::numeric_limits<uint64_t>::max(), m_vkSemaphoreImageAvailable, VK_NULL_HANDLE, &imageIndexInSwapchain);

			// VK_SUBOPTIMAL_KHR can be reported here, and it is not exactly a very bad thing, so no actions on that at the moment
			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				reinitSwapchain();
				printf("Swapchain out of date!\n");
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			{
				// TODO: error
				printf("Failed to acquire next image!\n");
				return;
			}
		}

		VkSemaphore renderBegSemaphore[] = { m_vkSemaphoreImageAvailable };
		VkSemaphore renderEndSemaphore[] = { m_vkSemaphoreRenderFinished };
		VkPipelineStageFlags pipelineWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = renderBegSemaphore;
		submitInfo.pWaitDstStageMask = pipelineWaitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_vkCommandBuffers[imageIndexInSwapchain];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = renderEndSemaphore;

		if (vkQueueSubmit(m_vkLogicalDeviceData.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		{
			// TODO: warning
			printf("Failed to submit draw command buffer!\n");
		}

		VkSwapchainKHR swapChains[] = { m_vkSwapchainData.vkHandle };

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = renderEndSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndexInSwapchain;
		presentInfo.pResults = nullptr;

		{
			VkResult result = vkQueuePresentKHR(m_vkLogicalDeviceData.presentingQueue, &presentInfo);

			// VK_SUBOPTIMAL_KHR can be reported here, and it is not exactly a very bad thing, so no actions on that at the moment
			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				reinitSwapchain();
				printf("Swapchain out of date on present!\n");
				return;
			}
			else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
			{
				// TODO: error
				printf("Failed to present image!\n");
				return;
			}
		}
	}

}