#include "gpch.h"
#include "ResourceReleaseQueue.h"

using namespace Gleam;

ResourceReleaseQueue::ResourceReleaseQueue(uint32_t framesInFlight)
	: mReleaseQueue(framesInFlight)
{

}

ResourceReleaseQueue::~ResourceReleaseQueue()
{
	Clear();
	mReleaseQueue.clear();
}

void ResourceReleaseQueue::Clear()
{
	for (uint32_t i = 0; i < mReleaseQueue.size(); ++i)
	{
		Flush(i);
	}
}

void ResourceReleaseQueue::Flush(uint32_t frameIndex)
{
	auto& releaseQueue = mReleaseQueue[frameIndex];
	for (auto& deallocator : releaseQueue)
	{
		deallocator();
	}
	releaseQueue.clear();
}

void ResourceReleaseQueue::AddResource(ObjectDeallocator&& deallocator, uint32_t frameIndex)
{
	GLEAM_ASSERT(frameIndex < mReleaseQueue.size(), "Frame index is out of bounds.");
	mReleaseQueue[frameIndex].push_back(deallocator);
}