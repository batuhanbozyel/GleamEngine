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

#define GLEAM_VIEW_CLASS(view) view(RendererContext* rendererContext) : View(rendererContext) {}\
    view(const view&) = default;\
    view& operator=(const view&) = default;

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
    T* GetRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "View does not have the renderer!");
        return mRenderPipeline.get<T>();
    }
    
    template<RendererType T>
    bool HasRenderer() const
    {
		return mRenderPipeline.contains<T>();
    }
    
    const PolyArray<IRenderer> GetRenderPipeline() const
    {
        return mRenderPipeline;
    }

protected:

	virtual void OnCreate() {}
    
	virtual void OnUpdate() {}
    
    virtual void OnFixedUpdate() {}
    
	virtual void OnDestroy() {}

	virtual void OnRender() {}

private:

	PolyArray<IRenderer> mRenderPipeline;
    
    RendererContext* mRendererContext = nullptr;

};

} // namespace Gleam
