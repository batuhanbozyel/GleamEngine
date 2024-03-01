#pragma once
#include "Renderer/Shader.h"
#include "Core/Application.h"

#include <metal_irconverter/metal_irconverter.h>

namespace Gleam {

struct Shader::Reflection
{
	static inline IRResourceLocation invalidResource = { .resourceType = IRResourceTypeInvalid };

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
        
        auto comparator = [](const IRResourceLocation& left, const IRResourceLocation& right)
        {
            return left.slot < right.slot;
        };
        
        std::sort(SRVs.begin(), SRVs.end(), comparator);
        std::sort(CBVs.begin(), CBVs.end(), comparator);
        std::sort(UAVs.begin(), UAVs.end(), comparator);
	}

	~Reflection()
	{
        IRShaderReflectionDestroy(reflection);
	}
    
    static const IRResourceLocation& GetResourceFromTypeArray(const TArray<IRResourceLocation>& resources, uint32_t slot)
    {
		if (resources.empty())
		{
			GLEAM_ASSERT(false, "Metal: Shader does not use any resource");
			return invalidResource;
		}

        uint32_t left = 0;
        uint32_t right = static_cast<uint32_t>(resources.size() - 1);
        while (left <= right)
        {
            uint32_t middle = (right + left) / 2;
            
            const auto& resource = resources[middle];
            if (resource.slot == slot) { return resource; }
            
            if (resource.slot < slot) { left = middle + 1; }
            else { right = middle - 1; }
        }
        
        GLEAM_ASSERT(false, "Metal: Resource reflection not found in slot {0}", slot);
        return invalidResource;
    }
};

} // namespace Gleam
