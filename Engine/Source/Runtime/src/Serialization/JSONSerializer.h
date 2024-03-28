#pragma once

namespace Gleam {

class JSONSerializer final
{
public:

    template<typename T>
    static TString Serialize(const T& object)
    {
        const auto& classDesc = Reflection::GetClass<T>();
        return Serialize(&object, classDesc);
    }
    
    static TString Serialize(const void* obj, const Reflection::ClassDescription& desc);
};

} // namespace Gleam
