//
//  RawVectorLifecycle.cpp
//  Editor
//

#include "RawVectorLifecycle.h"

#include <EASTL/vector.h>
#include <cstring>

using namespace GEditor;

using ByteVector = eastl::vector<uint8_t>;

struct RawVector : ByteVector
{
	using ByteVector::mpBegin;
	using ByteVector::mpEnd;
	using ByteVector::mCapacityAllocator;
};

static RawVector& AsRaw(void* obj)
{
	return *static_cast<RawVector*>(static_cast<ByteVector*>(obj));
}

static const RawVector& AsRaw(const void* obj)
{
	return *static_cast<const RawVector*>(static_cast<const ByteVector*>(obj));
}

RawVectorLifecycle::RawVectorLifecycle(const Gleam::TypeLifecycle& element)
	: mElement(element)
{
	mSize = sizeof(ByteVector);
	mAlignment = alignof(ByteVector);
	mTrivial = false;
}

void RawVectorLifecycle::Construct(void* obj) const
{
	new (obj) ByteVector();
}

void RawVectorLifecycle::Destruct(void* obj) const
{
	auto& vector = AsRaw(obj);
	DestructElements(vector.mpBegin, vector.mpEnd);
	static_cast<ByteVector&>(vector).~ByteVector();
}

void RawVectorLifecycle::Copy(void* obj, const void* source) const
{
	Construct(obj);

	const auto& src = AsRaw(source);
	const auto bytes = static_cast<size_t>(src.mpEnd - src.mpBegin);
	if (bytes == 0)
	{
		return;
	}

	auto& vector = AsRaw(obj);
	auto data = static_cast<uint8_t*>(eastl::allocate_memory(vector.get_allocator(), bytes, mElement.GetAlignment(), 0));
	if (mElement.IsTrivial())
	{
		std::memcpy(data, src.mpBegin, bytes);
	}
	else
	{
		const auto elementSize = mElement.GetSize();
		for (size_t offset = 0; offset < bytes; offset += elementSize)
		{
			mElement.Copy(data + offset, src.mpBegin + offset);
		}
	}

	vector.mpBegin = data;
	vector.mpEnd = data + bytes;
	vector.mCapacityAllocator.first() = data + bytes;
}

void RawVectorLifecycle::Move(void* obj, void* source) const
{
	new (obj) ByteVector(std::move(*static_cast<ByteVector*>(source)));
}

size_t RawVectorLifecycle::GetElementSize() const
{
	return mElement.GetSize();
}

void* RawVectorLifecycle::Data(void* obj) const
{
	return AsRaw(obj).mpBegin;
}

void RawVectorLifecycle::Reset(void* obj, size_t count) const
{
	auto& vector = AsRaw(obj);
	DestructElements(vector.mpBegin, vector.mpEnd);
	vector.mpEnd = vector.mpBegin;

	const auto elementSize = mElement.GetSize();
	const auto bytes = count * elementSize;
	const auto capacity = static_cast<size_t>(vector.mCapacityAllocator.first() - vector.mpBegin);
	if (capacity < bytes)
	{
		if (vector.mpBegin != nullptr)
		{
			EASTLFree(vector.get_allocator(), vector.mpBegin, capacity);
		}
		vector.mpBegin = static_cast<uint8_t*>(eastl::allocate_memory(vector.get_allocator(), bytes, mElement.GetAlignment(), 0));
		vector.mpEnd = vector.mpBegin;
		vector.mCapacityAllocator.first() = vector.mpBegin + bytes;
	}

	for (size_t offset = 0; offset < bytes; offset += elementSize)
	{
		mElement.Construct(vector.mpBegin + offset);
	}
	vector.mpEnd = vector.mpBegin + bytes;
}

void RawVectorLifecycle::DestructElements(void* begin, void* end) const
{
	if (mElement.IsTrivial())
	{
		return;
	}

	const auto elementSize = mElement.GetSize();
	for (auto elem = static_cast<uint8_t*>(begin); elem != end; elem += elementSize)
	{
		mElement.Destruct(elem);
	}
}
