#include "gpch.h"
#include "ConstantBuffer.h"
#include "GraphicsDevice.h"

using namespace Gleam;

ConstantBuffer::ConstantBuffer(GraphicsDevice* device, size_t size)
	: mDevice(device)
{
	HeapDescriptor heapDesc;
	heapDesc.name = "ConstantBuffer::Heap";
	heapDesc.size = size;
	heapDesc.memoryType = MemoryType::CPU;
	mHeap = mDevice->CreateHeap(heapDesc);

	BufferDescriptor bufferDesc;
	bufferDesc.name = "Buffer";
	bufferDesc.size = size;
	mBuffer = mHeap.CreateBuffer(bufferDesc);
}

ConstantBuffer::~ConstantBuffer()
{
	mDevice->Dispose(mBuffer);
	mDevice->Dispose(mHeap);
}

size_t ConstantBuffer::Write(const void* data, size_t size)
{
	auto alignedStackPtr = Utils::AlignUp(mStackPtr, mHeap.GetAlignment());
	auto newStackPtr = alignedStackPtr + size;

	if (Utils::AlignUp(mHeap.GetDescriptor().size, mHeap.GetAlignment()) < newStackPtr)
	{
		GLEAM_ASSERT(false, "ConstantBuffer has reached its capacity");
		return 0;
	}
	mStackPtr = newStackPtr;

	auto dst = OffsetPointer(mBuffer.GetContents(), alignedStackPtr);
	memcpy(dst, data, size);
	return alignedStackPtr;
}

void ConstantBuffer::Reset()
{
	mStackPtr = 0;
}