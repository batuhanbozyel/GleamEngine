#pragma once
#include "Container/Array.h"

#include <functional>

namespace Gleam {

class ResourceReleaseQueue
{
public:

	ResourceReleaseQueue(uint32_t framesInFlight);

	~ResourceReleaseQueue();

	void Clear();

	void Flush(uint32_t frameIndex);

	using ObjectDeallocator = std::function<void()>;
	void AddResource(ObjectDeallocator&& deallocator, uint32_t frameIndex);

private:

	using ReleaseQueue = TArray<ObjectDeallocator>;
	TArray<ReleaseQueue> mReleaseQueue;

};

} // namespace Gleam
