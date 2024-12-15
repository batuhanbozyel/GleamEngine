#include "gpch.h"
#include "BinarySerializer.h"

using namespace Gleam;

#pragma region mark SerializeForwardDecl
static void SerializePrimitive(const void* obj,
							   Reflection::PrimitiveType type,
							   FileStream& stream);

static void SerializeEnum(const void* obj,
						  const Reflection::EnumDescription& enumDesc,
						  FileStream& stream);

static void SerializeClass(const void* obj,
						   const Reflection::ClassDescription& classDesc,
						   FileStream& stream);

static void SerializeArray(const void* obj,
						   const Reflection::ArrayDescription& arrayDesc,
						   FileStream& stream);
#pragma endregion SerializeForwardDecl

#pragma region mark DeserializeForwardDecl
static void DeserializePrimitive(FileStream& stream,
								 Reflection::PrimitiveType type,
								 void* obj);

static void DeserializeEnum(FileStream& stream,
							const Reflection::EnumDescription& enumDesc,
							void* obj);

static void DeserializeClass(FileStream& stream,
							 const Reflection::ClassDescription& classDesc,
							 void* obj);

static void DeserializeArray(FileStream& stream,
							 const Reflection::ArrayDescription& arrayDesc,
							 void* obj);
#pragma endregion DeserializeForwardDecl

void BinarySerializer::Initialize(Engine* engine)
{
	// Custom serializers
	{
		mCustomSerializers[Reflection::GetClass<Guid>().ResolveName()] = [](const void* obj,
																			const Reflection::ClassDescription& classDesc,
																			FileStream& stream)
		{
			const auto& guid = Reflection::Get<Guid>(obj);
			const auto& bytes = guid.GetBytes();
			stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
		};
		
		mCustomSerializers[Reflection::GetClass<TString>().ResolveName()] = [](const void* obj,
																			   const Reflection::ClassDescription& classDesc,
																			   FileStream& stream)
		{
			const auto& str = Reflection::Get<TString>(obj);
			auto len = static_cast<uint32_t>(str.length());
			stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
			stream.write(str.data(), str.length());
		};
		
		mCustomSerializers[Reflection::GetClass<Filesystem::Path>().ResolveName()] = [](const void* obj,
																						const Reflection::ClassDescription& classDesc,
																						FileStream& stream)
		{
			const auto& path = Reflection::Get<Filesystem::Path>(obj);
			const auto& pathStr = path.string();
			auto len = static_cast<uint32_t>(pathStr.length());
			stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
			stream.write(pathStr.data(), pathStr.length());
		};
		
		mCustomSerializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](const void* obj,
																					   const Reflection::ClassDescription& classDesc,
																					   FileStream& stream)
		{
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
			auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arr.size(), arrDesc.GetStride());
			SerializeArray(arr.data(), containerDesc, stream);
		};
	}
	
	// Custom deserializers
	{
		mCustomDeserializers[Reflection::GetClass<Guid>().ResolveName()] = [](FileStream& stream,
																			  const Reflection::ClassDescription& classDesc,
																			  void* obj)
		{
			TArray<uint8_t, 16> bytes;
			stream.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
			Reflection::Get<Guid>(obj) = Guid(bytes);
		};
		
		mCustomDeserializers[Reflection::GetClass<TString>().ResolveName()] = [](FileStream& stream,
																				 const Reflection::ClassDescription& classDesc,
																				 void* obj)
		{
			uint32_t len = 0;
			stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
			
			auto& str = Reflection::Get<TString>(obj);
			str.resize(len + 1);
			stream.read(str.data(), len);
			str[len] = '\0';
		};
		
		mCustomDeserializers[Reflection::GetClass<Filesystem::Path>().ResolveName()] = [](FileStream& stream,
																						  const Reflection::ClassDescription& classDesc,
																						  void* obj)
		{
			uint32_t len = 0;
			stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
			
			TString pathStr;
			pathStr.resize(len + 1);
			stream.read(pathStr.data(), len);
			pathStr[len] = '\0';
			Reflection::Get<Filesystem::Path>(obj) = Filesystem::Path(pathStr);
		};
		
		mCustomDeserializers[Reflection::GetClass<TArray<uint8_t>>().ResolveName()] = [](FileStream& stream,
																						 const Reflection::ClassDescription& classDesc,
																						 void* obj)
		{
			auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			const auto& arrDesc = Reflection::GetArray(classDesc.ContainerHash());
			auto containerDesc = Reflection::ArrayDescription(arrDesc.ResolveName(), arrDesc.ElementType(), arrDesc.ElementHash(), arr.size(), arrDesc.GetStride());
			DeserializeArray(stream, containerDesc, arr.data());
		};
	}
}

