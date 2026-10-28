//
//  ComposedLifecycle.h
//  Editor
//

#pragma once
#include "Core/TypeLifecycle.h"
#include "Container/Array.h"

namespace GEditor {

class ComposedLifecycle final : public Gleam::TypeLifecycle
{
public:

	struct Member
	{
		size_t offset = 0;
		const Gleam::TypeLifecycle* lifecycle = nullptr;
	};

	explicit ComposedLifecycle(size_t size);

	// Separate from construction so a type can be registered before its members resolve, TArray<Self> refers back to it
	void Initialize(size_t alignment, const Gleam::TArray<Member>& members);

	// Destroys the prototype through the member lifecycles, so it runs before any of them is unregistered
	void ReleasePrototype();

	void* GetPrototype() const
	{
		return mPrototype;
	}

	virtual void Construct(void* obj) const override;

	virtual void Destruct(void* obj) const override;

	virtual void Copy(void* obj, const void* source) const override;

	virtual void Move(void* obj, void* source) const override;

private:

	Gleam::TArray<Member> mNonTrivialMembers;

	void* mPrototype = nullptr;
};

} // namespace GEditor
