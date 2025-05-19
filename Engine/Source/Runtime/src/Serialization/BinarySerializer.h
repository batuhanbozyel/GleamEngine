#pragma once
#include "Core/Subsystem.h"
#include "Reflection/Meta.h"

namespace Gleam {

struct BinaryHeader
{
	TString name = "";
	Guid guid = Guid::InvalidGuid();
	Reflection::MetaType kind = Reflection::MetaType::Invalid;
	uint32_t version = 0;
	uint32_t size = 0;
};

class BinarySerializer final : public EngineSubsystem
{
public:
	
	virtual void Initialize(Engine* engine) override;
	
	virtual void Shutdown() override;

	BinaryHeader ParseHeader(FileStream& stream);
    
	template<typename T>
	void Serialize(const T& object, FileStream& stream)
	{
		const auto& classDesc = Reflection::GetClass<T>();
		Serialize(&object, classDesc, stream);
	}
	
	void Serialize(const void* obj, const Reflection::ClassDescription& classDesc, FileStream& stream);
    
	template<typename T>
	T Deserialize(FileStream& stream)
	{
		T object{};
		const auto& classDesc = Reflection::GetClass<T>();
		Deserialize(stream, classDesc, &object);
		return object;
	}
	
	void Deserialize(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj);
	
	static bool TryCustomSerializer(const void* obj, const Reflection::ClassDescription& classDesc, FileStream& stream);

	static bool TryCustomArraySerializer(const void* obj, const Reflection::ClassDescription& classDesc, FileStream& stream);
	
	static bool TryCustomDeserializer(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj);

	static bool TryCustomArrayDeserializer(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj);
	
private:
	
	using SerializerFn = std::function<void(const void* obj,
											const Reflection::ClassDescription& classDesc,
											FileStream& stream)>;

	using DeserializerFn = std::function<void(FileStream& stream,
											  const Reflection::ClassDescription& classDesc,
											  void* obj)>;
	
	static inline HashMap<TStringView, SerializerFn> mCustomSerializers;

	static inline HashMap<TStringView, SerializerFn> mCustomArraySerializers;

	static inline HashMap<TStringView, DeserializerFn> mCustomDeserializers;

	static inline HashMap<TStringView, DeserializerFn> mCustomArrayDeserializers;
    
};

} // namespace Gleam
