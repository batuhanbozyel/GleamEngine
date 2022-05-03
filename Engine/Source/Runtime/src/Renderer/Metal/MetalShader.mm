#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Shader.h"
#include "MetalUtils.h"

using namespace Gleam;

struct
{
    id<MTLLibrary> library{ nil };
} mContext;

Shader::Shader(const TString& entryPoint)
    : mEntryPoint(entryPoint)
{
    if (mContext.library == nil)
    {
        auto binaryData = IOUtils::ReadBinaryFile("Assets/PrecompiledShaders.metallib");
        NSError* error;
        
        mContext.library = [MetalDevice newLibraryWithData:dispatch_data_create(binaryData.data(), binaryData.size(), nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT) error:&error];
        GLEAM_ASSERT(mContext.library);
    }
    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    mHandle = [mContext.library newFunctionWithName:functionName];
}

Shader::~Shader()
{
    mHandle = nil;
}

#endif
