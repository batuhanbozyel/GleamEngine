#include "ImGuiRenderer.h"
#include "ImGuiBackend.h"
#include "View/ViewStack.h"

#include "backends/imgui_impl_sdl3.h"

using namespace GEditor;

void ImGuiRenderer::OnCreate(Gleam::GraphicsDevice* device)
{
    mDevice = device;
    
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    
	ImGuiBackend::Init(device);
    GameInstance->SetEventHandler([](const SDL_Event* e)
    {
        ImGui_ImplSDL3_ProcessEvent(e);
    });
}

void ImGuiRenderer::OnDestroy(Gleam::GraphicsDevice* device)
{
	ImGuiBackend::Destroy();
    ImGui::DestroyContext();
}

void ImGuiRenderer::AddRenderPasses(Gleam::RenderGraph& graph, Gleam::RenderGraphBlackboard& blackboard)
{
	struct ImGuiPassData
	{
		Gleam::TextureHandle sceneTarget;
		Gleam::TextureHandle swapchainTarget;
	};
    
	graph.AddRenderPass<ImGuiPassData>("ImGuiPass", [&](Gleam::RenderGraphBuilder& builder, ImGuiPassData& passData)
	{
		const auto& renderingData = blackboard.Get<Gleam::RenderingData>();
		auto swapchainTarget = graph.ImportBackbuffer(mDevice->GetSwapchain()->GetTexture());
		passData.sceneTarget = builder.ReadTexture(renderingData.backbuffer);
		passData.swapchainTarget = builder.UseColorBuffer(swapchainTarget);
	},
    [this](const Gleam::CommandBuffer* cmd, const ImGuiPassData& passData)
    {
        ImGuiIO& io = ImGui::GetIO();
        auto drawableSize = GameInstance->GetSubsystem<Gleam::RenderSystem>()->GetDevice()->GetSwapchain()->GetDrawableSize();
        io.DisplaySize = ImVec2(drawableSize.width, drawableSize.height);
        
		ImGuiBackend::BeginFrame();
        ImGui::NewFrame();
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Editor Dockspace", nullptr, window_flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);
        
        ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        auto viewStack = Gleam::World::active->GetSystem<ViewStack>();
        for (auto view : viewStack->GetViews())
        {
            view->Render();
        }
        
        ImGui::End();
        ImGui::Render();
		ImGuiBackend::EndFrame(cmd->GetHandle(), cmd->GetActiveRenderPass());
        
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    });
}
