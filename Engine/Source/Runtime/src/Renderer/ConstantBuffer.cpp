#include "gpch.h"
#include "ConstantBuffer.h"
#include "GraphicsDevice.h"

using namespace Gleam;

size_t ConstantBuffer::Write(const void* data, size_t size)
{
	auto ptr = Allocate(size);
	GLEAM_ASSERT(mCapacity > ptr, "ConstantBuffer has reached its capacity");

	auto dst = OffsetPointer(mContents, ptr);
	memcpy(dst, data, size);
	return ptr;
}

size_t ConstantBuffer::Allocate(size_t size)
{
	auto alignedStackPtr = Utils::AlignUp(mStackPtr, mAlignment);
	auto newStackPtr = alignedStackPtr + size;
	GLEAM_ASSERT(mCapacity > newStackPtr, "ConstantBuffer has reached its capacity");

	mStackPtr = newStackPtr;
	return alignedStackPtr;
}

void ConstantBuffer::Reset()
{
	mStackPtr = 0;
}