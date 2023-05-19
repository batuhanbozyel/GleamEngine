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
    
    using Container = TArray<IRenderer*>;
    
public:
    
    ~RenderPipeline()
    {
        for (auto renderer : mRenderers)
        {
			renderer->OnDestroy();
            delete renderer;
        }
    }
    
    template<RendererType T>
    T* AddRenderer()
    {
        GLEAM_ASSERT(!HasRenderer<T>(), "Render pipeline already has the renderer!");
        auto renderer = mRenderers.emplace_back(new T());
        renderer->OnCreate(mRendererContext);
        return static_cast<T*>(renderer);
    }
    
    template<RendererType T>
    void RemoveRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Render pipeline does not have the renderer!");
        auto it = mRenderers.erase(std::remove_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* ptr)
                                        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
        }));

        if (it != mRenderers.end()) 
		{
            auto renderer = *it;
            renderer->OnDestroy();
			delete renderer;
		}
    }
    
    template<RendererType T>
    T* GetRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Render pipeline does not have the renderer!");
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* ptr)
        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
        });
        return static_cast<T*>(*it);
    }
    
    template<RendererType T>
    bool HasRenderer()
    {
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* ptr)
        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
        });
        return it != mRenderers.end();
    }
    
    template<RendererType T>
    uint32_t GetIndexOf() const
    {
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* ptr)
        {
            const auto& renderer = *ptr;
            return typeid(renderer) == typeid(T);
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
    
    // initialized and set when a Game instance is created
    static inline RendererContext* mRendererContext = nullptr;
    
};

} // Gleam
