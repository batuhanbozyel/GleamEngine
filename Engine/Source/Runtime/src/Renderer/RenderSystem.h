//
//  RenderSystem.h
//  Runtime
//
//  Created by Batuhan Bozyel on 24.06.2023.
//

#pragma once
#include "Core/Subsystem.h"
#include "CommandBuffer.h"
#include "RendererContext.h"

namespace Gleam {

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RenderSystem final : public Subsystem
{
    using Container = TArray<IRenderer*>;
    
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void Render();
    
    void Configure(const RendererConfig& config);
    
    const RendererConfig& GetConfiguration() const;
    
    const Texture& GetRenderTarget() const;
    
    void SetRenderTarget(const TextureDescriptor& descriptor);
    
    void SetRenderTarget(const Texture& texture);
    
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
    
    RendererConfig mConfiguration;
    
    RendererContext mRendererContext;
    
    Texture mRenderTarget;
    
};

} // namespace Gleam

