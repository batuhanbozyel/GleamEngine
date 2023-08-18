#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalShaderReflect.h"
#include "MetalUtils.h"

using namespace Gleam;

Shader::Shader(const TString& entryPoint, ShaderStage stage)
    : mEntryPoint(entryPoint), mStage(stage)
{
    TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(GameInstance->GetDefaultAssetPath().append(entryPoint + ".dxil"));
    
    auto compiler = IRCompilerCreate();
    IRCompilerSetEntryPointName(compiler, entryPoint.data());
    
    if (stage == ShaderStage::Vertex)
    {
        IRCompilerSetStageInGenerationMode(compiler, IRStageInCodeGenerationModeUseSeparateStageInFunction);
    }
    
    auto dxil = IRObjectCreateFromDXIL(shaderCode.data(), shaderCode.size(), IRBytecodeOwnershipNone);
    auto metalIR = IRCompilerAllocCompileAndLink(compiler, nullptr, 0, dxil, nullptr);
    
    IRMetalLibBinary* metallibBinary = IRMetalLibBinaryCreate();
    if (stage == ShaderStage::Vertex)
    {
        IRVersionedInputLayoutDescriptor layout;
        auto reflection = IRShaderReflectionCreate();
        IRMetalLibSynthesizeStageInFunction(compiler, reflection, &layout, metallibBinary);
        IRShaderReflectionDestroy(reflection);
    }
    
    IRObjectGetMetalLibBinary(metalIR, IRObjectGetMetalIRShaderStage(metalIR), metallibBinary);
    size_t metallibSize = IRMetalLibGetBytecodeSize(metallibBinary);
    
    TArray<uint8_t> metallib(metallibSize);
    IRMetalLibGetBytecode(metallibBinary, metallib.data());
    
    NSError* __autoreleasing error = nil;
    dispatch_data_t data = dispatch_data_create(metallib.data(), metallibSize, dispatch_get_main_queue(), NULL);
    id<MTLLibrary> library = [MetalDevice::GetHandle() newLibraryWithData:data error:&error];
    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    mHandle = [library newFunctionWithName:functionName];
    mReflection = CreateScope<Reflection>(metalIR);
    
    // Clean up
    IRMetalLibBinaryDestroy(metallibBinary);
    IRObjectDestroy(dxil);
    IRObjectDestroy(metalIR);
    IRCompilerDestroy(compiler);
}

Shader::~Shader()
{
    mHandle = nil;
}

#endif