void BinarySerializer::Shutdown()
{
	mCustomSerializers.clear();
	mCustomDeserializers.clear();
}

void BinarySerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc, FileStream& stream)
{
	if (TryCustomSerializer(obj, classDesc, stream) == false)
	{
		SerializeClass(obj, classDesc, stream);
	}
}

void BinarySerializer::Deserialize(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj)
{
	if (TryCustomDeserializer(stream, classDesc, obj) == false)
	{
		DeserializeClass(stream, classDesc, obj);
	}
}

bool BinarySerializer::TryCustomSerializer(const void* obj,
										   const Reflection::ClassDescription& classDesc,
										   FileStream& stream)
{
	auto it = mCustomSerializers.find(classDesc.ResolveName());
	if (it != mCustomSerializers.end())
	{
		it->second(obj, classDesc, stream);
		return true;
	}
	return false;
}

bool BinarySerializer::TryCustomDeserializer(FileStream& stream,
											 const Reflection::ClassDescription& classDesc,
											 void* obj)
{
	auto it = mCustomDeserializers.find(classDesc.ResolveName());
	if (it != mCustomDeserializers.end())
	{
		it->second(stream, classDesc, obj);
		return true;
	}
	return false;
}

#pragma region mark SerializeFunctionsImpl
void SerializePrimitive(const void* obj,
						Reflection::PrimitiveType type,
						FileStream& stream)
{
	switch (type)
	{
		case Reflection::PrimitiveType::Bool:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(bool));
			break;
		case Reflection::PrimitiveType::Int8:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(int8_t));
			break;
		case Reflection::PrimitiveType::WChar:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(wchar_t));
			break;
		case Reflection::PrimitiveType::Char:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(char));
			break;
		case Reflection::PrimitiveType::Int16:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(int16_t));
			break;
		case Reflection::PrimitiveType::Int32:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(int32_t));
			break;
		case Reflection::PrimitiveType::Int64:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(int64_t));
			break;
		case Reflection::PrimitiveType::UInt8:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(uint8_t));
			break;
		case Reflection::PrimitiveType::UInt16:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(uint16_t));
			break;
		case Reflection::PrimitiveType::UInt32:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(uint32_t));
			break;
		case Reflection::PrimitiveType::UInt64:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(uint64_t));
			break;
		case Reflection::PrimitiveType::Float:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(float));
			break;
		case Reflection::PrimitiveType::Double:
			stream.write(reinterpret_cast<const char*>(obj), sizeof(double));
			break;
		default:
			GLEAM_ASSERT(false, "BinarySerializer: Unknown primitive type");
			break;
	}
}

