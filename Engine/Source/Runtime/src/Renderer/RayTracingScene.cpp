#include "gpch.h"
#include "RayTracingScene.h"
#include "GraphicsDevice.h"
#include "Material/MaterialDescriptor.h"

using namespace Gleam;

RayTracingScene::RayTracingScene(GraphicsDevice* device, GPUAllocator* allocator)
	: mDevice(device)
	, mAllocator(allocator)
	, mHitGroupRegistry()
{
}

RayTracingScene::~RayTracingScene()
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mTLAS);
	}
}

void RayTracingScene::ReleaseAccelerationStructure()
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mTLAS);
	}
}

void RayTracingScene::RegisterShadingPipeline(const MaterialDescriptor& material, uint32_t hash)
{
	if (mHitGroupRegistry.Contains(hash) == false)
	{
		HitGroupEntry entry;

		HitGroupDescriptor& shadingGroup = entry[(uint32_t)DispatchRayType::Shading];
		shadingGroup.name = "HitGroup::" + material.name + "::Shading";
		shadingGroup.closestHitEntry = material.surfaceShader + "::ClosestHit";
		if (material.blendState.enabled)
		{
			shadingGroup.anyHitEntry = material.surfaceShader + "::AnyHit";
		}

		HitGroupDescriptor& shadowGroup = entry[(uint32_t)DispatchRayType::Shadow];
		shadowGroup.name = "HitGroup::" + material.name + "::Shadow";
		if (material.blendState.enabled)
		{
			shadowGroup.anyHitEntry = material.surfaceShader + "::ShadowAnyHit";
		}
		mHitGroupRegistry.Register(hash, entry);
	}
}

HitGroupRegistry::HitGroupRegistry()
{

}

uint32_t HitGroupRegistry::Register(uint32_t materialHash, const HitGroupEntry& entry)
{
	auto it = mHashToIndex.find(materialHash);
	if (it != mHashToIndex.end())
	{
		return it->second;
	}

	uint32_t hitGroupIndex = (uint32_t)mHitGroups.size() / (uint32_t)entry.size();
	for (const auto& hitGroup : entry)
	{
		mHitGroups.push_back(hitGroup);
	}
	it = mHashToIndex.emplace_hint(mHashToIndex.end(), materialHash, hitGroupIndex);
	return it->second;
}

uint32_t HitGroupRegistry::GetIndex(uint32_t materialHash) const
{
	auto it = mHashToIndex.find(materialHash);
	if (it != mHashToIndex.end())
	{
		return it->second;
	}
	return ~0u;
}

bool HitGroupRegistry::Contains(uint32_t materialHash) const
{
	return mHashToIndex.find(materialHash) != mHashToIndex.end();
}
