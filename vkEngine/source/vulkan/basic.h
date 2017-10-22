#include <stdio.h>
#include <vector>
#include <algorithm>

#define NOMINMAX
#include <Windows.h>		// GetModuleHandle

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan\vulkan.h>

#include "math\vec2.h"
#include "math\vec3.h"
#include "math\vec4.h"

namespace vulkan
{
	struct Vertex
	{
		math::Vec3 pos;
		math::Vec4 col;
		math::Vec2 tc;
	};

	struct UniformBufferObject
	{
		float time;
	};

	class Wrapper
	{
	public:

		int m_windowWidth = -1, m_windowHeight = -1;

		void setWindowSize(int width, int height)
		{
			m_windowWidth = width;
			m_windowHeight = height;
		}

		VkInstance m_vkInstance = VK_NULL_HANDLE;

		VkSurfaceKHR m_vkPresentableSurface = VK_NULL_HANDLE;

		struct VulkanPhysicalDeviceData
		{
			VkPhysicalDevice vkHandle = VK_NULL_HANDLE;
			VkPhysicalDeviceFeatures deviceFeatures;

			std::vector<const char *> requiredExtensionNamesList;
			std::vector<VkExtensionProperties> supportedExtensionsProps;

			struct SurfaceInfo
			{
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> presentModes;
			};

			SurfaceInfo surfaceInfo; 
		};
		VulkanPhysicalDeviceData m_vkPhysicalDeviceData;

		struct VulkanLogicalDeviceData
		{
			VkDevice vkHandle = VK_NULL_HANDLE;

			VkQueue graphicsQueue = VK_NULL_HANDLE;
			VkQueue presentingQueue = VK_NULL_HANDLE;

			uint32_t graphicsQueueFamilyIndex = 0xFFffFFff;
			uint32_t presentingQueueFamilyIndex = 0xFFffFFff;
		};
		VulkanLogicalDeviceData m_vkLogicalDeviceData;

		struct VulkanSwapchainData
		{
			VkSwapchainKHR vkHandle = VK_NULL_HANDLE;
			VkFormat format;
			VkExtent2D extent;
			VkColorSpaceKHR colorSpace;
			std::vector<VkImage> images;
			std::vector<VkImageView> imageViews;
			std::vector<VkFramebuffer> framebuffers;
		};
		VulkanSwapchainData m_vkSwapchainData;
		
		VkRenderPass m_vkRenderPass = VK_NULL_HANDLE;
		VkPipelineLayout m_vkPipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_vkGraphicsPipeline = VK_NULL_HANDLE;

		std::vector<const char *> m_requiredExtensionNamesList;
		std::vector<VkExtensionProperties> m_supportedExtensionsProps;

		bool m_debugCallbackInitialized = false;
		VkDebugReportCallbackEXT m_debugCallbackDesc = VK_NULL_HANDLE;
		std::vector<const char *> m_requiredInstanceValidationLayerNamesList;
		std::vector<const char *> m_requiredLogDevValidationLayerNamesList;

		void buildSupportedInstanceExtensionsList(bool printList = false);

		void buildRequiredInstanceExtensionsList(bool printList = false);

		bool m_enableValidationLayers = false;
		bool initValidationLayers(bool areValidationLayersEnabled = false);

		void initDebugCallback(PFN_vkDebugReportCallbackEXT debugCallback);

		void deinitDebugCallback();

		void initInstance();
		void deinitInstance();

		// TODO: implement through getQueueFamilyIndex (request idx if required and check if it's valid)
		static bool checkQueuesPresence(const VkPhysicalDevice & physDev, const VkSurfaceKHR & surface, bool needsGraphics, bool needsPresent, bool needsMemoryTransfer, bool needsCompute);

		static VulkanPhysicalDeviceData::SurfaceInfo queryDeviceSurfaceInfo(const VkPhysicalDevice & physDev, const VkSurfaceKHR & surface);

		static bool checkPhysicalDevice(const VkPhysicalDevice & physDev, const std::vector<const char *> & requiredExtensionNamesList, const VkSurfaceKHR & surface);

		void selectPhysicalDevice();

		void initLogicalDevice();
		void deinitLogicalDevice();

		void initWindowSurface(HWND hWnd);
		void deinitWindowSurface();

		static void getGenericSupportedDeviceExtensionsList(const VkPhysicalDevice & physDev, std::vector<VkExtensionProperties> * supportedExtensionsProps);
		void buildSupportedDeviceExtensionsList(bool printList = false);

		void buildRequiredDeviceExtensionsList(bool printList = false);

		static VkExtent2D selectPresentableSurfaceExtents(const VkSurfaceCapabilitiesKHR & capabilities, uint32_t w, uint32_t h);

		static VkSurfaceFormatKHR selectPresentableSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & availableFormats);

