#include "gpch.h"
#include "BinarySerializer.h"
#include "Container/BinaryBuffer.h"
#include "Renderer/Material/MaterialProperty.h"

using namespace Gleam;

static TStringView QualifiedNameWithoutTemplateDeclaration(const TStringView name)
{
	auto pos = name.find_first_of('<');
	if (pos == TStringView::npos)
	{
		return name;
	}
	return name.substr(0, pos);
}

#pragma region mark SerializeForwardDecl

#pragma region mark SerializeHeaders
static void SerializeHeader(const BinaryHeader& header, FileStream& stream);

static void SerializePrimitiveHeader(Reflection::PrimitiveType type, FileStream& stream);

static void SerializeEnumHeader(const Reflection::EnumDescription& enumDesc, FileStream& stream);

static void SerializeClassHeader(const Reflection::ClassDescription& classDesc, FileStream& stream);

static void SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc, FileStream& stream);
#pragma endregion SerializeHeaders

#pragma region mark SerializeValues
static void SerializePrimitiveValue(const void* obj,
									Reflection::PrimitiveType type,
									FileStream& stream);

static void SerializeEnumValue(const void* obj,
							   const Reflection::EnumDescription& enumDesc,
							   FileStream& stream);

static void SerializeClassFields(const void* obj,
								 const Reflection::ClassDescription& classDesc,
								 FileStream& stream);

static void SerializeArrayElements(const void* obj,
								   const Reflection::ArrayDescription& arrayDesc,
								   FileStream& stream);
#pragma endregion SerializeValues

#pragma region mark SerializeObjects
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
#pragma endregion SerializeObjects

#pragma endregion SerializeForwardDecl

#pragma region mark DeserializeForwardDecl
static void DeserializeHeader(FileStream& stream, BinaryHeader& header);

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

static void DeserializeClassFields(FileStream& stream,
								   const Reflection::ClassDescription& classDesc,
								   void* obj);

static void DeserializeArrayElements(FileStream& stream,
									 const Reflection::ArrayDescription& arrayDesc,
									 void* obj);
#pragma endregion DeserializeForwardDecl

