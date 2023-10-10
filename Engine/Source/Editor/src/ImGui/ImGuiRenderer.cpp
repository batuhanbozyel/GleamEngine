#include "ImGuiRenderer.h"
#include "View/ViewStack.h"

#include <imgui.h>

#include "backends/imgui_impl_sdl3.h"

using namespace GEditor;

void ImGuiRenderer::OnCreate(Gleam::RendererContext& context)
{
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    
	mBackend = Gleam::CreateScope<ImGuiBackend>();
    GameInstance->SetEventHandler([](const SDL_Event* e)
    {
        ImGui_ImplSDL3_ProcessEvent(e);
    });
}

void ImGuiRenderer::OnDestroy()
{
	mBackend.reset();
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
		auto swapchainTarget = graph.ImportBackbuffer(Gleam::Texture(), { .clearOnFirstUse = true });
		passData.sceneTarget = builder.ReadTexture(renderingData.backbuffer);
		passData.swapchainTarget = builder.UseColorBuffer(swapchainTarget);
	},
    [this](const Gleam::CommandBuffer* cmd, const ImGuiPassData& passData)
    {
        mBackend->BeginFrame();
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
        
        ImGuiIO& io = ImGui::GetIO();
        const auto& resolution = GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetResolution();
        io.DisplaySize = ImVec2(resolution.width, resolution.height);

        ImGui::Render();
        mBackend->EndFrame(cmd->GetHandle(), cmd->GetActiveRenderPass());
        
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    });
}
