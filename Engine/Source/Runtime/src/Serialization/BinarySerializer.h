#pragma once
#include "Core/Subsystem.h"

namespace Gleam {

struct BinaryHeader
{
	
};

class BinarySerializer final : public EngineSubsystem
{
public:
	
	virtual void Initialize(Engine* engine) override;
	
	virtual void Shutdown() override;
    
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
	
	static bool TryCustomDeserializer(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj);
	
private:
	
	using SerializerFn = std::function<void(const void* obj,
											const Reflection::ClassDescription& classDesc,
											FileStream& stream)>;

	using DeserializerFn = std::function<void(FileStream& stream,
											  const Reflection::ClassDescription& classDesc,
											  void* obj)>;
	
	static inline HashMap<TStringView, SerializerFn> mCustomSerializers;

	static inline HashMap<TStringView, DeserializerFn> mCustomDeserializers;
    
};

} // namespace Gleam
