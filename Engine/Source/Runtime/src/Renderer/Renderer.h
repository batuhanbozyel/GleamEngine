#pragma once
#include "RendererContext.h"

namespace Gleam {

class Renderer
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	static void BeginFrame();
    
	static void EndFrame();

	static handle_t GetDevice();
    
private:
    
    static inline Scope<RendererContext> mContext = nullptr;

};

} // namespace Gleam
