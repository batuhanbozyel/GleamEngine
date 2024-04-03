#pragma once
#include "Core/Subsystem.h"

namespace Gleam {

class JSONSerializer final : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;

    template<typename T>
    static TString Serialize(const T& object)
    {
        const auto& classDesc = Reflection::GetClass<T>();
        return Serialize(&object, classDesc);
    }
    
    static TString Serialize(const void* obj, const Reflection::ClassDescription& desc);
    
    static bool TryCustomSerializer(const void* obj,
                                    const Guid& fieldGuid,
                                    const TStringView fieldName,
                                    const Reflection::ClassField& classField,
                                    void* userData);
    
private:
    
    using SerializerFn = std::function<void(const void* obj,
                                            const Guid& fieldGuid,
                                            const TStringView fieldName,
                                            const Reflection::ClassDescription& classDesc,
                                            void* userData)>;

    static inline HashMap<size_t, SerializerFn> mCustomSerializers;
    
};

} // namespace Gleam
