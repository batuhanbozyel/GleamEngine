#pragma once
#ifdef USE_METAL_RENDERER
#include "MetalDevice.h"
#include "Renderer/Shader.h"
#include "Renderer/TextureFormat.h"
#include "Renderer/HeapDescriptor.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"
#include "Renderer/RenderGraph/RenderGraphResource.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

static constexpr TextureFormat MTLPixelFormatToTextureFormat(MTLPixelFormat format)
{
    switch (format)
    {
        case MTLPixelFormatR8Unorm_sRGB: return TextureFormat::R8_SRGB;
        case MTLPixelFormatRG8Unorm_sRGB: return TextureFormat::R8G8_SRGB;
        case MTLPixelFormatRGBA8Unorm_sRGB: return TextureFormat::R8G8B8A8_SRGB;

        case MTLPixelFormatR8Unorm: return TextureFormat::R8_UNorm;
        case MTLPixelFormatRG8Unorm: return TextureFormat::R8G8_UNorm;
        case MTLPixelFormatRGBA8Unorm: return TextureFormat::R8G8B8A8_UNorm;

        case MTLPixelFormatR8Snorm: return TextureFormat::R8_SNorm;
        case MTLPixelFormatRG8Snorm: return TextureFormat::R8G8_SNorm;
        case MTLPixelFormatRGBA8Snorm: return TextureFormat::R8G8B8A8_SNorm;

        case MTLPixelFormatR8Uint: return TextureFormat::R8_UInt;
        case MTLPixelFormatRG8Uint: return TextureFormat::R8G8_UInt;
        case MTLPixelFormatRGBA8Uint: return TextureFormat::R8G8B8A8_UInt;

        case MTLPixelFormatR8Sint: return TextureFormat::R8_SInt;
        case MTLPixelFormatRG8Sint: return TextureFormat::R8G8_SInt;
        case MTLPixelFormatRGBA8Sint: return TextureFormat::R8G8B8A8_SInt;

        case MTLPixelFormatR16Unorm: return TextureFormat::R16_UNorm;
        case MTLPixelFormatRG16Unorm: return TextureFormat::R16G16_UNorm;
        case MTLPixelFormatRGBA16Unorm: return TextureFormat::R16G16B16A16_UNorm;

        case MTLPixelFormatR16Snorm: return TextureFormat::R16_SNorm;
        case MTLPixelFormatRG16Snorm: return TextureFormat::R16G16_SNorm;
        case MTLPixelFormatRGBA16Snorm: return TextureFormat::R16G16B16A16_SNorm;

        case MTLPixelFormatR16Uint: return TextureFormat::R16_UInt;
        case MTLPixelFormatRG16Uint: return TextureFormat::R16G16_UInt;
        case MTLPixelFormatRGBA16Uint: return TextureFormat::R16G16B16A16_UInt;

        case MTLPixelFormatR16Sint: return TextureFormat::R16_SInt;
        case MTLPixelFormatRG16Sint: return TextureFormat::R16G16_SInt;
        case MTLPixelFormatRGBA16Sint: return TextureFormat::R16G16B16A16_SInt;

		case MTLPixelFormatR16Float: return TextureFormat::R16_SFloat;
		case MTLPixelFormatRG16Float: return TextureFormat::R16G16_SFloat;
		case MTLPixelFormatRGBA16Float: return TextureFormat::R16G16B16A16_SFloat;

        case MTLPixelFormatR32Uint: return TextureFormat::R32_UInt;
        case MTLPixelFormatRG32Uint: return TextureFormat::R32G32_UInt;
        case MTLPixelFormatRGBA32Uint: return TextureFormat::R32G32B32A32_UInt;

        case MTLPixelFormatR32Sint: return TextureFormat::R32_SInt;
        case MTLPixelFormatRG32Sint: return TextureFormat::R32G32_SInt;
        case MTLPixelFormatRGBA32Sint: return TextureFormat::R32G32B32A32_SInt;

        case MTLPixelFormatR32Float: return TextureFormat::R32_SFloat;
        case MTLPixelFormatRG32Float: return TextureFormat::R32G32_SFloat;
        case MTLPixelFormatRGBA32Float: return TextureFormat::R32G32B32A32_SFloat;

        case MTLPixelFormatBGRA8Unorm_sRGB: return TextureFormat::B8G8R8A8_SRGB;
        case MTLPixelFormatBGRA8Unorm: return TextureFormat::B8G8R8A8_UNorm;
            
        // Depth - Stencil formats
        case MTLPixelFormatStencil8: return TextureFormat::S8_UInt;
        case MTLPixelFormatDepth16Unorm: return TextureFormat::D16_UNorm;
        case MTLPixelFormatDepth32Float: return TextureFormat::D32_SFloat;
        case MTLPixelFormatDepth32Float_Stencil8: return TextureFormat::D32_SFloat_S8_UInt;

        default: return TextureFormat::None;
    }
}