void BinarySerializer::Initialize(Engine* engine)
{
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Float2);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Float3);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Float4);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Color);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Int2);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Int3);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Int4);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Float2x2);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Float3x3);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Float4x4);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Quaternion);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(Guid);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(BufferRange);
	REGISTER_POD_TYPE_BINARY_SERIALIZER(MaterialPropertyValue);

	// Custom serializers
	if constexpr (Reflection::Traits::IsReflected<TString>())
	{
		mCustomSerializers[Reflection::GetClass<TString>().ResolveQualifiedName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			FileStream& stream)
		{
			const auto& str = Reflection::Get<TString>(obj);
			auto len = static_cast<uint32_t>(str.length());

			SerializeClassHeader(classDesc, stream);
			stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
			stream.write(str.data(), str.length());
		};

		mCustomArraySerializers[Reflection::GetClass<TString>().ResolveQualifiedName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			FileStream& stream)
		{
			const auto& str = Reflection::Get<TString>(obj);
			auto len = static_cast<uint32_t>(str.length());
			stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
			stream.write(str.data(), str.length());
		};
	}

	if constexpr (Reflection::Traits::IsReflected<Path>())
	{
		mCustomSerializers[Reflection::GetClass<Path>().ResolveQualifiedName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			FileStream& stream)
		{
			const auto& path = Reflection::Get<Path>(obj);
			const auto& pathStr = path.String();
			auto len = static_cast<uint32_t>(pathStr.length());

			SerializeClassHeader(classDesc, stream);
			stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
			stream.write(pathStr.data(), pathStr.length());
		};

		mCustomArraySerializers[Reflection::GetClass<Path>().ResolveQualifiedName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			FileStream& stream)
		{
			const auto& path = Reflection::Get<Path>(obj);
			const auto& pathStr = path.String();
			auto len = static_cast<uint32_t>(pathStr.length());
			stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
			stream.write(pathStr.data(), pathStr.length());
		};
	}

	if constexpr (Reflection::Traits::IsReflected<BinaryBuffer>())
	{
		mCustomSerializers[Reflection::GetClass<BinaryBuffer>().ResolveQualifiedName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			FileStream& stream)
		{
			const auto& buffer = Reflection::Get<BinaryBuffer>(obj);
			auto size = static_cast<uint64_t>(buffer.size);

			SerializeClassHeader(classDesc, stream);
			stream.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
			stream.write(static_cast<const char*>(buffer.data), size);
		};

		mCustomArraySerializers[Reflection::GetClass<BinaryBuffer>().ResolveQualifiedName()] = [](const void* obj,
			const Reflection::ClassDescription& classDesc,
			FileStream& stream)
		{
			const auto& buffer = Reflection::Get<BinaryBuffer>(obj);
			auto size = static_cast<uint64_t>(buffer.size);
			stream.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
			stream.write(static_cast<const char*>(buffer.data), size);
		};
	}

	if constexpr (Reflection::Traits::IsReflected<eastl::vector<uint8_t>>())
	{
		const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(Reflection::GetClass<eastl::vector<uint8_t>>().ResolveQualifiedName());
		mCustomSerializers[qualifiedName] = [](const void* obj,
											   const Reflection::ClassDescription& classDesc,
											   FileStream& stream)
		{
			auto templateParams = classDesc.ResolveTemplateParameters();
			GLEAM_ASSERT(templateParams.size() == 1, "BinarySerializer: TArray must have exactly one template parameter for element type.");

			const auto& element = templateParams[0];
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			auto arrDesc = Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());
			SerializeArray(arr.data(), arrDesc, stream);
		};

		mCustomArraySerializers[qualifiedName] = [](const void* obj,
													const Reflection::ClassDescription& classDesc,
													FileStream& stream)
		{
			auto templateParams = classDesc.ResolveTemplateParameters();
			GLEAM_ASSERT(templateParams.size() == 1, "BinarySerializer: TArray must have exactly one template parameter for element type.");

			const auto& element = templateParams[0];
			const auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			auto arrDesc = Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());

			auto size = static_cast<uint32_t>(arr.size());
			stream.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
			SerializeArrayElements(arr.data(), arrDesc, stream);
		};
	}
	
	// Custom deserializers
	if constexpr (Reflection::Traits::IsReflected<TString>())
	{
		mCustomDeserializers[Reflection::GetClass<TString>().ResolveQualifiedName()] = [](FileStream& stream,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			BinaryHeader header;
			DeserializeHeader(stream, header);

			uint32_t len = 0;
			stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));

			auto& str = Reflection::Get<TString>(obj);
			str.resize(len);
			stream.read(str.data(), len);
		};

		mCustomArrayDeserializers[Reflection::GetClass<TString>().ResolveQualifiedName()] = [](FileStream& stream,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			uint32_t len = 0;
			stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));

			auto& str = Reflection::Get<TString>(obj);
			str.resize(len);
			stream.read(str.data(), len);
		};
	}

	if constexpr (Reflection::Traits::IsReflected<Path>())
	{
		mCustomDeserializers[Reflection::GetClass<Path>().ResolveQualifiedName()] = [](FileStream& stream,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			BinaryHeader header;
			DeserializeHeader(stream, header);

			uint32_t len = 0;
			stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));

			TString pathStr;
			pathStr.resize(len);
			stream.read(pathStr.data(), len);
			Reflection::Get<Path>(obj) = Path(pathStr);
		};

		mCustomArrayDeserializers[Reflection::GetClass<Path>().ResolveQualifiedName()] = [](FileStream& stream,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			uint32_t len = 0;
			stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));

			TString pathStr;
			pathStr.resize(len);
			stream.read(pathStr.data(), len);
			Reflection::Get<Path>(obj) = Path(pathStr);
		};
	}

	if constexpr (Reflection::Traits::IsReflected<BinaryBuffer>())
	{
		mCustomDeserializers[Reflection::GetClass<BinaryBuffer>().ResolveQualifiedName()] = [](FileStream& stream,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			BinaryHeader header;
			DeserializeHeader(stream, header);

			uint64_t size = 0;
			stream.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));

			auto& buffer = Reflection::Get<BinaryBuffer>(obj);
			buffer.Resize(size);
			stream.read(static_cast<char*>(buffer.data), size);
		};

		mCustomArrayDeserializers[Reflection::GetClass<BinaryBuffer>().ResolveQualifiedName()] = [](FileStream& stream,
			const Reflection::ClassDescription& classDesc,
			void* obj)
		{
			uint64_t size = 0;
			stream.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));

			auto& buffer = Reflection::Get<BinaryBuffer>(obj);
			buffer.Resize(size);
			stream.read(static_cast<char*>(buffer.data), size);
		};
	}

	if constexpr (Reflection::Traits::IsReflected<eastl::vector<uint8_t>>())
	{
		const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(Reflection::GetClass<eastl::vector<uint8_t>>().ResolveQualifiedName());
		mCustomDeserializers[qualifiedName] = [](FileStream& stream,
												 const Reflection::ClassDescription& classDesc,
												 void* obj)
		{
			auto templateParams = classDesc.ResolveTemplateParameters();
			GLEAM_ASSERT(templateParams.size() == 1, "BinarySerializer: TArray must have exactly one template parameter for element type.");

			BinaryHeader header;
			DeserializeHeader(stream, header);

			auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			arr.resize(header.size);

			const auto& element = templateParams[0];
			auto arrDesc = Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());
			DeserializeArrayElements(stream, arrDesc, arr.data());
		};

		mCustomArrayDeserializers[qualifiedName] = [](FileStream& stream,
													  const Reflection::ClassDescription& classDesc,
													  void* obj)
		{
			auto templateParams = classDesc.ResolveTemplateParameters();
			GLEAM_ASSERT(templateParams.size() == 1, "BinarySerializer: TArray must have exactly one template parameter for element type.");

			uint32_t size = 0;
			stream.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));

			auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			arr.resize(size);

			const auto& element = templateParams[0];
			auto arrDesc = Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());
			DeserializeArrayElements(stream, arrDesc, arr.data());
		};
	}
}

