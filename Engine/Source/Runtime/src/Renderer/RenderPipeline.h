//
//  RenderPipeline.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "Renderer.h"

namespace Gleam {

class RendererContext;

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RenderPipeline
{
    friend class RendererContext;
    
public:
    
    static RenderPipeline* Get()
    {
        return mPipeline.get();
    }
    
    static void Set(const RefCounted<RenderPipeline>& pipeline)
    {
        mPipeline = pipeline;
    }
    
    template<RendererType T>
    T* AddRenderer()
    {
        GLEAM_ASSERT(!HasRenderer<T>(), "Game already has the renderer!");
        T* renderer = mRenderers.emplace<T>();
        renderer->OnCreate(mRendererContext);
        return renderer;
    }
    
    template<RendererType T>
    void RemoveRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Game does not have the renderer!");
        mRenderers.erase<T>();
    }
    
    template<RendererType T>
    T* GetRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Game does not have the renderer!");
        return mRenderers.get<T>();
    }
    
    template<RendererType T>
    bool HasRenderer()
    {
        return mRenderers.contains<T>();
    }
    
    PolyArray<IRenderer>& GetRenderers()
    {
        return mRenderers;
    }
    
private:
    
    PolyArray<IRenderer> mRenderers;
    
    // initialized and set when a Game instance is created
    static inline RendererContext* mRendererContext = nullptr;
    
    static inline RefCounted<RenderPipeline> mPipeline = CreateRef<RenderPipeline>();
    
};

} // Gleam
