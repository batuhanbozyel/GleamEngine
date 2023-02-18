#pragma once
#include "GraphicsObject.h"

namespace Gleam {

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
public:

    Shader(const TString& entryPoint, ShaderStage stage);
    ~Shader();

    ShaderStage GetStage() const
    {
        return mStage;
    }

    const TString& GetEntryPoint() const
    {
        return mEntryPoint;
    }

    struct Reflection;
    const Scope<Reflection>& GetReflection() const
    {
        return mReflection;
    }

private:
    
    Scope<Reflection> mReflection;

    ShaderStage mStage;
    TString mEntryPoint;

};


} // namespace Gleam
