#include "gpch.h"
#include "ResourceReleaseQueue.h"

using namespace Gleam;

ResourceReleaseQueue::ResourceReleaseQueue(uint32_t framesInFlight)
	: mFramesInFlight(framesInFlight)
{

}

ResourceReleaseQueue::~ResourceReleaseQueue()
{
	Clear();
}

void ResourceReleaseQueue::Clear()
{
	for (auto& release : mPendingReleases)
	{
		release.deallocator();
	}
	mPendingReleases.clear();
}

void ResourceReleaseQueue::BeginFrame()
{
	++mFrame;

	auto it = mPendingReleases.begin();
	while (it != mPendingReleases.end() and it->frame + mFramesInFlight <= mFrame)
	{
		it->deallocator();
		++it;
	}
	mPendingReleases.erase(mPendingReleases.begin(), it);
}

void ResourceReleaseQueue::AddResource(ObjectDeallocator&& deallocator)
{
	mPendingReleases.push_back(PendingRelease{ .frame = mFrame, .deallocator = eastl::move(deallocator) });
}