void SerializeEnum(const void* obj,
				   const Reflection::EnumDescription& enumDesc,
				   FileStream& stream)
{
	GLEAM_ASSERT(enumDesc.GetSize() <= sizeof(int64_t), "BinarySerializer: Enum is larger than 8 bytes");
	int64_t value = 0;
	memcpy(&value, obj, enumDesc.GetSize());
	stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void SerializeClass(const void* obj,
					const Reflection::ClassDescription& classDesc,
					FileStream& stream)
{
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		SerializeClass(obj, baseClass, stream);
	}
	
	for (const auto& field : classDesc.ResolveFields())
	{
		if (field.HasAttribute<Reflection::Attribute::Serializable>())
		{
			switch (field.GetType())
			{
				case Reflection::FieldType::Class:
				{
					const auto& classField = field.GetField<Reflection::ClassField>();
					const auto& fieldDesc = Reflection::GetClass(classField.hash);
					if (BinarySerializer::TryCustomSerializer(OffsetPointer(obj, classField.offset), fieldDesc, stream) == false)
					{
						SerializeClass(OffsetPointer(obj, classField.offset), fieldDesc, stream);
					}
					break;
				}
				case Reflection::FieldType::Array:
				{
					const auto& arrayField = field.GetField<Reflection::ArrayField>();
					const auto& arrayDesc = Reflection::GetArray(arrayField.hash);
					SerializeArray(OffsetPointer(obj, arrayField.offset), arrayDesc, stream);
					break;
				}
				case Reflection::FieldType::Enum:
				{
					const auto& enumField = field.GetField<Reflection::EnumField>();
					const auto& enumDesc = Reflection::GetEnum(enumField.hash);
					SerializeEnum(OffsetPointer(obj, enumField.offset), enumDesc, stream);
					break;
				}
				case Reflection::FieldType::Primitive:
				{
					const auto& primitiveField = field.GetField<Reflection::PrimitiveField>();
					auto primitiveType = primitiveField.primitive;
					SerializePrimitive(OffsetPointer(obj, primitiveField.offset), primitiveType, stream);
					break;
				}
				default:
					continue;
			}
		}
	}
}

void SerializeArray(const void* obj,
					const Reflection::ArrayDescription& arrayDesc,
					FileStream& stream)
{
	size_t size = arrayDesc.GetSize();
	stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
	if (arrayDesc.ElementType() == Reflection::FieldType::Primitive)
	{
		stream.write(reinterpret_cast<const char*>(obj), size);
	}
	else if (arrayDesc.ElementType() == Reflection::FieldType::Array)
	{
		const auto& innerDesc = Reflection::GetArray(arrayDesc.ElementHash());
		for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
		{
			SerializeArray(OffsetPointer(obj, elementOffset), innerDesc, stream);
		}
	}
	else if (arrayDesc.ElementType() == Reflection::FieldType::Class)
	{
		const auto& classDesc = Reflection::Database::GetClass(arrayDesc.ElementHash());
		for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
		{
			SerializeClass(OffsetPointer(obj, elementOffset), classDesc, stream);
		}
	}
	else if (arrayDesc.ElementType() == Reflection::FieldType::Enum)
	{
		stream.write(reinterpret_cast<const char*>(obj), size);
	}
	else
	{
		GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
		return;
	}
}
#pragma endregion SerializeFunctionsImpl

#pragma region mark DeserializeFunctionsImpl
void DeserializePrimitive(FileStream& stream,
						  Reflection::PrimitiveType type,
						  void* obj)
{
	switch (type)
	{
		case Reflection::PrimitiveType::Bool:
			stream.read(reinterpret_cast<char*>(obj), sizeof(bool));
			break;
		case Reflection::PrimitiveType::Int8:
			stream.read(reinterpret_cast<char*>(obj), sizeof(int8_t));
			break;
		case Reflection::PrimitiveType::WChar:
			stream.read(reinterpret_cast<char*>(obj), sizeof(wchar_t));
			break;
		case Reflection::PrimitiveType::Char:
			stream.read(reinterpret_cast<char*>(obj), sizeof(char));
			break;
		case Reflection::PrimitiveType::Int16:
			stream.read(reinterpret_cast<char*>(obj), sizeof(int16_t));
			break;
		case Reflection::PrimitiveType::Int32:
			stream.read(reinterpret_cast<char*>(obj), sizeof(int32_t));
			break;
		case Reflection::PrimitiveType::Int64:
			stream.read(reinterpret_cast<char*>(obj), sizeof(int64_t));
			break;
		case Reflection::PrimitiveType::UInt8:
			stream.read(reinterpret_cast<char*>(obj), sizeof(uint8_t));
			break;
		case Reflection::PrimitiveType::UInt16:
			stream.read(reinterpret_cast<char*>(obj), sizeof(uint16_t));
			break;
		case Reflection::PrimitiveType::UInt32:
			stream.read(reinterpret_cast<char*>(obj), sizeof(uint32_t));
			break;
		case Reflection::PrimitiveType::UInt64:
			stream.read(reinterpret_cast<char*>(obj), sizeof(uint64_t));
			break;
		case Reflection::PrimitiveType::Float:
			stream.read(reinterpret_cast<char*>(obj), sizeof(float));
			break;
		case Reflection::PrimitiveType::Double:
			stream.read(reinterpret_cast<char*>(obj), sizeof(double));
			break;
		default:
			GLEAM_ASSERT(false, "BinarySerializer: Unknown primitive type");
			break;
	}
}

