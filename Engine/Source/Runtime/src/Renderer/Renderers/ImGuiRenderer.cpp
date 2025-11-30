#include "ImGuiRenderer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/EventSystem.h"
#include "Core/WindowSystem.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/ImGui/imgui_impl_sdl3.h"

using namespace Gleam;

static constexpr size_t kImGuiDataBufferSize = 4 * 1024 * 1024;

void ImGuiRenderer::OnCreate(RenderContext& context)
{
    mDevice = context.device;
	mReleaseQueue = context.releaseQueue;
	mSurface = static_cast<Swapchain*>(context.surface);
    
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
	ImGui_ImplSDL3_InitForOther(Globals::Engine->GetSubsystem<WindowSystem>()->GetSDLWindow());

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	Texture2DDescriptor textureDesc;
	textureDesc.name = "ImGui Font Texture";
	textureDesc.size.width = (float)width;
	textureDesc.size.height = (float)height;
	textureDesc.format = TextureFormat::R8G8B8A8_UNorm;
	textureDesc.pixels.resize(width * height * 4);
	memcpy(textureDesc.pixels.data(), pixels, textureDesc.pixels.size());
	mFontTexture = new Texture2D(textureDesc);

	uint64_t fontTextureId = static_cast<uint64_t>(mFontTexture->GetResourceView().data);
	io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(fontTextureId));
	
	GraphicsPipelineStateDescriptor pipelineDesc;
	pipelineDesc.blendState.enabled = true;
	pipelineDesc.blendState.sourceColorBlendMode = Gleam::BlendMode::SrcAlpha;
	pipelineDesc.blendState.destinationColorBlendMode = Gleam::BlendMode::OneMinusSrcAlpha;
	pipelineDesc.blendState.colorBlendOperation = Gleam::BlendOp::Add;
	pipelineDesc.blendState.alphaBlendOperation = Gleam::BlendOp::Add;
	pipelineDesc.blendState.sourceAlphaBlendMode = Gleam::BlendMode::One;
	pipelineDesc.blendState.destinationAlphaBlendMode = Gleam::BlendMode::OneMinusSrcAlpha;
	pipelineDesc.colorFormats = { Gleam::TextureFormat::B8G8R8A8_UNorm };
	pipelineDesc.vertexEntry = "imguiVertexShader";
	pipelineDesc.fragmentEntry = "imguiFragmentShader";
	mPipeline = mDevice->CreateGraphicsPipeline(pipelineDesc);
	mBuffer = mDevice->CreateBuffer(context.allocator, { .name = "ImGui RenderDrawData", .memoryType = MemoryType::CPU, .size = kImGuiDataBufferSize * mSurface->GetFramesInFlight() });

    Globals::Engine->GetSubsystem<EventSystem>()->SetEventHandler([](const SDL_Event* e)
    {
		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplSDL3_ProcessEvent(e);

		switch (e->type)
		{
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
			case SDL_EVENT_TEXT_INPUT:
			case SDL_EVENT_TEXT_EDITING:
			case SDL_EVENT_MOUSE_MOTION:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_WHEEL:
			{
				return io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput;
			}
		}

		return false;
    });
}

void ImGuiRenderer::OnDestroy(RenderContext& context)
{
	ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

	delete mFontTexture;
	mDevice->Dispose(context.allocator, mBuffer);
}

