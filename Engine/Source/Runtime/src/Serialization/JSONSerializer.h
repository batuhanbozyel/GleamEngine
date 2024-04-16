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

	template<typename T>
	static T Deserialize(const TString& json)
	{
		T object = T();
		const auto& classDesc = Reflection::GetClass<T>();
		Deserialize(json, classDesc, &object);
		return object;
	}

    static TString Serialize(const void* obj, const Reflection::ClassDescription& desc);
    
	static void Deserialize(const TString& json, const Reflection::ClassDescription& desc, void* obj);
    
    static bool TryCustomObjectSerializer(const void* obj,
                                          const Guid& fieldGuid,
                                          const TStringView fieldName,
                                          const Reflection::ClassDescription& classDesc,
                                          void* userData);
    
    static bool TryCustomArraySerializer(const void* obj,
                                         const Reflection::ClassDescription& classDesc,
                                         void* userData);

	static bool TryCustomObjectDeserializer(const void* userData,
											const Reflection::ClassDescription& classDesc,
											void* obj);
    
private:
    
    using ObjectSerializerFn = std::function<void(const void* obj,
                                                  const Guid& fieldGuid,
                                                  const TStringView fieldName,
                                                  const Reflection::ClassDescription& classDesc,
                                                  void* userData)>;
    
    using ArraySerializerFn = std::function<void(const void* obj,
                                                 const Reflection::ClassDescription& classDesc,
                                                 void* userData)>;

	using ObjectDeserializerFn = std::function<void(const void* userData,
													const Reflection::ClassDescription& classDesc,
													void* obj)>;

	using ArrayDeserializerFn = std::function<void(const void* userData,
												   const Reflection::ClassDescription& classDesc,
												   void* obj)>;

    static inline HashMap<TStringView, ObjectSerializerFn> mCustomObjectSerializers;
    
    static inline HashMap<TStringView, ArraySerializerFn> mCustomArraySerializers;

	static inline HashMap<TStringView, ObjectDeserializerFn> mCustomObjectDeserializers;
    
};

} // namespace Gleam
