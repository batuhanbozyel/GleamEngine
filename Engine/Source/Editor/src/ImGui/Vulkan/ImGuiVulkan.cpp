#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Vulkan/VulkanDevice.h"
#include "Renderer/Vulkan/VulkanPipelineStateManager.h"

#include "ImGui/imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

static VkDescriptorPool gDescriptorPool = VK_NULL_HANDLE;
static VkRenderPass gRenderPass = VK_NULL_HANDLE;

void ImGuiBackend::Init()
{
	// Create descriptor pool
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	VK_CHECK(vkCreateDescriptorPool(Gleam::VulkanDevice::GetHandle(), &pool_info, nullptr, &gDescriptorPool));

	// Create render pass
	VkAttachmentDescription attachment = {};
	attachment.format = Gleam::TextureFormatToVkFormat(Gleam::VulkanDevice::GetSwapchain().GetFormat());
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment = {};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;
	VK_CHECK(vkCreateRenderPass(Gleam::VulkanDevice::GetHandle(), &info, nullptr, &gRenderPass));

	// Initialize ImGui
	ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = Gleam::VulkanDevice::GetInstance();
    init_info.PhysicalDevice = Gleam::VulkanDevice::GetPhysicalDevice();
    init_info.Device = Gleam::VulkanDevice::GetHandle();
    init_info.QueueFamily = Gleam::VulkanDevice::GetGraphicsQueue().index;
    init_info.Queue = Gleam::VulkanDevice::GetGraphicsQueue().handle;
    init_info.PipelineCache = Gleam::VulkanDevice::GetPipelineCache();
	init_info.DescriptorPool = gDescriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = Gleam::VulkanDevice::GetSwapchain().GetFramesInFlight();
    init_info.ImageCount = Gleam::VulkanDevice::GetSwapchain().GetFramesInFlight();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult result)
	{
		GLEAM_ASSERT(result == VK_SUCCESS, VkResultToString(x));;
	};

	ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void*) { return vkGetInstanceProcAddr(Gleam::VulkanDevice::GetInstance(), function_name); });
	ImGui_ImplSDL3_InitForVulkan(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
    ImGui_ImplVulkan_Init(&init_info, gRenderPass);

	Gleam::CommandBuffer cmd;
	cmd.Begin();
	ImGui_ImplVulkan_CreateFontsTexture(Gleam::As<VkCommandBuffer>(cmd.GetHandle()));
	cmd.End();
	cmd.Commit();
	cmd.WaitUntilCompleted();
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void ImGuiBackend::Destroy()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();

	vkDestroyDescriptorPool(Gleam::VulkanDevice::GetHandle(), gDescriptorPool, nullptr);
	vkDestroyRenderPass(Gleam::VulkanDevice::GetHandle(), gRenderPass, nullptr);
}

void ImGuiBackend::BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderPass)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Gleam::As<VkCommandBuffer>(commandBuffer));
}

ImTextureID ImGuiBackend::GetImTextureIDForTexture(const Gleam::Texture& texture)
{
	auto textureID = ImGui_ImplVulkan_AddTexture(Gleam::VulkanPipelineStateManager::GetSampler(0), Gleam::As<VkImageView>(texture.GetView()), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	Gleam::VulkanDevice::GetSwapchain().AddPooledObject(textureID, [](void* object)
	{
		ImGui_ImplVulkan_RemoveTexture(Gleam::As<VkDescriptorSet>(object));
	});
	return textureID;
}

#endif
