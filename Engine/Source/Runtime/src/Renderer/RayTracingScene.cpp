#include "gpch.h"
#include "RayTracingScene.h"
#include "GraphicsDevice.h"

using namespace Gleam;

RayTracingScene::RayTracingScene(GraphicsDevice* device, GPUAllocator* allocator)
	: mDevice(device)
	, mAllocator(allocator)
{
}

RayTracingScene::~RayTracingScene()
{
	if (mTLAS.IsValid())
	{
		mDevice->Dispose(mAllocator, mTLAS);
	}
}