static constexpr MTLPixelFormat TextureFormatToMTLPixelFormat(TextureFormat format)
{
    switch (format)
    {
        case TextureFormat::R8_SRGB: return MTLPixelFormatR8Unorm_sRGB;
        case TextureFormat::R8G8_SRGB: return MTLPixelFormatRG8Unorm_sRGB;
        case TextureFormat::R8G8B8A8_SRGB: return MTLPixelFormatRGBA8Unorm_sRGB;

        case TextureFormat::R8_UNorm: return MTLPixelFormatR8Unorm;
        case TextureFormat::R8G8_UNorm: return MTLPixelFormatRG8Unorm;
        case TextureFormat::R8G8B8A8_UNorm: return MTLPixelFormatRGBA8Unorm;

        case TextureFormat::R8_SNorm: return MTLPixelFormatR8Snorm;
        case TextureFormat::R8G8_SNorm: return MTLPixelFormatRG8Snorm;
        case TextureFormat::R8G8B8A8_SNorm: return MTLPixelFormatRGBA8Snorm;

        case TextureFormat::R8_UInt: return MTLPixelFormatR8Uint;
        case TextureFormat::R8G8_UInt: return MTLPixelFormatRG8Uint;
        case TextureFormat::R8G8B8A8_UInt: return MTLPixelFormatRGBA8Uint;

        case TextureFormat::R8_SInt: return MTLPixelFormatR8Sint;
        case TextureFormat::R8G8_SInt: return MTLPixelFormatRG8Sint;
        case TextureFormat::R8G8B8A8_SInt: return MTLPixelFormatRGBA8Sint;

        case TextureFormat::R16_UNorm: return MTLPixelFormatR16Unorm;
        case TextureFormat::R16G16_UNorm: return MTLPixelFormatRG16Unorm;
        case TextureFormat::R16G16B16A16_UNorm: return MTLPixelFormatRGBA16Unorm;

        case TextureFormat::R16_SNorm: return MTLPixelFormatR16Snorm;
        case TextureFormat::R16G16_SNorm: return MTLPixelFormatRG16Snorm;
        case TextureFormat::R16G16B16A16_SNorm: return MTLPixelFormatRGBA16Snorm;

        case TextureFormat::R16_UInt: return MTLPixelFormatR16Uint;
        case TextureFormat::R16G16_UInt: return MTLPixelFormatRG16Uint;
        case TextureFormat::R16G16B16A16_UInt: return MTLPixelFormatRGBA16Uint;

        case TextureFormat::R16_SInt: return MTLPixelFormatR16Sint;
        case TextureFormat::R16G16_SInt: return MTLPixelFormatRG16Sint;
        case TextureFormat::R16G16B16A16_SInt: return MTLPixelFormatRGBA16Sint;

		case TextureFormat::R16_SFloat: return MTLPixelFormatR16Float;
		case TextureFormat::R16G16_SFloat: return MTLPixelFormatRG16Float;
		case TextureFormat::R16G16B16A16_SFloat: return MTLPixelFormatRGBA16Float;

        case TextureFormat::R32_UInt: return MTLPixelFormatR32Uint;
        case TextureFormat::R32G32_UInt: return MTLPixelFormatRG32Uint;
        case TextureFormat::R32G32B32A32_UInt: return MTLPixelFormatRGBA32Uint;

        case TextureFormat::R32_SInt: return MTLPixelFormatR32Sint;
        case TextureFormat::R32G32_SInt: return MTLPixelFormatRG32Sint;
        case TextureFormat::R32G32B32A32_SInt: return MTLPixelFormatRGBA32Sint;

        case TextureFormat::R32_SFloat: return MTLPixelFormatR32Float;
        case TextureFormat::R32G32_SFloat: return MTLPixelFormatRG32Float;
        case TextureFormat::R32G32B32A32_SFloat: return MTLPixelFormatRGBA32Float;
            
        case TextureFormat::B8G8R8A8_SRGB: return MTLPixelFormatBGRA8Unorm_sRGB;
        case TextureFormat::B8G8R8A8_UNorm: return MTLPixelFormatBGRA8Unorm;

        // Depth - Stencil formats
        case TextureFormat::S8_UInt: return MTLPixelFormatStencil8;
        case TextureFormat::D16_UNorm: return MTLPixelFormatDepth16Unorm;
        case TextureFormat::D32_SFloat: return MTLPixelFormatDepth32Float;
        case TextureFormat::D32_SFloat_S8_UInt: return MTLPixelFormatDepth32Float_Stencil8;

        default: return MTLPixelFormatInvalid;
    }
}