void BinarySerializer::Shutdown(Engine* engine)
{
	mCustomSerializers.clear();
	mCustomDeserializers.clear();
}

BinaryHeader BinarySerializer::ParseHeader(FileStream& stream)
{
	BinaryHeader header;
	DeserializeHeader(stream, header);
	return header;
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
	const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName());
	auto it = mCustomSerializers.find(qualifiedName);
	if (it != mCustomSerializers.end())
	{
		it->second(obj, classDesc, stream);
		return true;
	}
	return false;
}

bool BinarySerializer::TryCustomArraySerializer(const void* obj,
												const Reflection::ClassDescription& classDesc,
												FileStream& stream)
{
	const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName());
	auto it = mCustomArraySerializers.find(qualifiedName);
	if (it != mCustomArraySerializers.end())
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
	const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName());
	auto it = mCustomDeserializers.find(qualifiedName);
	if (it != mCustomDeserializers.end())
	{
		it->second(stream, classDesc, obj);
		return true;
	}
	return false;
}

bool BinarySerializer::TryCustomArrayDeserializer(FileStream& stream,
												  const Reflection::ClassDescription& classDesc,
												  void* obj)
{
	const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName());
	auto it = mCustomArrayDeserializers.find(qualifiedName);
	if (it != mCustomArrayDeserializers.end())
	{
		it->second(stream, classDesc, obj);
		return true;
	}
	return false;
}

#pragma region mark SerializeFunctionsImpl

