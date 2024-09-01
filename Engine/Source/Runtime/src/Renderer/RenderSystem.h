//
//  RenderSystem.h
//  Runtime
//
//  Created by Batuhan Bozyel on 24.06.2023.
//

#pragma once
#include "Core/Subsystem.h"
#include "CommandBuffer.h"
#include "GraphicsDevice.h"
#include "RenderSceneProxy.h"

namespace Gleam {

class World;

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RenderSystem final : public Subsystem
{
    using Container = TArray<IRenderer*>;
    
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void Render(const World* world);
    
    void Configure(const RendererConfig& config);
    
    GraphicsDevice* GetDevice();
    
    const GraphicsDevice* GetDevice() const;
    
    const Texture& GetRenderTarget() const;
    
    void UpdateCamera(const Camera& camera);
    
    void SetBackbuffer(const TextureDescriptor& descriptor);
    
    void SetBackbuffer(const Texture& texture);
    
    void ResetRenderTarget();
    
    template<RendererType T, class...Args>
    T* AddRenderer(Args&&... args)
    {
        GLEAM_ASSERT(!HasRenderer<T>(), "Render pipeline already has the renderer!");
        auto renderer = mRenderers.emplace_back(new T(std::forward<Args>(args)...));
        renderer->OnCreate(mDevice.get());
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
            renderer->OnDestroy(mDevice.get());
            delete renderer;
			mRenderers.erase(it);
        }
    }
    
    template<RendererType T>
    T* GetRenderer()
    {
        GLEAM_ASSERT(HasRenderer<T>(), "Render pipeline does not have the renderer!");
        auto it = std::find_if(mRenderers.begin(), mRenderers.end(), [](const IRenderer* renderer)
        {
            return typeid(*renderer) == typeid(T);
        });
        return static_cast<T*>(*it);
    }
    
    template<RendererType T>
    bool HasRenderer()
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
    
    Texture mRenderTarget;
    
    RenderSceneProxy mSceneProxy;
    
    Scope<GraphicsDevice> mDevice;
    
    TArray<Scope<CommandBuffer>> mCommandBuffers;
    
};

} // namespace Gleam

