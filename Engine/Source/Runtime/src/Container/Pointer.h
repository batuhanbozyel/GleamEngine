#pragma once
#include "Core/Macro.h"

#include <memory>
#include <optional>

namespace Gleam {

template<typename T>
using Scope = std::unique_ptr<T>;
    
template<typename T>
using Ref = std::reference_wrapper<T>;

template<typename T>
using RefCounted = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
using Optional = std::optional<T>;

static constexpr auto Null = std::nullopt;

class MemoryView
{
public:
	// Types
	using value_type = uint8_t;
	using pointer = value_type*;
	using const_pointer = const value_type*;
	using reference = value_type&;
	using const_reference = const value_type&;
	using iterator = pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	// Constructors
	constexpr MemoryView() noexcept = default;
	constexpr MemoryView(const void* data, size_type size) noexcept
		: mData(data), mSize(size)
	{
	}

	constexpr MemoryView(const MemoryView&) noexcept = default;
	constexpr MemoryView& operator=(const MemoryView&) noexcept = default;

	// Element access
	constexpr const_reference operator[](size_type pos) const noexcept
	{
		return static_cast<const_reference>(static_cast<const_pointer>(mData)[pos]);
	}

	constexpr const_reference at(size_type pos) const
	{
		GLEAM_ASSERT(pos < mSize, "MemoryView::at: pos out of range");
		return operator[](pos);
	}

	constexpr const_reference front() const noexcept
	{
		return operator[](0);
	}

	constexpr const_reference back() const noexcept
	{
		return operator[](mSize - 1);
	}

	constexpr const_pointer data() const noexcept
	{
		return static_cast<const_pointer>(mData);
	}

	constexpr size_type size() const noexcept
	{
		return mSize;
	}

	// Iterators
	constexpr const_iterator begin() const noexcept
	{
		return static_cast<const_iterator>(mData);
	}

	constexpr const_iterator end() const noexcept
	{
		return begin() + mSize;
	}

	constexpr const_iterator cbegin() const noexcept
	{
		return begin();
	}

	constexpr const_iterator cend() const noexcept
	{
		return end();
	}

	constexpr const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}

	constexpr const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}

	constexpr const_reverse_iterator crbegin() const noexcept
	{
		return rbegin();
	}

	constexpr const_reverse_iterator crend() const noexcept
	{
		return rend();
	}

	// Comparison
	int compare(MemoryView v) const noexcept
	{
		// early exit if the same memory is compared
		if (mData == v.mData && mSize == v.mSize) return 0;

		const size_type rlen = std::min(mSize, v.mSize);
		const int result = std::memcmp(mData, v.mData, rlen);
		if (result == 0)
		{
			if (mSize < v.mSize) return -1;
			if (mSize > v.mSize) return 1;
		}
		return result;
	}

private:

	const void* mData = nullptr;
	size_t mSize = 0;
};

template<typename T, typename ... Args>
NO_DISCARD FORCE_INLINE static constexpr Scope<T> CreateScope(Args&& ... args) noexcept
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename ... Args>
NO_DISCARD FORCE_INLINE static constexpr  RefCounted<T> CreateRef(Args&& ... args) noexcept
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

NO_DISCARD FORCE_INLINE static constexpr void* OffsetPointer(void* ptr, size_t offset)
{
    return static_cast<char*>(ptr) + offset;
}

NO_DISCARD FORCE_INLINE static constexpr const void* OffsetPointer(const void* ptr, size_t offset)
{
    return static_cast<const char*>(ptr) + offset;
}

// Non-member functions
FORCE_INLINE static bool operator==(MemoryView lhs, MemoryView rhs) noexcept
{
	return lhs.compare(rhs) == 0;
}

FORCE_INLINE static bool operator!=(MemoryView lhs, MemoryView rhs) noexcept
{
	return lhs.compare(rhs) != 0;
}

FORCE_INLINE static bool operator<(MemoryView lhs, MemoryView rhs) noexcept
{
	return lhs.compare(rhs) < 0;
}

FORCE_INLINE static bool operator<=(MemoryView lhs, MemoryView rhs) noexcept
{
	return lhs.compare(rhs) <= 0;
}

FORCE_INLINE static bool operator>(MemoryView lhs, MemoryView rhs) noexcept
{
	return lhs.compare(rhs) > 0;
}

FORCE_INLINE static bool operator>=(MemoryView lhs, MemoryView rhs) noexcept
{
	return lhs.compare(rhs) >= 0;
}

} // namespace Gleam