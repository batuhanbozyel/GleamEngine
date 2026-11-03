#include "gpch.h"
#include "RenderPipeline.h"
#include "RenderSystem.h"
#include "Material/Material.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

RenderPipeline::RenderPipeline(const AssetReference& reference, const AssetHeader& header, const RenderPipelineDescriptor& descriptor)
	: Asset(reference, header)
{
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	for (const auto& guid : descriptor.renderers)
	{
		const auto classDesc = Reflection::IDatabase::GetInstance()->GetClass(guid);
		if (classDesc == nullptr)
		{
			GLEAM_CORE_ERROR("Render pipeline {0} references a missing renderer class: {1}", GetName(), guid.ToString());
			continue;
		}
		mRenderers.Insert(renderSystem->AcquireRenderer(*classDesc), classDesc->GetAttribute<Reflection::Attribute::Renderer>()->stage);
	}
	renderSystem->RegisterPipeline(this);

	// Built from scratch on load and on reload, so the materials already in use need their shading pipelines
	Globals::GameInstance->GetSubsystem<AssetManager>()->ForEachLoaded<Material>([this](const Material* material)
	{
		RegisterShadingPipeline(material);
	});
}

RenderPipeline::~RenderPipeline()
{
	auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	renderSystem->UnregisterPipeline(this);
	for (const auto& entry : mRenderers)
	{
		renderSystem->ReleaseRenderer(entry.renderer);
	}
}

void RenderPipeline::RegisterShadingPipeline(const Material* material) const
{
	for (const auto& entry : mRenderers)
	{
		entry.renderer->RegisterShadingPipeline(material);
	}
}
