#pragma once
#include "Renderer/Shader.h"
#include "Core/Application.h"

#include <metal_irconverter/metal_irconverter.h>

namespace Gleam {

struct Shader::Reflection
{
    IRShaderReflection* reflection;
    TArray<IRResourceLocation> SRVs;
    TArray<IRResourceLocation> CBVs;
    TArray<IRResourceLocation> UAVs;
    TArray<IRResourceLocation> samplers;
    size_t argumentBufferSize = 0;
    
	Reflection(IRObject* metalIR)
	{
        reflection = IRShaderReflectionCreate();
        IRObjectGetReflection(metalIR, IRObjectGetMetalIRShaderStage(metalIR), reflection);
        
        TArray<IRResourceLocation> resources(IRShaderReflectionGetResourceCount(reflection));
        IRShaderReflectionGetResourceLocations(reflection, resources.data());
        
        for (const auto& resource : resources)
        {
            argumentBufferSize += resource.sizeBytes;
            switch (resource.resourceType)
            {
                case IRResourceTypeSampler:
                {
                    samplers.push_back(resource);
                    break;
                }
                case IRResourceTypeSRV:
                {
                    SRVs.push_back(resource);
                    break;
                }
                case IRResourceTypeCBV:
                {
                    CBVs.push_back(resource);
                    break;
                }
                case IRResourceTypeUAV:
                {
                    UAVs.push_back(resource);
                    break;
                }
                default:
                {
                    GLEAM_ASSERT(false, "Metal: HLSL resource type is not supported!");
                    break;
                }
            }
        }
	}

	~Reflection()
	{
        IRShaderReflectionDestroy(reflection);
	}
};

} // namespace Gleam
