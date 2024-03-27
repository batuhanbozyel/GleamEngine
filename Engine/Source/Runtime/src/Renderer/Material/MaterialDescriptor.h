//
//  Material.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "MaterialProperty.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

enum class RenderQueue
{
    Opaque,
    Transparent
};

struct MaterialPassDescriptor
{
    PipelineStateDescriptor pipelineState;
    TString vertexEntry;
    TString fragmentEntry;

    bool operator==(const MaterialPassDescriptor& other) const
    {
        return  pipelineState == other.pipelineState &&
				vertexEntry == other.vertexEntry &&
				fragmentEntry == other.fragmentEntry;
    }
};

struct MaterialDescriptor
{
	TString name;
    TArray<MaterialPassDescriptor> passes;
	TArray<MaterialProperty> properties;
    RenderQueue renderQueue = RenderQueue::Opaque;

    bool operator==(const MaterialDescriptor& other) const
    {
		if (passes.size() != other.passes.size()) return false;
        for (uint32_t i = 0; i < passes.size(); i++)
        {
            if (passes[i] != other.passes[i]) { return false; }
        }

		if (properties.size() != other.properties.size()) return false;
        for (uint32_t i = 0; i < properties.size(); i++)
        {
            if (properties[i] != other.properties[i]) { return false; }
        }

        return renderQueue == other.renderQueue;
    }
};

} // namespace Gleam

template <>
struct std::hash<Gleam::MaterialPassDescriptor>
{
    size_t operator()(const Gleam::MaterialPassDescriptor& descriptor) const
    {
        std::size_t hash = 0;
        Gleam::hash_combine(hash, descriptor.pipelineState);
        Gleam::hash_combine(hash, descriptor.vertexEntry);
        Gleam::hash_combine(hash, descriptor.fragmentEntry);
        return hash;
    }
};

template <>
struct std::hash<Gleam::MaterialDescriptor>
{
    size_t operator()(const Gleam::MaterialDescriptor& descriptor) const
    {
        std::size_t hash = 0;
        Gleam::hash_combine(hash, descriptor.renderQueue);
        
        for (const auto& pass : descriptor.passes)
        {
            Gleam::hash_combine(hash, pass);
        }
        
        for (const auto& property : descriptor.properties)
        {
            Gleam::hash_combine(hash, property);
        }
        
        return hash;
    }
};

GLEAM_TYPE(Gleam::MaterialPassDescriptor, Guid("123C5669-2376-46B4-B5FD-4076ADE86397"))
	GLEAM_FIELD(pipelineState, Serializable())
	GLEAM_FIELD(vertexEntry, Serializable())
	GLEAM_FIELD(fragmentEntry, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::MaterialDescriptor, Guid("37CF7896-D930-435B-A5FF-DF9CEB5C605D"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(passes, Serializable())
	GLEAM_FIELD(properties, Serializable())
	GLEAM_FIELD(renderQueue, Serializable())
GLEAM_END
