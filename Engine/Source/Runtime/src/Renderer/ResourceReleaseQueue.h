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

	void BeginFrame();

	using ObjectDeallocator = std::function<void()>;
	void AddResource(ObjectDeallocator&& deallocator);

private:

	struct PendingRelease
	{
		uint64_t frame = 0;
		ObjectDeallocator deallocator;
	};

	TArray<PendingRelease> mPendingReleases;
	uint64_t mFrame = 0;
	uint32_t mFramesInFlight = 0;

};

} // namespace Gleam
