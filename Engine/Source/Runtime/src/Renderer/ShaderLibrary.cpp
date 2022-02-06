#include "gpch.h"
#include "ShaderLibrary.h"

using namespace Gleam;

static TArray<Ref<GraphicsShader>, BuiltinGrahpicsShaderCOUNT> sBuiltinGraphicsShaders;
static TArray<Ref<ComputeShader>, BuiltinComputeShaderCOUNT> sBuiltinComputeShaders;

/************************************************************************/
/*    Init                                                              */
/************************************************************************/
void ShaderLibrary::Init()
{
	
}
/************************************************************************/
/*    Destroy                                                           */
/************************************************************************/
void ShaderLibrary::Destroy()
{
	ClearCache();
}
/************************************************************************/
/*    ClearCache                                                        */
/************************************************************************/
void ShaderLibrary::ClearCache()
{
	mGraphicsShaderCache.clear();
	mComputeShaderCache.clear();
}
/************************************************************************/
/*    GetBuiltinGraphicsShader                                          */
/************************************************************************/
const Ref<GraphicsShader>& ShaderLibrary::GetBuiltinGraphicsShader(BuiltinGraphicsShader id)
{
	if (sBuiltinGraphicsShaders[id])
	{
		return sBuiltinGraphicsShaders[id];
	}

	switch (id)
	{
		case FullscreenTriangleShader:
		{
			sBuiltinGraphicsShaders[id] = CreateRef<GraphicsShader>("fullscreenTriangleVertexShader", "fullscreenTriangleFragmentShader");
			return sBuiltinGraphicsShaders[id];
		}
		default:
		{
			GLEAM_ASSERT(false, "Invalid built-in Graphics Shader!");
			return nullptr;
		}
	}
}
/************************************************************************/
/*    GetBuiltinComputeShader                                           */
/************************************************************************/
const Ref<ComputeShader>& ShaderLibrary::GetBuiltinComputeShader(BuiltinComputeShader id)
{
	if (sBuiltinComputeShaders[id])
	{
		return sBuiltinComputeShaders[id];
	}

	switch (id)
	{
		default:
		{
			GLEAM_ASSERT(false, "Invalid built-in Compute Shader!");
			return nullptr;
		}
	}
}