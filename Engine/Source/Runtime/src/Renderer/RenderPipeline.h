#pragma once
#include "Renderer.h"

namespace Gleam {

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RenderPipeline
{
	using Container = TArray<IRenderer*>;
public:

	RenderPipeline(const RenderContext& context)
		: mContext(context)
	{
		
	}

	~RenderPipeline()
	{
		for (auto* renderer : mOwnedRenderers)
		{
			renderer->OnDestroy(mContext);
			delete renderer;
		}
		mRenderers.clear();
		mOwnedRenderers.clear();
	}

	template<RendererType T, class...Args>
    T* AddRenderer(Args&&... args)
    {
        GLEAM_ASSERT(not HasRenderer<T>(), "Render pipeline already has the renderer!");
        auto renderer = new T(std::forward<Args>(args)...);
        mOwnedRenderers.push_back(renderer);
        renderer->OnCreate(mContext);
        InsertRenderer(renderer);
        return renderer;
    }

    template<RendererType T>
    void AddSharedRenderer(T* renderer)
    {
        GLEAM_ASSERT(not HasRenderer<T>(), "Render pipeline already has the renderer!");
        InsertRenderer(renderer);
    }
	
    template<RendererType T>
    void RemoveRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Render pipeline does not have the renderer!");
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* renderer)
        {
            return typeid(*renderer) == typeid(T);
        });

        if (it != mRenderers.end())
        {
            auto renderer = *it;
            auto ownedIt = std::find(mOwnedRenderers.begin(), mOwnedRenderers.end(), renderer);
            if (ownedIt != mOwnedRenderers.end())
            {
                renderer->OnDestroy(mContext);
                delete renderer;
                mOwnedRenderers.erase(ownedIt);
            }
			mRenderers.erase(it);
        }
    }

	template<RendererType T>
	void RemoveSharedRenderer(T* renderer = nullptr)
	{
		auto it = renderer
			? std::find(mRenderers.begin(), mRenderers.end(), static_cast<IRenderer*>(renderer))
			: std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* r)
		{
			return typeid(*r) == typeid(T);
		});

		GLEAM_ASSERT(it != mRenderers.end(), "Render pipeline does not have the renderer!");
		GLEAM_ASSERT(std::find(mOwnedRenderers.begin(), mOwnedRenderers.end(), *it) == mOwnedRenderers.end(),
			"Use RemoveRenderer() for owned renderers!");
		mRenderers.erase(it);
	}
    
    template<RendererType T>
    T* GetRenderer() const
    {
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* renderer)
        {
            return typeid(*renderer) == typeid(T);
        });
		
		if (it != mRenderers.end())
		{
			return static_cast<T*>(*it);
		}
		return nullptr;
    }
    
    template<RendererType T>
    bool HasRenderer() const
    {
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* renderer)
        {
            return typeid(*renderer) == typeid(T);
        });
        return it != mRenderers.end();
    }
    
    template<RendererType T>
    uint32_t GetIndexOf() const
    {
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* renderer)
        {
            return typeid(*renderer) == typeid(T);
        });
        return std::distance(mRenderers.begin(), it);
    }
	
	Container::iterator begin()
	{
		return mRenderers.begin();
	}
	
	Container::iterator end()
	{
		return mRenderers.end();
	}
	
	Container::const_iterator begin() const
	{
		return mRenderers.begin();
	}
	
	Container::const_iterator end() const
	{
		return mRenderers.end();
	}

private:
	
	void InsertRenderer(IRenderer* renderer)
	{
		auto stage = renderer->GetStage();
		auto it = mRenderers.begin();
		while (it != mRenderers.end() && (*it)->GetStage() <= stage)
		{
			++it;
		}
		mRenderers.insert(it, renderer);
	}

	Container mRenderers;
	Container mOwnedRenderers;
	RenderContext mContext;

};

} // namespace Gleam
