#pragma once
#include <Reflection/Macro.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace Gleam {

GSTRUCT(BufferRange, "7C2E9B14-A85F-4D6B-9E37-2F508C41D6A9", Serializable)
{
	GFIELD("3D8F25C1-6E9A-4B72-A1C8-5B94E07D3F26", Serializable)
	uint32_t offset = 0;

	GFIELD("9A46D8E2-1C73-4F58-B60D-8E25A19C74B3", Serializable)
	uint32_t size = 0;
};

GSTRUCT(BinaryBuffer, "D9C1A4E7-3B58-4F02-A6D9-71E8C5B2F4A3", Serializable)
{
	void* data = nullptr;
	size_t size = 0;

	BinaryBuffer() = default;

	explicit BinaryBuffer(size_t bufferSize)
		: data(bufferSize > 0 ? std::malloc(bufferSize) : nullptr)
		, size(bufferSize)
	{

	}

	BinaryBuffer(const void* src, size_t bufferSize)
		: BinaryBuffer(bufferSize)
	{
		if (bufferSize > 0)
		{
			std::memcpy(data, src, bufferSize);
		}
	}

	BinaryBuffer(const BinaryBuffer& other)
		: BinaryBuffer(other.data, other.size)
	{

	}

	BinaryBuffer(BinaryBuffer&& other) noexcept
		: data(other.data)
		, size(other.size)
	{
		other.data = nullptr;
		other.size = 0;
	}

	BinaryBuffer& operator=(const BinaryBuffer& other)
	{
		if (this != &other)
		{
			Resize(other.size);
			if (size > 0)
			{
				std::memcpy(data, other.data, size);
			}
		}
		return *this;
	}

	BinaryBuffer& operator=(BinaryBuffer&& other) noexcept
	{
		if (this != &other)
		{
			std::free(data);
			data = other.data;
			size = other.size;
			other.data = nullptr;
			other.size = 0;
		}
		return *this;
	}

	~BinaryBuffer()
	{
		std::free(data);
	}

	void Resize(size_t newSize)
	{
		if (newSize == 0)
		{
			std::free(data);
			data = nullptr;
			size = 0;
		}
		else if (newSize != size)
		{
			data = std::realloc(data, newSize);
			size = newSize;
		}
	}
};

} // namespace Gleam