#pragma region mark SerializeHeaders
void SerializeHeader(const BinaryHeader& header, FileStream& stream)
{
	stream.write(reinterpret_cast<const char*>(&header.kind), sizeof(Reflection::MetaType));

	uint32_t len = static_cast<uint32_t>(header.name.length());
	stream.write(reinterpret_cast<const char*>(&len), sizeof(uint32_t));
	stream.write(header.name.data(), len);

	stream.write(reinterpret_cast<const char*>(header.guid.mBytes), sizeof(header.guid.mBytes));
	stream.write(reinterpret_cast<const char*>(&header.version), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(&header.size), sizeof(uint32_t));
}

void SerializePrimitiveHeader(Reflection::PrimitiveType type, FileStream& stream)
{
	Reflection::PrimitiveDescription desc(type);

	BinaryHeader header;
	header.kind = Reflection::MetaType::Primitive;
	header.name = desc.ResolveName();
	header.size = (uint32_t)desc.GetSize();
	SerializeHeader(header, stream);
}

void SerializeEnumHeader(const Reflection::EnumDescription& enumDesc, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Enum;
	header.name = enumDesc.ResolveQualifiedName();
	header.guid = enumDesc.Guid();

	if (enumDesc.HasAttribute<Reflection::Attribute::Version>())
	{
		header.version = enumDesc.GetAttribute<Reflection::Attribute::Version>()->version;
	}
	header.size = static_cast<uint32_t>(enumDesc.GetSize());
	SerializeHeader(header, stream);
}

void SerializeClassHeader(const Reflection::ClassDescription& classDesc, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Class;
	header.name = classDesc.ResolveQualifiedName();
	header.guid = classDesc.Guid();

	if (classDesc.HasAttribute<Reflection::Attribute::Version>())
	{
		header.version = classDesc.GetAttribute<Reflection::Attribute::Version>()->version;
	}
	header.size = static_cast<uint32_t>(classDesc.GetSize());
	SerializeHeader(header, stream);
}

void SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Array;
	if (arrayDesc.ElementType() == Reflection::MetaType::Class)
	{
		const auto classDesc = Reflection::GetClass(arrayDesc.ElementHash());
		if (classDesc->HasAttribute<Reflection::Attribute::Version>())
		{
			header.version = classDesc->GetAttribute<Reflection::Attribute::Version>()->version;
		}
		header.guid = classDesc->Guid();
		header.name = classDesc->ResolveQualifiedName();
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Enum)
	{
		const auto enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());
		if (enumDesc->HasAttribute<Reflection::Attribute::Version>())
		{
			header.version = enumDesc->GetAttribute<Reflection::Attribute::Version>()->version;
		}
		header.guid = enumDesc->Guid();
		header.name = enumDesc->ResolveQualifiedName();
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Primitive)
	{
		const auto& primitiveDesc = Reflection::GetPrimitive(arrayDesc.ElementHash());
		header.name = primitiveDesc.ResolveName();
		header.version = 0;
		header.guid = Guid();
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Array)
	{
		const auto innerDesc = Reflection::GetArray(arrayDesc.ElementHash());
		header.name = "NestedArray";
		header.version = 0;
		header.guid = Guid();
	}
	else
	{
		GLEAM_ASSERT(false, "BinarySerializer: Unknown array element type");
		return;
	}
	header.size = static_cast<uint32_t>(arrayDesc.GetSize());
	SerializeHeader(header, stream);
}
#pragma endregion SerializeHeaders

