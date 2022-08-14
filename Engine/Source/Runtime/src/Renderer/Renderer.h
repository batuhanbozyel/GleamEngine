#pragma once

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

	virtual ~Renderer() = default;

	virtual void Render() = 0;

	static inline Color clearColor{ 0.1f, 0.1f, 0.1f, 1.0f };

};

} // namespace Gleam
