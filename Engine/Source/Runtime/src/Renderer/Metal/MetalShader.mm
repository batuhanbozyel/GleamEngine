#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/ShaderLibrary.h"
#include "MetalUtils.h"

using namespace Gleam;

static HashMap<TString, id<MTLFunction>> mShaderCache;
static id<MTLLibrary> mMetalLibrary;
static id<MTLLibrary> mPrecompiledMetalLibrary;

static bool ShouldCompileShaderSources()
{
    if (std::filesystem::exists("Assets/PrecompiledShaders.metallib"))
    {
        const auto& binaryChangeTime = std::filesystem::last_write_time("Assets/PrecompiledShaders.metallib");
        for (const auto& file : std::filesystem::directory_iterator("Engine/Source/Runtime/src/Renderer/Metal/Shaders"))
        {
            if (file.path().extension() == ".metal")
            {
                const auto& sourceChangeTime = std::filesystem::last_write_time(file.path());
                if (sourceChangeTime > binaryChangeTime)
                {
                    return true;
                }
            }
        }
        return false;
    }
    return true;
}

/************************************************************************/
/*    Init                                    */
/************************************************************************/
void ShaderLibrary::Init()
{
    // TODO: Make metallib generation as part of development so that distributed code won't even check this
    if (ShouldCompileShaderSources())
    {
        TStringStream genMetalLibCommand;
        genMetalLibCommand << "xcrun -sdk macosx metal -frecord-sources=flat ";
        for (const auto& file : std::filesystem::directory_iterator("Engine/Source/Runtime/src/Renderer/Metal/Shaders"))
        {
            if (file.path().extension() == ".metal")
            {
                const auto& sourceChangeTime = std::filesystem::last_write_time(file.path());
                genMetalLibCommand << file.path().string() << ' ';
            }
        }
        genMetalLibCommand << "-o Assets/PrecompiledShaders.metallib";
        
        IOUtils::ExecuteCommand(genMetalLibCommand.str());
    }
    
    NSError* error = nil;
    auto binaryData = IOUtils::ReadBinaryFile("Assets/PrecompiledShaders.metallib");
    mPrecompiledMetalLibrary = [MetalDevice newLibraryWithData:dispatch_data_create(binaryData.data(), binaryData.size(), nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT) error:&error];
    GLEAM_ASSERT(mPrecompiledMetalLibrary);
}
/************************************************************************/
/*    Destroy                                 */
/************************************************************************/
void ShaderLibrary::Destroy()
{
    ClearCache();
    mMetalLibrary = nil;
    mPrecompiledMetalLibrary = nil;
}
/************************************************************************/
/*    CreateOrGetCachedShaderFromLibrary      */
/************************************************************************/
static Shader CreateOrGetCachedShaderFromLibrary(id<MTLLibrary> library, const TString& entryPoint)
{
    NSString* entryPointString = [NSString stringWithCString:entryPoint.c_str() encoding:[NSString defaultCStringEncoding]];
    
    Shader shader;
    shader.entryPoint = entryPoint;

    auto shaderCacheIt = mShaderCache.find(entryPoint);
    if (shaderCacheIt != mShaderCache.end())
    {
        shader.handle = (__bridge handle_t)shaderCacheIt->second;
        return shader;
    }
    
    id<MTLFunction> handle = [library newFunctionWithName:entryPointString];
    GLEAM_ASSERT(handle);
    mShaderCache.insert(mShaderCache.end(), { entryPoint, handle });

    shader.handle = (__bridge handle_t)handle;
    return shader;
}
/************************************************************************/
/*    CreatePrecompiledShaderProgram          */
/************************************************************************/
ShaderProgram ShaderLibrary::CreatePrecompiledShaderProgram(const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
    ShaderProgram program;
    program.vertexShader = CreateOrGetCachedShaderFromLibrary(mPrecompiledMetalLibrary, vertexEntryPoint);
    program.fragmentShader = CreateOrGetCachedShaderFromLibrary(mPrecompiledMetalLibrary, fragmentEntryPoint);
    return program;
}
/************************************************************************/
/*    CreatePrecompiledComputeShader          */
/************************************************************************/
Shader ShaderLibrary::CreatePrecompiledComputeShader(const TString& entryPoint)
{
    return CreateOrGetCachedShaderFromLibrary(mPrecompiledMetalLibrary, entryPoint);
}
/************************************************************************/
/*    CreateShaderProgram                     */
/************************************************************************/
ShaderProgram ShaderLibrary::CreateShaderProgram(const TString& filename, const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
    if (mMetalLibrary == nil)
    {
        mMetalLibrary = [MetalDevice newDefaultLibrary];
        GLEAM_ASSERT(mMetalLibrary);
    }
    
    ShaderProgram program;
    program.vertexShader = CreateOrGetCachedShaderFromLibrary(mMetalLibrary, vertexEntryPoint);
    program.fragmentShader = CreateOrGetCachedShaderFromLibrary(mMetalLibrary, fragmentEntryPoint);
    return program;
}
/************************************************************************/
/*    CreateComputeShader                     */
/************************************************************************/
Shader ShaderLibrary::CreateComputeShader(const TString& filename, const TString& entryPoint)
{
    if (mMetalLibrary == nil)
    {
        mMetalLibrary = [MetalDevice newDefaultLibrary];
        GLEAM_ASSERT(mMetalLibrary);
    }
    
    return CreateOrGetCachedShaderFromLibrary(mMetalLibrary, entryPoint);
}
/************************************************************************/
/*    ClearCache                              */
/************************************************************************/
void ShaderLibrary::ClearCache()
{
    for (auto&[_, shader] : mShaderCache)
    {
        shader = nil;
    }
    mShaderCache.clear();
}

#endif
