//
//  View.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once
#include "Renderer.h"
#include "BlendState.h"

namespace Gleam {

class Application;

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

#define GLEAM_VIEW_CLASS(view) view(RendererContext* rendererContext) : View(rendererContext) {}

class View
{
    friend class Application;

public:
    
    View(RendererContext* rendererContext)
        : mRendererContext(rendererContext)
    {
        
    }
    
    virtual ~View() = default;
    
    View(const View&) = default;
    View& operator=(const View&) = default;

	template<RendererType T>
	T* AddRenderer()
	{
		GLEAM_ASSERT(!HasRenderer<T>(), "View already has the renderer!");
        T* renderer = mRenderPipeline.emplace<T>();
        renderer->OnCreate(mRendererContext);
        return renderer;
	}

	template<RendererType T>
    void RemoveRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "View does not have the renderer!");
        mRenderPipeline.erase<T>();
    }

	template<RendererType T>
    T* GetRenderer() const
    {
        GLEAM_ASSERT(HasRenderer<T>(), "View does not have the renderer!");
        return mRenderPipeline.get<T>();
    }
    
    template<RendererType T>
    bool HasRenderer() const
    {
		return mRenderPipeline.contains<T>();
    }
    
    void Resize(const Size& size)
    {
        mRect.size = size;
        // TODO: maybe publish an event?
    }
    
    const Size& GetSize() const
    {
        return mRect.size;
    }

protected:

	virtual void OnCreate() {}
    
	virtual void OnUpdate() {}
    
    virtual void OnFixedUpdate() {}
    
	virtual void OnDestroy() {}

	virtual void OnRender() {}

private:

    Rect mRect;
    
	PolyArray<IRenderer> mRenderPipeline;
    
    RendererContext* mRendererContext = nullptr;

};

} // namespace Gleam
