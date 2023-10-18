#include "gpch.h"
#include "Heap.h"
#include "Buffer.h"
#include "CommandBuffer.h"

using namespace Gleam;

Buffer::Buffer(NativeGraphicsHandle handle, const BufferDescriptor& descriptor, void* contents)
    : GraphicsObject(handle), mDescriptor(descriptor), mContents(contents)
{

}

void* Buffer::GetContents() const
{
    return mContents;
}

const BufferDescriptor& Buffer::GetDescriptor() const
{
    return mDescriptor;
}