static constexpr MTLTextureUsage TextureUsageToMTLTextureUsage(TextureUsageFlagBits flags)
{
    MTLTextureUsage usage = 0;
    if (flags & TextureUsage_Sampled)
    {
        usage |= MTLTextureUsageShaderRead;
    }
    
    if (flags & TextureUsage_Storage)
    {
        usage |= MTLTextureUsageShaderWrite;
    }
    
    if (flags & TextureUsage_Attachment)
    {
        usage |= MTLTextureUsageRenderTarget;
    }
    return usage;
}

static constexpr MTLRenderStages ShaderStagesToMTLRenderStages(ShaderStageFlagBits flags)
{
    MTLRenderStages stages = 0;
    if (flags & ShaderStage_Vertex)
    {
        stages |= MTLRenderStageVertex;
    }
    
    if (flags & ShaderStage_Fragment)
    {
        stages |= MTLRenderStageFragment;
    }
    
    if (flags & ShaderStage_Compute)
    {
        stages |= MTLRenderStageTile;
    }
    return stages;
}

static constexpr MTLCullMode CullModeToMTLCullMode(CullMode cullMode)
{
    switch (cullMode)
    {
        case CullMode::Off: return MTLCullModeNone;
        case CullMode::Back: return MTLCullModeBack;
        case CullMode::Front: return MTLCullModeFront;
        default: GLEAM_ASSERT(false, "Metal: Unknown culling mode specified!"); return MTLCullMode(~0);
    }
}

static constexpr MTLPrimitiveType PrimitiveTopologyToMTLPrimitiveType(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::Points: return MTLPrimitiveTypePoint;
        case PrimitiveTopology::Lines: return MTLPrimitiveTypeLine;
        case PrimitiveTopology::LineStrip: return MTLPrimitiveTypeLineStrip;
        case PrimitiveTopology::Triangles: return MTLPrimitiveTypeTriangle;
        case PrimitiveTopology::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
        default: GLEAM_ASSERT(false, "Metal: Unknown primitive topology specified!"); return MTLPrimitiveType(~0);
    }
}

static constexpr MTLPrimitiveTopologyClass PrimitiveTopologyToMTLPrimitiveTopologyClass(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::Points: return MTLPrimitiveTopologyClassPoint;
        case PrimitiveTopology::Lines: return MTLPrimitiveTopologyClassLine;
        case PrimitiveTopology::LineStrip: return MTLPrimitiveTopologyClassLine;
        case PrimitiveTopology::Triangles: return MTLPrimitiveTopologyClassTriangle;
        case PrimitiveTopology::TriangleStrip: return MTLPrimitiveTopologyClassTriangle;
        default: GLEAM_ASSERT(false, "Metal: Unknown primitive topology specified!"); return MTLPrimitiveTopologyClassUnspecified;
    }
}

static constexpr MTLLoadAction AttachmentLoadActionToMTLLoadAction(AttachmentLoadAction loadAction)
{
    switch (loadAction)
    {
        case AttachmentLoadAction::Clear: return MTLLoadActionClear;
        case AttachmentLoadAction::Load: return MTLLoadActionLoad;
        case AttachmentLoadAction::DontCare: return MTLLoadActionDontCare;
        default: GLEAM_ASSERT(false, "Metal: Unknown attachment load action specified!"); return MTLLoadAction(~0);
    }
}

static constexpr MTLStoreAction AttachmentStoreActionToMTLStoreAction(AttachmentStoreAction storeAction)
{
    switch (storeAction)
    {
        case AttachmentStoreAction::Store: return MTLStoreActionStore;
        case AttachmentStoreAction::Resolve: return MTLStoreActionMultisampleResolve;
        case AttachmentStoreAction::StoreAndResolve: return MTLStoreActionStoreAndMultisampleResolve;
        case AttachmentStoreAction::DontCare: return MTLStoreActionDontCare;
        default: GLEAM_ASSERT(false, "Metal: Unknown attachment store action specified!"); return MTLStoreActionUnknown;
    }
}

static constexpr MTLCompareFunction CompareFunctionToMTLCompareFunction(CompareFunction compareFunction)
{
    switch (compareFunction)
    {
        case CompareFunction::Never: return MTLCompareFunctionNever;
        case CompareFunction::Less: return MTLCompareFunctionLess;
        case CompareFunction::Equal: return MTLCompareFunctionEqual;
        case CompareFunction::LessEqual: return MTLCompareFunctionLessEqual;
        case CompareFunction::Greater: return MTLCompareFunctionGreater;
        case CompareFunction::NotEqual: return MTLCompareFunctionNotEqual;
        case CompareFunction::GreaterEqual: return MTLCompareFunctionGreaterEqual;
        case CompareFunction::Always: return MTLCompareFunctionAlways;
        default: GLEAM_ASSERT(false, "Metal: Unknown compare function specified!"); return MTLCompareFunction(~0);
    }
}