void DeserializeEnum(FileStream& stream,
					 const Reflection::EnumDescription& enumDesc,
					 void* obj)
{
	GLEAM_ASSERT(enumDesc.GetSize() <= sizeof(int64_t), "BinarySerializer: Enum is larger than 8 bytes");
	int64_t value = 0;
	stream.read(reinterpret_cast<char*>(&value), sizeof(value));
	memcpy(obj, &value, enumDesc.GetSize());
}

void DeserializeClass(FileStream& stream,
					  const Reflection::ClassDescription& classDesc,
					  void* obj)
{
	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		DeserializeClass(stream, base, obj);
	}

	for (const auto& fieldDesc : classDesc.ResolveFields())
	{
		if (fieldDesc.GetType() == Reflection::FieldType::Primitive)
		{
			const auto& primitiveField = fieldDesc.GetField<Reflection::PrimitiveField>();
			DeserializePrimitive(stream, primitiveField.primitive, OffsetPointer(obj, primitiveField.offset));
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Array)
		{
			const auto& arrayField = fieldDesc.GetField<Reflection::ArrayField>();
			const auto& desc = Reflection::GetArray(arrayField.hash);
			DeserializeArray(stream, desc, OffsetPointer(obj, arrayField.offset));
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Class)
		{
			const auto& classField = fieldDesc.GetField<Reflection::ClassField>();
			const auto& desc = Reflection::GetClass(classField.hash);
			if (BinarySerializer::TryCustomDeserializer(stream, desc, OffsetPointer(obj, classField.offset)) == false)
			{
				DeserializeClass(stream, desc, OffsetPointer(obj, classField.offset));
			}
		}
		else if (fieldDesc.GetType() == Reflection::FieldType::Enum)
		{
			const auto& enumField = fieldDesc.GetField<Reflection::EnumField>();
			const auto& desc = Reflection::GetEnum(enumField.hash);
			DeserializeEnum(stream, desc, OffsetPointer(obj, enumField.offset));
		}
		else
		{
			GLEAM_ASSERT(false, "BinarySerializer: Unknown object kind");
			continue;
		}
	}
}

void DeserializeArray(FileStream& stream,
					  const Reflection::ArrayDescription& arrayDesc,
					  void* obj)
{
	switch (arrayDesc.ElementType())
	{
		case Reflection::FieldType::Primitive:
		{
			auto primitiveType = Reflection::Database::GetPrimitiveType(arrayDesc.ElementHash());
			for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
			{
				DeserializePrimitive(stream, primitiveType, OffsetPointer(obj, elementOffset));
			}
			return;
		}
		case Reflection::FieldType::Array:
		{
			const auto& innerDesc = Reflection::GetArray(arrayDesc.ElementHash());
			for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
			{
				DeserializeArray(stream, innerDesc, OffsetPointer(obj, elementOffset));
			}
			return;
		}
		case Reflection::FieldType::Class:
		{
			const auto& classDesc = Reflection::GetClass(arrayDesc.ElementHash());
			for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
			{
				if (BinarySerializer::TryCustomDeserializer(stream, classDesc, OffsetPointer(obj, elementOffset)) == false)
				{
					DeserializeClass(stream, classDesc, OffsetPointer(obj, elementOffset));
				}
			}
			return;
		}
		case Reflection::FieldType::Enum:
		{
			const auto& enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());
			for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += arrayDesc.GetStride())
			{
				DeserializeEnum(stream, enumDesc, OffsetPointer(obj, elementOffset));
			}
			return;
		}
		default:
		{
			GLEAM_ASSERT(false, "BinarySerializer: Unknown object kind");
			return;
		}
	}
}
#pragma endregion DeserializeFunctionsImpl
