#include "ImGui/ImGuiBackend.h"

using namespace GEditor;

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Vulkan/VulkanDevice.h"

#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_vulkan.h"

ImGuiBackend::ImGuiBackend()
{
	ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = Gleam::VulkanDevice::GetInstance();
    init_info.PhysicalDevice = Gleam::VulkanDevice::GetPhysicalDevice();
    init_info.Device = Gleam::VulkanDevice::GetHandle();
    init_info.QueueFamily = Gleam::VulkanDevice::GetGraphicsQueue().index;
    init_info.Queue = Gleam::VulkanDevice::GetGraphicsQueue().handle;
    init_info.PipelineCache = Gleam::VulkanDevice::GetPipelineCache();
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = Gleam::VulkanDevice::GetSwapchain().GetFramesInFlight();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult result)
	{
		GLEAM_ASSERT(result == VK_SUCCESS, VkResultToString(x));;
	};

	VkRenderPass renderPass;
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
	VK_CHECK(vkCreateRenderPass(Gleam::VulkanDevice::GetHandle(), &info, nullptr, &renderPass));

    ImGui_ImplVulkan_Init(&init_info, renderPass);
    ImGui_ImplSDL3_InitForVulkan(GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetSDLWindow());
}

ImGuiBackend::~ImGuiBackend()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
}

void ImGuiBackend::BeginFrame() const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
}

void ImGuiBackend::EndFrame(NativeGraphicsHandle commandBuffer, NativeGraphicsHandle renderPass) const
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Gleam::As<VkCommandBuffer>(commandBuffer));
}

#endif