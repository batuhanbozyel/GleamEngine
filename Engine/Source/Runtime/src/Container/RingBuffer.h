//
//  RingBuffer.h
//  GleamEngine
//

#pragma once
#include <EASTL/bonus/ring_buffer.h>
#include <EASTL/utility.h>

#include <new>

namespace Gleam {

template<typename T, typename Container = eastl::vector<T>, typename Allocator = typename Container::allocator_type>
using RingBuffer = eastl::ring_buffer<T, Container, Allocator>;

template<typename T, size_t count>
class FixedRingBuffer
{
	static_assert(count > 0, "FixedRingBuffer capacity must be greater than zero.");

public:

	FixedRingBuffer() = default;

	FixedRingBuffer(const FixedRingBuffer& other)
	{
		for (size_t i = 0; i < other.mSize; ++i)
		{
			emplace_back(other[i]);
		}
	}

	FixedRingBuffer(FixedRingBuffer&& other)
	{
		for (size_t i = 0; i < other.mSize; ++i)
		{
			emplace_back(eastl::move(other[i]));
		}
		other.clear();
	}

	FixedRingBuffer& operator=(const FixedRingBuffer& other)
	{
		if (this != &other)
		{
			clear();
			for (size_t i = 0; i < other.mSize; ++i)
			{
				emplace_back(other[i]);
			}
		}
		return *this;
	}

	FixedRingBuffer& operator=(FixedRingBuffer&& other)
	{
		if (this != &other)
		{
			clear();
			for (size_t i = 0; i < other.mSize; ++i)
			{
				emplace_back(eastl::move(other[i]));
			}
			other.clear();
		}
		return *this;
	}

	~FixedRingBuffer()
	{
		clear();
	}

	// Indexed from the oldest element
	T& operator[](size_t index)
	{
		return *Address(Wrap(mHead + index));
	}

	const T& operator[](size_t index) const
	{
		return *Address(Wrap(mHead + index));
	}

	T& front()
	{
		return (*this)[0];
	}

	const T& front() const
	{
		return (*this)[0];
	}

	T& back()
	{
		return (*this)[mSize - 1];
	}

	const T& back() const
	{
		return (*this)[mSize - 1];
	}

	template<typename ... Args>
	T& emplace_back(Args&& ... args)
	{
		if (full())
		{
			pop_front();
		}

		auto element = new (Address(Wrap(mHead + mSize))) T(eastl::forward<Args>(args)...);
		++mSize;
		return *element;
	}

	void push_back(const T& value)
	{
		emplace_back(value);
	}

	void push_back(T&& value)
	{
		emplace_back(eastl::move(value));
	}

	void pop_back()
	{
		--mSize;
		Destroy(Wrap(mHead + mSize));
	}

	void pop_front()
	{
		Destroy(mHead);
		mHead = Wrap(mHead + 1);
		--mSize;
	}

	void clear()
	{
		for (size_t i = 0; i < mSize; ++i)
		{
			Destroy(Wrap(mHead + i));
		}
		mHead = 0;
		mSize = 0;
	}

	bool empty() const
	{
		return mSize == 0;
	}

	bool full() const
	{
		return mSize == count;
	}

	size_t size() const
	{
		return mSize;
	}

	static constexpr size_t capacity()
	{
		return count;
	}

private:

	static size_t Wrap(size_t index)
	{
		return index % count;
	}

	T* Address(size_t index)
	{
		return reinterpret_cast<T*>(mStorage) + index;
	}

	const T* Address(size_t index) const
	{
		return reinterpret_cast<const T*>(mStorage) + index;
	}

	void Destroy(size_t index)
	{
		Address(index)->~T();
	}

	alignas(T) uint8_t mStorage[sizeof(T) * count];

	size_t mHead = 0;

	size_t mSize = 0;

};

} // namespace Gleam