void ImGuiRenderer::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{    
	graph.AddRenderPass<ImGuiPassData>("ImGuiPass", [&](RenderGraphBuilder& builder, ImGuiPassData& passData)
	{
		const auto& sceneData = blackboard.Get<Gleam::SceneRenderingData>();
		passData.sceneTarget = builder.ReadTexture(sceneData.sceneTarget);
		passData.backbuffer = builder.WriteTexture(sceneData.backbuffer);
	},
    [this](const CommandBuffer* cmd, const ImGuiPassData& passData)
    {
        ImGuiIO& io = ImGui::GetIO();
        const auto& drawableSize = mSurface->GetSize();
        io.DisplaySize = ImVec2(drawableSize.width, drawableSize.height);
        
		ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Editor Dockspace", nullptr, windowFlags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);
        
        ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");
        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        for (auto& view : mViews)
        {
            std::invoke(view, passData);
        }
		mViews.clear();
        
        ImGui::End();
        ImGui::Render();

		void* bufferPtr = OffsetPointer(mBuffer.GetContents(), kImGuiDataBufferSize * mSurface->GetFrameIndex());
		ImDrawData* drawData = ImGui::GetDrawData();

		uint32_t vtxOffset = 0;
		ImDrawIdx* idxDest = (ImDrawIdx*)bufferPtr;
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* drawList = drawData->CmdLists[n];
			memcpy(idxDest, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
			idxDest += drawList->IdxBuffer.Size;
			vtxOffset += drawList->IdxBuffer.Size * sizeof(ImDrawIdx);
		}

		vtxOffset = (uint32_t)Utils::AlignUp(vtxOffset, mBuffer.GetAlignment());
		ImDrawVert* vtxDest = (ImDrawVert*)((char*)bufferPtr + vtxOffset);
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* drawList = drawData->CmdLists[n];
			memcpy(vtxDest, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
			vtxDest += drawList->VtxBuffer.Size;
		}

		Float4x4 projMatrix = Float4x4::Ortho(0, (float)mSurface->GetSize().width,
											  (float)mSurface->GetSize().height, 0,
											  -1, 1);

		RenderPassDescriptor renderPassDesc;
		renderPassDesc.size = mSurface->GetSize();
		renderPassDesc.colorAttachments.resize(1);
		renderPassDesc.colorAttachments[0].texture = passData.backbuffer;
		renderPassDesc.colorAttachments[0].loadAction = AttachmentLoadAction::Load;
		renderPassDesc.colorAttachments[0].storeAction = AttachmentStoreAction::Store;
		renderPassDesc.colorAttachments[0].clearColor = Color::clear;
		cmd->BeginRenderPass(renderPassDesc, "ImGuiPass");
		cmd->BindGraphicsPipeline(mPipeline);
		cmd->SetViewport(renderPassDesc.size);

		int globalVtxOffset = 0;
		int globalIdxOffset = 0;
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* drawList = drawData->CmdLists[n];
			for (int cmd_i = 0; cmd_i < drawList->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* drawCmd = &drawList->CmdBuffer[cmd_i];
				GLEAM_ASSERT(drawCmd->UserCallback == nullptr);

				Rect rect;
				rect.offset = { (drawCmd->ClipRect.x - drawData->DisplayPos.x) * drawData->FramebufferScale.x, (drawCmd->ClipRect.y - drawData->DisplayPos.y) * drawData->FramebufferScale.y };
				rect.size = { (drawCmd->ClipRect.z - drawCmd->ClipRect.x) * drawData->FramebufferScale.x, (drawCmd->ClipRect.w - drawCmd->ClipRect.y) * drawData->FramebufferScale.y };
				if (rect.size.width <= Math::Epsilon || rect.size.height <= Math::Epsilon)
				{
					continue;
				}

				uint64_t texID = reinterpret_cast<uint64_t>(drawCmd->GetTexID());
				ShaderResourceIndex texture = ShaderResourceIndex(static_cast<uint32_t>(texID));

				ImGuiResources passConstants;
				passConstants.projMatrix = projMatrix;
				passConstants.vertexOffset = kImGuiDataBufferSize * mSurface->GetFrameIndex() + static_cast<uint32_t>(vtxOffset + (globalVtxOffset * sizeof(ImDrawVert)));
				passConstants.vertexBuffer = mBuffer.GetResourceView();
				passConstants.texture = texture;

				cmd->SetScissorRect(rect);
				cmd->SetPushConstant(passConstants);
				cmd->DrawIndexed(mBuffer, IndexType::UINT16, drawCmd->ElemCount, 1, drawCmd->IdxOffset + globalIdxOffset, drawCmd->VtxOffset);
			}

			globalIdxOffset += drawList->IdxBuffer.Size;
			globalVtxOffset += drawList->VtxBuffer.Size;
		}
		cmd->EndRenderPass();
        
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    });
}

void ImGuiRenderer::PushView(ImGuiView&& view)
{
	mViews.emplace_back(std::move(view));
}

void ImGuiRenderer::AddFontTexture(const Path& fontPath, const Path& defaultPath, float fontSize)
{
	// TODO: error handling
	if (mFontTexture)
	{
		mReleaseQueue->AddResource([fontTexture = mFontTexture]()
		{
			delete fontTexture;
		}, mSurface->GetFrameIndex());
		mFontTexture = nullptr;
	}
	
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	io.Fonts->AddFontFromFileTTF(fontPath.String().c_str(), fontSize);
	io.FontDefault = io.Fonts->AddFontFromFileTTF(defaultPath.String().c_str(), fontSize);

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	Texture2DDescriptor textureDesc;
	textureDesc.name = "ImGui Font Texture";
	textureDesc.size.width = (float)width;
	textureDesc.size.height = (float)height;
	textureDesc.format = TextureFormat::R8G8B8A8_UNorm;
	textureDesc.pixels.resize(width * height * 4);
	memcpy(textureDesc.pixels.data(), pixels, textureDesc.pixels.size());
	mFontTexture = new Texture2D(textureDesc);

	uint64_t fontTextureId = static_cast<uint64_t>(mFontTexture->GetResourceView().data);
	io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(fontTextureId));
}

ImTextureID ImGuiRenderer::GetImTextureIDForTexture(const Texture& texture) const
{
	uint64_t id = static_cast<uint64_t>(texture.GetResourceView().data);
	return reinterpret_cast<ImTextureID>(id);
}