#pragma region mark SerializeValues
void SerializePrimitiveValue(const void* obj,
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

void SerializeEnumValue(const void* obj,
						const Reflection::EnumDescription& enumDesc,
						FileStream& stream)
{
	stream.write(reinterpret_cast<const char*>(obj), enumDesc.GetSize());
}

void SerializeClassFields(const void* obj,
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
				case Reflection::MetaType::Class:
				{
					const auto fieldDesc = Reflection::GetClass(field.TypeHash());
					if (BinarySerializer::TryCustomSerializer(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream) == false)
					{
						SerializeClass(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream);
					}
					break;
				}
				case Reflection::MetaType::Array:
				{
					const auto fieldDesc = Reflection::GetArray(field.TypeHash());
					SerializeArray(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream);
					break;
				}
				case Reflection::MetaType::Enum:
				{
					const auto fieldDesc = Reflection::GetEnum(field.TypeHash());
					SerializeEnum(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream);
					break;
				}
				case Reflection::MetaType::Primitive:
				{
					auto primitiveType = Reflection::GetPrimitiveType(field.TypeHash());
					SerializePrimitive(OffsetPointer(obj, field.GetOffset()), primitiveType, stream);
					break;
				}
				default:
					continue;
			}
		}
	}
}

void SerializeArrayElements(const void* obj,
							const Reflection::ArrayDescription& arrayDesc,
							FileStream& stream)
{
	if (arrayDesc.ElementType() == Reflection::MetaType::Primitive)
	{
		stream.write(reinterpret_cast<const char*>(obj), arrayDesc.GetSize());
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Array)
	{
		const auto innerDesc = Reflection::GetArray(arrayDesc.ElementHash());
		for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += innerDesc->GetSize())
		{
			SerializeArrayElements(OffsetPointer(obj, elementOffset), *innerDesc, stream);
		}
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Class)
	{
		const auto& innerDesc = Reflection::GetClass(arrayDesc.ElementHash());
		for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += innerDesc->GetSize())
		{
			if (BinarySerializer::TryCustomArraySerializer(OffsetPointer(obj, elementOffset), *innerDesc, stream) == false)
			{
				SerializeClassFields(OffsetPointer(obj, elementOffset), *innerDesc, stream);
			}
		}
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Enum)
	{
		stream.write(reinterpret_cast<const char*>(obj), arrayDesc.GetSize());
	}
	else
	{
		GLEAM_ASSERT(false, "JSONSerializer: Unknown object kind");
		return;
	}
}
#pragma endregion SerializeValues

#pragma region mark SerializeObjects
void SerializePrimitive(const void* obj,
						Reflection::PrimitiveType type,
						FileStream& stream)
{
	SerializePrimitiveHeader(type, stream);
	SerializePrimitiveValue(obj, type, stream);
}

void SerializeEnum(const void* obj,
				   const Reflection::EnumDescription& enumDesc,
				   FileStream& stream)
{
	SerializeEnumHeader(enumDesc, stream);
	SerializeEnumValue(obj, enumDesc, stream);
}

void SerializeClass(const void* obj,
					const Reflection::ClassDescription& classDesc,
					FileStream& stream)
{
	SerializeClassHeader(classDesc, stream);
	SerializeClassFields(obj, classDesc, stream);
}

void SerializeArray(const void* obj,
					const Reflection::ArrayDescription& arrayDesc,
					FileStream& stream)
{
	SerializeArrayHeader(arrayDesc, stream);
	SerializeArrayElements(obj, arrayDesc, stream);
}
#pragma endregion SerializeObjects

#pragma endregion SerializeFunctionsImpl

#pragma region mark DeserializeFunctionsImpl
void DeserializePrimitive(FileStream& stream,
						  Reflection::PrimitiveType type,
						  void* obj)
{
	BinaryHeader header;
	DeserializeHeader(stream, header);

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
	BinaryHeader header;
	DeserializeHeader(stream, header);
	stream.read(reinterpret_cast<char*>(obj), enumDesc.GetSize());
}