static constexpr MTLStencilOperation StencilOpToMTLStencilOperation(StencilOp stencilOp)
{
    switch (stencilOp)
    {
        case StencilOp::Keep: return MTLStencilOperationKeep;
        case StencilOp::Zero: return MTLStencilOperationZero;
        case StencilOp::Replace: return MTLStencilOperationReplace;
        case StencilOp::IncrementClamp: return MTLStencilOperationIncrementClamp;
        case StencilOp::DecrementClamp: return MTLStencilOperationDecrementClamp;
        case StencilOp::IncrementWrap: return MTLStencilOperationIncrementWrap;
        case StencilOp::DecrementWrap: return MTLStencilOperationDecrementWrap;
        case StencilOp::Invert: return MTLStencilOperationInvert;
        default: GLEAM_ASSERT(false, "Metal: Unknown stencil operation specified!"); return MTLStencilOperation(~0);
    }
}

static constexpr MTLBlendFactor BlendModeToMTLBlendFactor(BlendMode blendMode)
{
    switch (blendMode)
    {
        case BlendMode::Zero: return MTLBlendFactorZero;
        case BlendMode::One: return MTLBlendFactorOne;
        case BlendMode::DstColor: return MTLBlendFactorDestinationColor;
        case BlendMode::SrcColor: return MTLBlendFactorSourceColor;
        case BlendMode::OneMinusDstColor: return MTLBlendFactorOneMinusDestinationColor;
        case BlendMode::SrcAlpha: return MTLBlendFactorSourceAlpha;
        case BlendMode::OneMinusSrcColor: return MTLBlendFactorOneMinusSourceColor;
        case BlendMode::DstAlpha: return MTLBlendFactorDestinationAlpha;
        case BlendMode::OneMinusDstAlpha: return MTLBlendFactorOneMinusDestinationAlpha;
        case BlendMode::SrcAlphaClamp: return MTLBlendFactorSourceAlphaSaturated;
        case BlendMode::OneMinusSrcAlpha: return MTLBlendFactorOneMinusSourceAlpha;
        default: GLEAM_ASSERT(false, "Metal: Unknown blend mode specified!"); return MTLBlendFactor(~0);
    }
}

static constexpr MTLBlendOperation BlendOpToMTLBlendOperation(BlendOp operation)
{
    switch (operation)
    {
        case BlendOp::Add: return MTLBlendOperationAdd;
        case BlendOp::Subtract: return MTLBlendOperationSubtract;
        case BlendOp::ReverseSubtract: return MTLBlendOperationReverseSubtract;
        case BlendOp::Min: return MTLBlendOperationMin;
        case BlendOp::Max: return MTLBlendOperationMax;
        default: GLEAM_ASSERT(false, "Metal: Unknown blend operation specified!"); return MTLBlendOperation(~0);
    }
}

static constexpr MTLColorWriteMask ColorWriteMaskToMTLColorWriteMask(ColorWriteMask mask)
{
    switch (mask)
    {
        case ColorWriteMask::Alpha: return MTLColorWriteMaskAlpha;
        case ColorWriteMask::Blue: return MTLColorWriteMaskBlue;
        case ColorWriteMask::Green: return MTLColorWriteMaskGreen;
        case ColorWriteMask::Red: return MTLColorWriteMaskRed;
        case ColorWriteMask::All: return MTLColorWriteMaskAll;
        default: GLEAM_ASSERT(false, "Metal: Unknown color write mask specified!"); return MTLColorWriteMaskNone;
    }
}

static constexpr MTLResourceOptions MemoryTypeToMTLResourceOption(MemoryType type)
{
    switch (type)
    {
        case MemoryType::GPU: return MTLResourceStorageModePrivate;
#if defined(PLATFORM_MACOS) && defined(__arm64__)
        case MemoryType::Shared: return MTLResourceStorageModeShared;
#elif defined(PLATFORM_MACOS)
        case MemoryType::Shared: return MTLResourceStorageModeManaged;
#elif defined(PLATFORM_IOS)
        case MemoryType::Shared: return MTLResourceStorageModeShared;
#endif
        case MemoryType::CPU: return MTLResourceStorageModeShared;
        default: GLEAM_ASSERT(false, "Metal: Unknown memory type specified!"); return MTLResourceOptions(~0);
    }
}

static constexpr MTLResourceUsage ResourceAccessToMTLResourceUsage(ResourceAccess access)
{
    switch (access)
    {
        case ResourceAccess::Read: return MTLResourceUsageRead;
        case ResourceAccess::Write: return MTLResourceUsageWrite;
        default: GLEAM_ASSERT(false, "Metal: Unknown access mode specified!"); return MTLResourceUsage(~0);
    }
}

} // namespace Gleam
#endif
