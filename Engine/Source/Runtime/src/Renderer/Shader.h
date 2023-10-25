#pragma once
#include "GraphicsObject.h"

namespace Gleam {

class GraphicsDevice;

enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

enum ShaderStageFlag
{
    ShaderStage_Vertex = BIT(static_cast<uint32_t>(ShaderStage::Vertex)),
    ShaderStage_Fragment = BIT(static_cast<uint32_t>(ShaderStage::Fragment)),
    ShaderStage_Compute = BIT(static_cast<uint32_t>(ShaderStage::Compute))
};
typedef uint32_t ShaderStageFlagBits;

class Shader final : public GraphicsObject
{
    friend class GraphicsDevice;
    
public:
    
    Shader() = default;
    
    Shader(const Shader& other) = default;
    
    Shader& operator=(const Shader& other) = default;
    
    Shader(const TString& entryPoint, ShaderStage stage)
        : mEntryPoint(entryPoint), mStage(stage)
    {
        
    }

    ShaderStage GetStage() const
    {
        return mStage;
    }

    const TString& GetEntryPoint() const
    {
        return mEntryPoint;
    }

    struct Reflection;
    const Reflection* GetReflection() const
    {
        return mReflection.get();
    }

private:
    
    RefCounted<Reflection> mReflection;

    ShaderStage mStage;
    TString mEntryPoint;

};


} // namespace Gleam

template <>
struct std::hash<Gleam::Shader>
{
    size_t operator()(const Gleam::Shader& shader) const
    {
        size_t hash = 0;
        Gleam::hash_combine(hash, shader.GetEntryPoint());
        Gleam::hash_combine(hash, shader.GetStage());
        return hash;
    }
};
