#include "gpch.h"
#include "ComponentStorage.h"
#include "Core/TypeLifecycle.h"

#include <EASTL/algorithm.h>
#include <cstring>

namespace Gleam {

ComponentStorage::ComponentStorage(const allocator_type& allocator)
	: Base(entt::type_id<ComponentStorageTag>(), entt::deletion_policy::swap_and_pop, allocator)
{

}

ComponentStorage::~ComponentStorage()
{
	DestructAll();
	ReleasePages(0);
}

void ComponentStorage::Initialize(const TypeLifecycle& lifecycle)
{
	mLifecycle = &lifecycle;
	mElementSize = lifecycle.GetSize();
	mAlignment = lifecycle.GetAlignment();
}

void* ComponentStorage::AssureAtLeast(size_t pos)
{
	const auto page = pos / PAGE_SIZE;
	while (mPages.size() <= page)
	{
		mPages.push_back(static_cast<std::byte*>(::operator new(PAGE_SIZE * mElementSize, std::align_val_t{ mAlignment })));
	}
	return ElementAt(pos);
}

void ComponentStorage::DestructAll()
{
	if (mLifecycle->IsTrivial())
	{
		return;
	}

	for (size_t pos = 0; pos < size(); ++pos)
	{
		mLifecycle->Destruct(ElementAt(pos));
	}
}

void ComponentStorage::ReleasePages(size_t first)
{
	for (size_t page = first; page < mPages.size(); ++page)
	{
		::operator delete(mPages[page], std::align_val_t{ mAlignment });
	}
	mPages.resize(first);
}

const void* ComponentStorage::get_at(size_t pos) const
{
	return ElementAt(pos);
}

void ComponentStorage::swap_or_move(size_t from, size_t to)
{
	// Swap and pop storages never hold tombstones, so this is always a swap
	auto lhs = ElementAt(from);
	auto rhs = ElementAt(to);
	if (mLifecycle->IsTrivial())
	{
		eastl::swap_ranges(static_cast<uint8_t*>(lhs), static_cast<uint8_t*>(lhs) + mElementSize, static_cast<uint8_t*>(rhs));
	}
	else
	{
		auto scratch = ::operator new(mElementSize, std::align_val_t{ mAlignment });
		mLifecycle->Move(scratch, lhs);
		mLifecycle->Destruct(lhs);
		mLifecycle->Move(lhs, rhs);
		mLifecycle->Destruct(rhs);
		mLifecycle->Move(rhs, scratch);
		mLifecycle->Destruct(scratch);
		::operator delete(scratch, std::align_val_t{ mAlignment });
	}
}

void ComponentStorage::pop(basic_iterator first, basic_iterator last)
{
	for (; first != last; ++first)
	{
		const auto entity = *first;
		mRemoved.publish(entity);

		// Listeners may have modified the storage, the element is looked up again like EnTT's signal mixin does
		const auto it = find(entity);
		auto elem = ElementAt(index(entity));
		auto back = ElementAt(size() - 1u);
		if (mLifecycle->IsTrivial())
		{
			if (elem != back)
			{
				std::memcpy(elem, back, mElementSize);
			}
		}
		else
		{
			mLifecycle->Destruct(elem);
			if (elem != back)
			{
				mLifecycle->Move(elem, back);
				mLifecycle->Destruct(back);
			}
		}
		swap_and_pop(it);
	}
}

void ComponentStorage::pop_all()
{
	for (const auto entity : static_cast<const Base&>(*this))
	{
		mRemoved.publish(entity);
	}

	DestructAll();
	Base::pop_all();
}

ComponentStorage::basic_iterator ComponentStorage::try_emplace(entt::entity entity, bool forceBack, const void* value)
{
	const auto it = Base::try_emplace(entity, forceBack);
	auto elem = AssureAtLeast(static_cast<size_t>(it.index()));
	if (value != nullptr)
	{
		mLifecycle->Copy(elem, value);
	}
	else
	{
		mLifecycle->Construct(elem);
	}
	return it;
}

void ComponentStorage::reserve(size_type capacity)
{
	if (capacity != 0)
	{
		Base::reserve(capacity);
		AssureAtLeast(capacity - 1u);
	}
}

ComponentStorage::size_type ComponentStorage::capacity() const noexcept
{
	return mPages.size() * PAGE_SIZE;
}

void ComponentStorage::shrink_to_fit()
{
	Base::shrink_to_fit();
	ReleasePages((size() + PAGE_SIZE - 1u) / PAGE_SIZE);
}

ComponentStorage& AssureComponentStorage(entt::registry& registry, uint32_t typeHash)
{
	auto& storage = registry.storage<ComponentStorageTag>(typeHash);
	if (storage.IsInitialized() == false)
	{
		const auto lifecycle = TypeLifecycleRegistry::GetInstance()->Find(typeHash);
		GLEAM_ASSERT(lifecycle != nullptr, "Component type has no registered lifecycle");
		storage.Initialize(*lifecycle);
	}
	return storage;
}

} // namespace Gleam
