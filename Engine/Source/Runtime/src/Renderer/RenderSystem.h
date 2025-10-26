//
//  RenderSystem.h
//  Runtime
//
//  Created by Batuhan Bozyel on 24.06.2023.
//

#pragma once
#include "Core/Subsystem.h"
#include "Renderer.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "UploadManager.h"
#include "GraphicsDevice.h"
#include "RenderResourcePool.h"
#include "ResourceReleaseQueue.h"

namespace Gleam {

class World;
class UploadManager;

template <typename T>
concept RendererType = std::is_base_of<IRenderer, T>::value;

class RenderSystem final : public EngineSubsystem
{
    using Container = TArray<IRenderer*>;
    
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown() override;
    
    void Render(const World* world);
    
    void Configure(const RendererConfig& config);

	UploadManager* GetUploadManager();
    
    GraphicsDevice* GetDevice();
    
    const GraphicsDevice* GetDevice() const;

	RenderSurface* GetSurface();

	const RenderSurface* GetSurface() const;

	void RecompileShader(const TString& entryPoint);
    
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
    
private:

	void InitializeBackend();

	bool mRendererResized = false;

	Size mSwapchainSize = {};

	Engine* mEngine;
    
    Container mRenderers;

	RenderContext mContext;

    Scope<Swapchain> mSwapchain;

	Scope<GraphicsDevice> mDevice;

	Scope<UploadManager> mUploadManager;

	Scope<RenderResourcePool> mResourcePool;

	Scope<ResourceReleaseQueue> mReleaseQueue;
    
    TArray<Scope<CommandBuffer>> mCommandBuffers;
    
};

} // namespace Gleam

