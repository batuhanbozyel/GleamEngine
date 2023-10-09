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
    TArray<MaterialPassDescriptor> passes;
	TArray<MaterialProperty> properties;
    RenderQueue renderQueue;

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
        Gleam::hash_combine(hash, std::hash<Gleam::PipelineStateDescriptor>()(descriptor.pipelineState));
        Gleam::hash_combine(hash, std::hash<Gleam::TString>()(descriptor.vertexEntry));
        Gleam::hash_combine(hash, std::hash<Gleam::TString>()(descriptor.fragmentEntry));
        return hash;
    }
};

template <>
struct std::hash<Gleam::MaterialDescriptor>
{
    size_t operator()(const Gleam::MaterialDescriptor& descriptor) const
    {
        std::size_t hash = 0;
        Gleam::hash_combine(hash, std::hash<int>()(static_cast<int>(descriptor.renderQueue)));
        
        for (const auto& pass : descriptor.passes)
        {
            Gleam::hash_combine(hash, std::hash<Gleam::MaterialPassDescriptor>()(pass));
        }
        
        for (const auto& property : descriptor.properties)
        {
            Gleam::hash_combine(hash, std::hash<Gleam::MaterialProperty>()(property));
        }
        
        return hash;
    }
};