		static VkPresentModeKHR selectPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes);

		void initSwapchain();
		void deinitSwapchain();

		void reinitSwapchain()
		{
			// At the moment, swapchain recreation requires full cease of rendering operations
			//	while it is possible to change swapchain mid-rendering, by keeping old swapchain for a while,
			//	and passing it to `VkSwapchainCreateInfoKHR` as well.
			vkDeviceWaitIdle(m_vkLogicalDeviceData.vkHandle);
		
			// Destroy everything swapchain-related
			// TODO: see `destroyCommandBuffers()` to avoid needless command pool destruction in the future
			deinitCommandPool();
			deinitSwapchainFramebuffers();
			deinitPipelineState();
			deinitRenderPass();
			deinitSwapchain();

			initSwapchain();
			initRenderPass();
			// Theoretically we could avoid recreating the whole pipeline by using the pipeline dynamic states
			initPipelineState();
			initSwapchainFramebuffers();
			// Theoretically, we could avoid command pool recreation by just removing all of the command lists
			//	that are related to the swap chains. But for now we'll go with the sweeping clean approach.
			initCommandPool();
			buildCommandBuffers();
		}

		void onWindowResize(int width, int height)
		{
			if (width == 0 || height == 0)
				return;

			reinitSwapchain();
			setWindowSize(width, height);
		}

		PFN_vkDebugReportCallbackEXT m_vkDebugCallback = nullptr;
		void setDebugCallback(PFN_vkDebugReportCallbackEXT debugCallback)
		{
			m_vkDebugCallback = debugCallback;
		}
		PFN_vkDebugReportCallbackEXT getDebugCallback() const
		{
			return m_vkDebugCallback;
		}

		std::vector<VkShaderModule> m_vkShaderModules;
		VkShaderModule initShaderModule(const std::vector<char> & shaderByteCode);
		void deinitShaderModules();

		void initRenderPass();
		void deinitRenderPass();

		static void getVertexInputDescriptions(VkVertexInputBindingDescription * bindingDescr, VkVertexInputAttributeDescription attribsDescr[], int numAttribs = 3);
		static uint32_t Wrapper::findMemoryType(const VkPhysicalDevice & physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties);

		void initPipelineState();
		void deinitPipelineState();

		void initSwapchainFramebuffers();
		void deinitSwapchainFramebuffers();

		VkCommandPool m_vkCommandPool;
		std::vector<VkCommandBuffer> m_vkCommandBuffers;

		void initCommandPool();
		void deinitCommandPool();

		int m_vkTriangleVerticesCount = -1;
		VkBuffer m_vkTriangleVertexBuffer;
		VkDeviceMemory m_vkTriangleVertexBufferDeviceMemory;

		int m_vkTriangleIndicesCount = -1;
		VkIndexType m_vkTriangleIndexBufferType;
		VkBuffer m_vkTriangleIndexBuffer;
		VkDeviceMemory m_vkTriangleIndexBufferDeviceMemory;

		VkCommandPool getTransientCommandPool();
		VkCommandBuffer beginTransientCommandBuffer();
		void endTransientCommandBuffer(const VkCommandBuffer & transientCommandBuffer);

		static void createBuffer(
						const VkPhysicalDevice & physDev,
						const VkDevice & logicDev,
						VkDeviceSize size,
						VkBufferUsageFlags usage,
						VkMemoryPropertyFlags memoryProperties,
						VkBuffer * buffer,
						VkDeviceMemory * bufferDeviceMemory
						);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		static void createImage(
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
						);
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldImageLayout, VkImageLayout newImageLayout);
		// Image layout should be transitioned to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		void copyBufferToImage(uint32_t width, uint32_t height, VkBuffer buffer, VkImage image);

		VkFormat m_vkTextureImageFormat;
		VkImage m_vkTextureImage;
		VkDeviceMemory m_vkTextureImageDeviceMemory;
		void initTextureImage();
		void deinitTextureImage();

		static VkImageView createImageView2D(
						const VkDevice & logicDev,
						VkImage image,
						VkFormat format
						);

		VkImageView m_vkTextureImageView;
		void initTextureImageView();
		void deinitTextureImageView();

		VkSampler m_vkTextureSampler;
		void initTextureSampler();
		void deinitTextureSampler();

		void initFSQuadBuffers();
		void deinitFSQuadBuffers();

		VkDescriptorSetLayout m_vkUBODescriptorSetLayout;
		void initDescriptorSetLayout();
		void deinitDescriptorSetLayout();

		VkBuffer m_vkUBOBuffer;
		VkDeviceMemory m_vkUBOBufferDeviceMemory;
		void initUBO();
		void deinitUBO();

		VkDescriptorPool m_vkDescriptorPool;
		void initDescriptorPool();
		void deinitDescriptorPool();

		VkDescriptorSet m_vkDescriptorSet;
		void initDescriptorSet();
		void deinitDescriptorSet();

		void buildCommandBuffers();
		void destroyCommandBuffers();

		void initSemaphores();
		void deinitSemaphores();

		HWND m_hWnd;
		void init(HWND hWnd, int width, int height);
		void deinit();

		bool m_isExitting = false;
		void setIsExitting(bool isExitting) { m_isExitting = isExitting; }
		bool getIsExitting() const { return m_isExitting; }

		VkSemaphore m_vkSemaphoreImageAvailable;
		VkSemaphore m_vkSemaphoreRenderFinished;

		double m_elapsedTimeMS = 0.0;
		void update(double dtMS);
		void render();

		static const int titleBufSize = 256;
		char title[titleBufSize];
		void setDTime(double dtimeMS)
		{
			sprintf_s(title, titleBufSize, "Test: %.1f (%.3f ms)", 1000.0 / dtimeMS, dtimeMS);
			SetWindowTextA(m_hWnd, title);
		}
	};
}