//
//  PostProcessStack.cpp
//  Runtime
//
//  Created by Batuhan Bozyel on 19.05.2023.
//

#include "gpch.h"
#include "PostProcessStack.h"

#include "Renderer/CommandBuffer.h"
#include "Renderer/RendererContext.h"

#include "WorldRenderer.h"

#define USE_METAL_IRCONVERTER

#ifndef USE_METAL_IRCONVERTER
#include "../Metal/MetalUtils.h"
#include "../Metal/MetalPipelineStateManager.h"

struct
{
    id<MTLLibrary> library;
    id<MTLFunction> vertexShader;
    id<MTLFunction> fragmentShader;
    id<MTLRenderPipelineState> pipeline;
} metalData;
#endif

using namespace Gleam;

void PostProcessStack::OnCreate(RendererContext& context)
{
    mFullscreenTriangleVertexShader = context.CreateShader("fullscreenTriangleVertexShader", ShaderStage::Vertex);
    mTonemappingFragmentShader = context.CreateShader("tonemappingFragmentShader", ShaderStage::Fragment);
    
#ifndef USE_METAL_IRCONVERTER
    static const char* sources = R"(
        #include <metal_stdlib>
        #include <simd/simd.h>
        #include <metal_math>
    
        using namespace metal;
        
        struct VertexOut
        {
            float4 position [[position]];
            float2 texCoord;
        };
        
        vertex VertexOut fullscreenTriangleVertexShader(uint vertexID [[vertex_id]])
        {
            VertexOut out;
            float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
            out.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
            out.texCoord = uv;
            return out;
        }
    
        struct ArgumentBuffer
        {
            texture2d<float> finalColorRT;
            sampler smp;
        };
    
        fragment float4 tonemappingFragmentShader(VertexOut in [[stage_in]],
                                                  constant ArgumentBuffer& ab [[buffer(0)]])
        {
            return ab.finalColorRT.sample(ab.smp, in.texCoord);
        }
    )";
    
    __autoreleasing NSError* errorLib = nil;
    metalData.library = [MetalDevice::GetHandle() newLibraryWithSource:[NSString stringWithCString:sources encoding:NSUTF8StringEncoding] options:nil error:&errorLib];
    metalData.vertexShader = [metalData.library newFunctionWithName:@"fullscreenTriangleVertexShader"];
    metalData.fragmentShader = [metalData.library newFunctionWithName:@"tonemappingFragmentShader"];
    
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.rasterSampleCount = 1;
    pipelineDescriptor.vertexFunction = metalData.vertexShader;
    pipelineDescriptor.fragmentFunction = metalData.fragmentShader;
    pipelineDescriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassTriangle;
    pipelineDescriptor.colorAttachments[0].pixelFormat = TextureFormatToMTLPixelFormat(TextureFormat::R8G8B8A8_SRGB);
    
    __autoreleasing NSError* error = nil;
    metalData.pipeline = [MetalDevice::GetHandle() newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
#endif
}

void PostProcessStack::AddRenderPasses(RenderGraph& graph, RenderGraphBlackboard& blackboard)
{
    struct PostProcessData
    {
        RenderTextureHandle colorTarget;
        RenderTextureHandle sceneTarget;
    };
    
    graph.AddRenderPass<PostProcessData>("PostProcessStack::Tonemapping", [&](RenderGraphBuilder& builder, PostProcessData& passData)
    {
        auto& renderingData = blackboard.Get<RenderingData>();
        const auto& worldRenderingData = blackboard.Get<WorldRenderingData>();
        passData.colorTarget = builder.UseColorBuffer(renderingData.backbuffer);
        passData.sceneTarget = builder.ReadRenderTexture(worldRenderingData.colorTarget);
        
        renderingData.backbuffer = passData.colorTarget;
    },
    [this](const RenderGraphContext& renderGraphContext, const PostProcessData& passData)
    {
        const auto& sceneRT = renderGraphContext.registry->GetRenderTexture(passData.sceneTarget);
        
#ifdef USE_METAL_IRCONVERTER
        PipelineStateDescriptor pipelineDesc;
        renderGraphContext.cmd->BindGraphicsPipeline(pipelineDesc, mFullscreenTriangleVertexShader, mTonemappingFragmentShader);
        renderGraphContext.cmd->BindTexture(*sceneRT, 0, ShaderStage_Fragment);
        renderGraphContext.cmd->Draw(3);
#else
        struct ArgumentBuffer
        {
            uint64_t textureResourceID;
            uint64_t samplerResourceID;
        } ab;
        
        id<MTLTexture> texture = sceneRT->GetView();
        id<MTLSamplerState> samplerState = MetalPipelineStateManager::GetSamplerState(0);
        id<MTLRenderCommandEncoder> renderCommandEncoder = renderGraphContext.cmd->GetActiveRenderPass();
        id<MTLBuffer> argumentBuffer = [MetalDevice::GetHandle() newBufferWithLength:sizeof(ArgumentBuffer) options:MTLResourceStorageModeShared];
        auto argumentBufferPtr = static_cast<uint8_t*>([argumentBuffer contents]);
        
        ab.textureResourceID = [texture gpuResourceID]._impl;
        ab.samplerResourceID = [samplerState gpuResourceID]._impl;
        
        memcpy(argumentBufferPtr, &ab.textureResourceID, sizeof(uint64_t));
        memcpy(argumentBufferPtr + sizeof(uint64_t), &ab.samplerResourceID, sizeof(uint64_t));
        
        [renderCommandEncoder setRenderPipelineState:metalData.pipeline];
        [renderCommandEncoder setFragmentBuffer:argumentBuffer offset:0 atIndex:0];
        
        [renderCommandEncoder useResource:texture usage:MTLResourceUsageRead stages:MTLRenderStageFragment];
        [renderCommandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
#endif
    });
}
