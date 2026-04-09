#include "gpch.h"
#include "RayTracingScene.h"
#include "GraphicsDevice.h"
#include "Material/Material.h"

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

void RayTracingScene::RegisterShadingPipeline(const Material* material)
{
	auto hash = material->GetSurfaceShaderHash();
	mHitGroupRegistry.Register(hash);
}

const HitGroupRegistry& RayTracingScene::GetRegistry() const
{
	return mHitGroupRegistry;
}

HitGroupRegistry::HitGroupRegistry()
{

}

uint32_t HitGroupRegistry::Register(uint32_t surfaceHash)
{
	auto it = mHashToIndex.find(surfaceHash);
	if (it != mHashToIndex.end())
	{
		return it->second;
	}

	uint32_t hitGroupIndex = (uint32_t)mHashToIndex.size();
	it = mHashToIndex.emplace_hint(mHashToIndex.end(), surfaceHash, hitGroupIndex);
	return hitGroupIndex;
}

uint32_t HitGroupRegistry::GetIndex(uint32_t surfaceHash) const
{
	auto it = mHashToIndex.find(surfaceHash);
	if (it != mHashToIndex.end())
	{
		return it->second;
	}
	return ~0u;
}

bool HitGroupRegistry::Contains(uint32_t surfaceHash) const
{
	return mHashToIndex.find(surfaceHash) != mHashToIndex.end();
}

HitGroupTable::HitGroupTable(const RayTracingScene* rtScene)
	: mRegistry(rtScene->GetRegistry())
{

}

HitGroupTable& HitGroupTable::AddPrimaryRay(uint32_t registryHash, HitGroupDescriptor&& descriptor)
{
	AddRay(registryHash, eastl::move(descriptor), RayType::PrimaryRay);
	return *this;
}

HitGroupTable& HitGroupTable::AddShadowRay(uint32_t registryHash, HitGroupDescriptor&& descriptor)
{
	AddRay(registryHash, eastl::move(descriptor), RayType::ShadowRay);
	return *this;
}

void HitGroupTable::AddRay(uint32_t registryHash, HitGroupDescriptor&& descriptor, RayType rayType)
{
	uint32_t hitGroupIndex = mRegistry.get().GetIndex(registryHash);
	GLEAM_ASSERT(hitGroupIndex != ~0u, "Material is not registered to RayTracingScene.");

	uint32_t baseIndex = hitGroupIndex * (uint32_t)RayType::COUNT;
	if (baseIndex + (uint32_t)RayType::COUNT >= mHitGroups.size())
	{
		mHitGroups.resize(baseIndex + (uint32_t)RayType::COUNT);
	}
	auto& hitGroup = mHitGroups[baseIndex + (uint32_t)rayType] = eastl::move(descriptor);
	hitGroup.name = hitGroup.name + (rayType == RayType::PrimaryRay ? "Primary" : "Shadow");
}

bool HitGroupTable::Contains(uint32_t registryHash, RayType rayType) const
{
	uint32_t hitGroupIndex = mRegistry.get().GetIndex(registryHash);
	if (hitGroupIndex == ~0u)
	{
		return false;
	}

	uint32_t baseIndex = hitGroupIndex * (uint32_t)RayType::COUNT;
	if (baseIndex + (uint32_t)RayType::COUNT >= mHitGroups.size())
	{
		return false;
	}
	return not mHitGroups[baseIndex + (uint32_t)rayType].name.empty();
}

const TArray<HitGroupDescriptor>& HitGroupTable::GetDescriptors() const
{
	return mHitGroups;
}
