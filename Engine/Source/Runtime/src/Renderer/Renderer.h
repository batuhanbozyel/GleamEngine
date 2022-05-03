#pragma once

namespace Gleam {

struct Version;
struct RendererProperties;

class Renderer
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	static void BeginFrame();

	static void EndFrame();
    
    static void SetClearColor(const Color& color)
    {
        mClearColor = color;
    }
    
	/**
	* Renderer specific implementations
	*/

	virtual ~Renderer() = default;

	virtual void Render() = 0;
    
protected:
    
    static inline Color mClearColor{0.1f, 0.1f, 0.1f, 1.0f};

};

} // namespace Gleam
