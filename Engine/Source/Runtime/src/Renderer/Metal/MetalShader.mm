#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Shader.h"
#include "MetalShaderReflect.h"
#include "MetalUtils.h"

using namespace Gleam;

Shader::Shader(const TString& entryPoint, ShaderStage stage)
    : mEntryPoint(entryPoint), mStage(stage)
{    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    mHandle = [MetalDevice::GetShaderLibrary() newFunctionWithName:functionName];
}

Shader::~Shader()
{
    mHandle = nil;
}

#endif
