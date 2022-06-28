#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/ShaderLibrary.h"
#include "MetalUtils.h"

using namespace Gleam;

struct
{
    id<MTLLibrary> library{ nil };
} mContext;

void ShaderLibrary::Init()
{
	auto binaryData = IOUtils::ReadBinaryFile("Assets/PrecompiledShaders.metallib");
    NSError* error;
        
    mContext.library = [MetalDevice newLibraryWithData:dispatch_data_create(binaryData.data(), binaryData.size(), nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT) error:&error];
    GLEAM_ASSERT(mContext.library);
}

Shader::Shader(const TString& entryPoint, ShaderStage stage)
    : mEntryPoint(entryPoint), mStage(stage)
{    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    mHandle = [mContext.library newFunctionWithName:functionName];
}

Shader::~Shader()
{
    mHandle = nil;
}

#endif
