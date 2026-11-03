#pragma once
#include "Renderer.h"

#include <EASTL/algorithm.h>

namespace Gleam {

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RendererList
{
	struct Entry
	{
		IRenderer* renderer = nullptr;
		RenderStage stage = RenderStage::Opaque;
	};
	using Container = TArray<Entry>;
public:

	void Insert(IRenderer* renderer, RenderStage stage)
	{
		auto it = eastl::find_if(mEntries.begin(), mEntries.end(), [stage](const Entry& entry)
		{
			return entry.stage > stage;
		});
		mEntries.insert(it, Entry{ .renderer = renderer, .stage = stage });
	}

	void Insert(const RendererList& other)
	{
		for (const auto& entry : other.mEntries)
		{
			Insert(entry.renderer, entry.stage);
		}
	}

	void Remove(IRenderer* renderer)
	{
		mEntries.erase(eastl::find_if(mEntries.begin(), mEntries.end(), [renderer](const Entry& entry)
		{
			return entry.renderer == renderer;
		}));
	}

	template<RendererType T>
	T* Find() const
	{
		auto it = eastl::find_if(mEntries.begin(), mEntries.end(), [](const Entry& entry)
		{
			return typeid(*entry.renderer) == typeid(T);
		});
		return it != mEntries.end() ? static_cast<T*>(it->renderer) : nullptr;
	}

	Container::const_iterator begin() const
	{
		return mEntries.begin();
	}

	Container::const_iterator end() const
	{
		return mEntries.end();
	}

private:

	Container mEntries;

};

} // namespace Gleam
