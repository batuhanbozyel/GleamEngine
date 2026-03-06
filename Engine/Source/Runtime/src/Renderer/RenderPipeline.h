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
		for (auto renderer : mRenderers)
		{
			renderer->OnDestroy(mContext);
			delete renderer;
		}
		mRenderers.clear();
	}

	template<RendererType T, class...Args>
    T* AddRenderer(Args&&... args)
    {
        GLEAM_ASSERT(!HasRenderer<T>(), "Render pipeline already has the renderer!");
        auto renderer = mRenderers.emplace_back(new T(std::forward<Args>(args)...));
        renderer->OnCreate(mContext);
        return static_cast<T*>(renderer);
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
            renderer->OnDestroy(mContext);
            delete renderer;
			mRenderers.erase(it);
        }
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

	Container mRenderers;
	RenderContext mContext;

};

} // namespace Gleam
