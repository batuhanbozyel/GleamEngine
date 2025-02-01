#include "gpch.h"
#include "ConstantBuffer.h"
#include "GraphicsDevice.h"

using namespace Gleam;

#ifdef USE_DIRECTX_RENDERER
static constexpr size_t Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
#else
static constexpr size_t Alignment = 4;
#endif

ConstantBuffer::ConstantBuffer(GraphicsDevice* device, size_t size)
	: mDevice(device)
	, mCapacity(Utils::AlignUp(size, Alignment))
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
	auto ptr = Allocate(size);
	GLEAM_ASSERT(mCapacity > ptr, "ConstantBuffer has reached its capacity");

	auto dst = OffsetPointer(mBuffer.GetContents(), ptr);
	memcpy(dst, data, size);
	return ptr;
}

size_t ConstantBuffer::Allocate(size_t size)
{
	auto alignedStackPtr = Utils::AlignUp(mStackPtr, Alignment);
	auto newStackPtr = alignedStackPtr + size;
	GLEAM_ASSERT(mCapacity > newStackPtr, "ConstantBuffer has reached its capacity");

	mStackPtr = newStackPtr;
	return alignedStackPtr;
}

void ConstantBuffer::Reset()
{
	mStackPtr = 0;
}