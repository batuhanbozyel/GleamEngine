//
//  ComposedLifecycle.cpp
//  Editor
//

#include "ComposedLifecycle.h"

#include <cstring>

using namespace GEditor;

static uint8_t* Offset(void* obj, size_t offset)
{
	return static_cast<uint8_t*>(obj) + offset;
}

static const uint8_t* Offset(const void* obj, size_t offset)
{
	return static_cast<const uint8_t*>(obj) + offset;
}

ComposedLifecycle::ComposedLifecycle(size_t size)
{
	mSize = size;
}

void ComposedLifecycle::Initialize(size_t alignment, const Gleam::TArray<Member>& members)
{
	mAlignment = alignment;

	mPrototype = ::operator new(mSize, std::align_val_t{ alignment });
	std::memset(mPrototype, 0, mSize);
	for (const auto& member : members)
	{
		member.lifecycle->Construct(Offset(mPrototype, member.offset));
		if (member.lifecycle->IsTrivial() == false)
		{
			mNonTrivialMembers.push_back(member);
		}
	}
	mTrivial = mNonTrivialMembers.empty();
}

void ComposedLifecycle::ReleasePrototype()
{
	Destruct(mPrototype);
	::operator delete(mPrototype, std::align_val_t{ mAlignment });
	mPrototype = nullptr;
}

void ComposedLifecycle::Construct(void* obj) const
{
	Copy(obj, mPrototype);
}

void ComposedLifecycle::Destruct(void* obj) const
{
	for (auto it = mNonTrivialMembers.rbegin(); it != mNonTrivialMembers.rend(); ++it)
	{
		it->lifecycle->Destruct(Offset(obj, it->offset));
	}
}

void ComposedLifecycle::Copy(void* obj, const void* source) const
{
	std::memcpy(obj, source, mSize);
	for (const auto& member : mNonTrivialMembers)
	{
		member.lifecycle->Copy(Offset(obj, member.offset), Offset(source, member.offset));
	}
}

void ComposedLifecycle::Move(void* obj, void* source) const
{
	std::memcpy(obj, source, mSize);
	for (const auto& member : mNonTrivialMembers)
	{
		member.lifecycle->Move(Offset(obj, member.offset), Offset(source, member.offset));
	}
}
