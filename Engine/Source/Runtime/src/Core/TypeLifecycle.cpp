#include "gpch.h"
#include "TypeLifecycle.h"

namespace Gleam {

void TypeLifecycleRegistry::Initialize()
{
	sInstance = this;
}

void TypeLifecycleRegistry::Shutdown()
{
	for (auto& [typeHash, lifecycle] : mLifecycles)
	{
		delete lifecycle;
	}
	mLifecycles.clear();
	mVectors.clear();
	sInstance = nullptr;
}

const TypeLifecycle* TypeLifecycleRegistry::Find(uint32_t typeHash) const
{
	auto it = mLifecycles.find(typeHash);
	if (it != mLifecycles.end())
	{
		return it->second;
	}
	return nullptr;
}

const VectorLifecycle* TypeLifecycleRegistry::FindVector(uint32_t typeHash) const
{
	auto it = mVectors.find(typeHash);
	if (it != mVectors.end())
	{
		return it->second;
	}
	return nullptr;
}

} // namespace Gleam
