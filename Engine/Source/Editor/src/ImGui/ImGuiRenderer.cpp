#include "ImGuiRenderer.h"
#include "View/ViewStack.h"

#include <imgui.h>

#include "backends/imgui_impl_sdl2.h"

using namespace GEditor;

void ImGuiRenderer::OnCreate(Gleam::RendererContext* context)
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
        ImGui_ImplSDL2_ProcessEvent(e);
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
		Gleam::RenderTextureHandle swapchainTarget;
	};
	graph.AddRenderPass<ImGuiPassData>("ImGui", [&](Gleam::RenderGraphBuilder& builder, ImGuiPassData& passData)
    {
        const auto& renderingData = blackboard.Get<Gleam::RenderingData>();
        passData.swapchainTarget = builder.UseColorBuffer({.texture = renderingData.swapchainTarget});
    },
    [this](const Gleam::RenderGraphContext& renderGraphContext, const ImGuiPassData& passData)
    {
	#ifdef USE_METAL_RENDERER
		@autoreleasepool
	#endif
		{
			mBackend->BeginFrame();
			ImGui::NewFrame();

			static bool dockspaceOpen = true;
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

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
			ImGui::Begin("Editor Dockspace", &dockspaceOpen, window_flags);
			ImGui::PopStyleVar();
            ImGui::PopStyleVar(2);

			ImGuiIO& io = ImGui::GetIO();
            ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			
            auto viewStack = GameInstance->GetSystem<ViewStack>();
			for (auto view : viewStack->GetViews())
			{
				view->Render();
			}
            
            ImGui::End();
			const auto& resolution = GameInstance->GetWindow()->GetResolution();
			io.DisplaySize = ImVec2(resolution.width, resolution.height);

            ImGui::Render();
			mBackend->EndFrame(renderGraphContext.cmd->GetHandle(), renderGraphContext.cmd->GetActiveRenderPass());
			
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
    });
}
