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
    
    View(const View&) = default;
    View& operator=(const View&) = default;

	template<RendererType T>
	void AddRenderer()
	{
		GLEAM_ASSERT(!HasRenderer<T>(), "View already has the renderer!");
        T& renderer = mRenderPipeline.emplace<T>();
        renderer.OnCreate(mRendererContext);
	}

	template<RendererType T>
    void RemoveRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "View does not have the renderer!");
        mRenderPipeline.erase<T>();
    }

	template<RendererType T>
    T& GetRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "View does not have the renderer!");
        return mRenderPipeline.get_unsafe<T>();
    }
    
    template<RendererType T>
    bool HasRenderer() const
    {
		return mRenderPipeline.contains<T>();
    }
    
    TArray<IRenderer*> GetRenderPipeline()
    {
        TArray<IRenderer*> renderPipeline;
        
        for (auto& feature : mRenderPipeline)
        {
            IRenderer* renderer = &(std::any_cast<IRenderer&>(feature));
            renderPipeline.push_back(renderer);
        }
        
        return renderPipeline;
    }

protected:

	virtual void OnCreate() {}
    
	virtual void OnUpdate() {}
    
    virtual void OnFixedUpdate() {}
    
	virtual void OnDestroy() {}

	virtual void OnRender() {}

private:

	AnyArray mRenderPipeline;
    
    RendererContext* mRendererContext = nullptr;

};

} // namespace Gleam