void DeserializeClass(FileStream& stream,
					  const Reflection::ClassDescription& classDesc,
					  void* obj)
{
	BinaryHeader header;
	DeserializeHeader(stream, header);

	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		DeserializeClass(stream, base, obj);
	}

	for (const auto& fieldDesc : classDesc.ResolveFields())
	{
		if (fieldDesc.GetType() == Reflection::MetaType::Primitive)
		{
			auto primitiveType = Reflection::GetPrimitiveType(fieldDesc.TypeHash());
			DeserializePrimitive(stream, primitiveType, OffsetPointer(obj, fieldDesc.GetOffset()));
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Array)
		{
			const auto desc = Reflection::GetArray(fieldDesc.TypeHash());
			DeserializeArray(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Class)
		{
			const auto desc = Reflection::GetClass(fieldDesc.TypeHash());
			if (BinarySerializer::TryCustomDeserializer(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset())) == false)
			{
				DeserializeClass(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
			}
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Enum)
		{
			const auto desc = Reflection::GetEnum(fieldDesc.TypeHash());
			DeserializeEnum(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
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
	BinaryHeader header;
	DeserializeHeader(stream, header);
	DeserializeArrayElements(stream, arrayDesc, obj);
}

void DeserializeArrayElements(FileStream& stream,
							  const Reflection::ArrayDescription& arrayDesc,
							  void* obj)
{
	switch (arrayDesc.ElementType())
	{
		case Reflection::MetaType::Primitive:
		{
			stream.read(reinterpret_cast<char*>(obj), arrayDesc.GetSize());
			return;
		}
		case Reflection::MetaType::Array:
		{
			const auto innerDesc = Reflection::GetArray(arrayDesc.ElementHash());
			for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += innerDesc->GetSize())
			{
				DeserializeArrayElements(stream, *innerDesc, OffsetPointer(obj, elementOffset));
			}
			return;
		}
		case Reflection::MetaType::Class:
		{
			const auto classDesc = Reflection::GetClass(arrayDesc.ElementHash());
			for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += classDesc->GetSize())
			{
				if (BinarySerializer::TryCustomArrayDeserializer(stream, *classDesc, OffsetPointer(obj, elementOffset)) == false)
				{
					DeserializeClassFields(stream, *classDesc, OffsetPointer(obj, elementOffset));
				}
			}
			return;
		}
		case Reflection::MetaType::Enum:
		{
			stream.read(reinterpret_cast<char*>(obj), arrayDesc.GetSize());
			return;
		}
		default:
		{
			GLEAM_ASSERT(false, "BinarySerializer: Unknown object kind");
			return;
		}
	}
}

void DeserializeClassFields(FileStream& stream,
							const Reflection::ClassDescription& classDesc,
							void* obj)
{
	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		DeserializeClass(stream, base, obj);
	}

	for (const auto& fieldDesc : classDesc.ResolveFields())
	{
		if (fieldDesc.GetType() == Reflection::MetaType::Primitive)
		{
			auto primitiveType = Reflection::GetPrimitiveType(fieldDesc.TypeHash());
			DeserializePrimitive(stream, primitiveType, OffsetPointer(obj, fieldDesc.GetOffset()));
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Array)
		{
			const auto desc = Reflection::GetArray(fieldDesc.TypeHash());
			DeserializeArray(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Class)
		{
			const auto desc = Reflection::GetClass(fieldDesc.TypeHash());
			if (BinarySerializer::TryCustomDeserializer(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset())) == false)
			{
				DeserializeClass(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
			}
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Enum)
		{
			const auto desc = Reflection::GetEnum(fieldDesc.TypeHash());
			DeserializeEnum(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
		}
		else
		{
			GLEAM_ASSERT(false, "BinarySerializer: Unknown object kind");
			continue;
		}
	}
}

void DeserializeHeader(FileStream& stream, BinaryHeader& header)
{
	stream.read(reinterpret_cast<char*>(&header.kind), sizeof(Reflection::MetaType));

	uint32_t len = 0;
	stream.read(reinterpret_cast<char*>(&len), sizeof(uint32_t));
	header.name.resize(len);
	stream.read(header.name.data(), len);

	stream.read(reinterpret_cast<char*>(header.guid.mBytes), sizeof(header.guid.mBytes));
	stream.read(reinterpret_cast<char*>(&header.version), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&header.size), sizeof(uint32_t));
}
#pragma endregion DeserializeFunctionsImpl
