#pragma once
#include "Camera.h"
#include "Buffer.h"
#include "ShaderTypes.h"

namespace Gleam {

struct Version;
struct RendererProperties;

class Renderer
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	static Vector2 GetDrawableSize();
    
	/**
	* Renderer specific implementations
	*/
    
    Renderer();
    
	virtual ~Renderer() = default;

	virtual void Render() = 0;
    
    void UpdateView(Camera& camera);
    
    const UniformBuffer<CameraUniforms, MemoryType::Dynamic>& GetCameraBuffer() const;
    
	static inline Color clearColor{ 0.1f, 0.1f, 0.1f, 1.0f };
    
private:
    
    TArray<Scope<UniformBuffer<CameraUniforms, MemoryType::Dynamic>>> mCameraBuffers;

};

} // namespace Gleam
