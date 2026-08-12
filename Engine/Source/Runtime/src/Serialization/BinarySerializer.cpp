#include "gpch.h"
#include "BinarySerializer.h"
#include "ReflectionUtils.h"
#include "Container/BinaryBuffer.h"
#include "Renderer/Material/MaterialProperty.h"

using namespace Gleam;

static constexpr uint32_t BinaryFormatMagic = 0x4E494247u;
static constexpr uint32_t BinaryFormatVersion = 1;

static TStringView QualifiedNameWithoutTemplateDeclaration(const TStringView name)
{
	auto pos = name.find_first_of('<');
	if (pos == TStringView::npos)
	{
		return name;
	}
	return name.substr(0, pos);
}

static size_t ResolveElementSize(Reflection::MetaType type, uint32_t typeHash)
{
	switch (type)
	{
		case Reflection::MetaType::Primitive: return Reflection::GetPrimitive(typeHash).GetSize();
		case Reflection::MetaType::Enum:      return Reflection::GetEnum(typeHash)->GetSize();
		case Reflection::MetaType::Class:     return Reflection::GetClass(typeHash)->GetSize();
		case Reflection::MetaType::Array:     return Reflection::GetArray(typeHash)->GetSize();
		default:                              return 0;
	}
}

#pragma region mark SerializeForwardDecl

#pragma region mark SerializeHeaders
static void SerializeHeader(const BinaryHeader& header, FileStream& stream);

static void SerializeMemberGuid(const Reflection::Attribute::Guid& guid, FileStream& stream);

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

static void SerializeArrayValues(const void* obj,
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

static void DeserializeMemberGuid(FileStream& stream, Reflection::Attribute::Guid& guid);

static void DeserializeEnumValue(FileStream& stream,
								 const Reflection::EnumDescription& enumDesc,
								 void* obj);

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

static void DeserializeClassPayload(FileStream& stream,
									std::streampos payloadEnd,
									const Reflection::ClassDescription& classDesc,
									void* obj);

static void DeserializeArrayElements(FileStream& stream,
									 const Reflection::ArrayDescription& arrayDesc,
									 void* obj);

static void DeserializeArrayValues(FileStream& stream,
								   uint32_t elementCount,
								   const Reflection::ArrayDescription& arrayDesc,
								   void* obj);
#pragma endregion DeserializeForwardDecl

#pragma region mark NodeFraming

class BinarySizeScope
{
public:

	explicit BinarySizeScope(FileStream& stream)
		: mStream(stream)
		, mPayloadStart(stream.tellp())
	{

	}

	~BinarySizeScope()
	{
		const auto payloadEnd = mStream.tellp();
		const auto payloadSize = static_cast<uint32_t>(payloadEnd - mPayloadStart);
		mStream.seekp(mPayloadStart - static_cast<std::streamoff>(sizeof(uint32_t)));
		mStream.write(reinterpret_cast<const char*>(&payloadSize), sizeof(uint32_t));
		mStream.seekp(payloadEnd);
	}

private:

	FileStream& mStream;
	std::streampos mPayloadStart;
};

static void SerializePayloadSize(FileStream& stream)
{
	constexpr uint32_t placeholder = 0;
	stream.write(reinterpret_cast<const char*>(&placeholder), sizeof(uint32_t));
}

static uint32_t DeserializePayloadSize(FileStream& stream)
{
	uint32_t payloadSize = 0;
	stream.read(reinterpret_cast<char*>(&payloadSize), sizeof(uint32_t));
	return payloadSize;
}

class BinaryMemberDictionary
{
public:

	struct Member
	{
		Guid guid;
		Reflection::MetaType kind = Reflection::MetaType::Invalid;
		uint32_t typeHash = 0;
		std::streampos nodeStart;
	};

	BinaryMemberDictionary(FileStream& stream, std::streampos payloadEnd)
	{
		Collect(stream, payloadEnd);
	}

	const Member* Find(const Reflection::Attribute::Guid& guid, Reflection::MetaType kind) const
	{
		for (const auto& member : mMembers)
		{
			if (member.guid == guid)
			{
				if (IsKindCompatible(member.kind, kind))
				{
					return &member;
				}
				else
				{
					return nullptr;
				}
			}
		}
		return nullptr;
	}

private:

	void Collect(FileStream& stream, std::streampos payloadEnd)
	{
		uint32_t baseCount = 0;
		stream.read(reinterpret_cast<char*>(&baseCount), sizeof(uint32_t));

		for (uint32_t index = 0; index < baseCount and stream.tellg() < payloadEnd; ++index)
		{
			Guid baseGuid;
			DeserializeMemberGuid(stream, baseGuid);

			BinaryHeader header;
			DeserializeHeader(stream, header);

			const auto baseEnd = stream.tellg() + static_cast<std::streamoff>(header.payloadSize);
			Collect(stream, baseEnd);
			stream.seekg(baseEnd);
		}

		while (stream.tellg() < payloadEnd)
		{
			Guid fieldGuid;
			DeserializeMemberGuid(stream, fieldGuid);

			const auto nodeStart = stream.tellg();
			BinaryHeader header;
			DeserializeHeader(stream, header);

			mMembers.push_back({ fieldGuid, header.kind, header.typeHash, nodeStart });
			stream.seekg(stream.tellg() + static_cast<std::streamoff>(header.payloadSize));
		}
	}

	static bool IsKindCompatible(Reflection::MetaType streamKind, Reflection::MetaType fieldKind)
	{
		if (streamKind == Reflection::MetaType::Invalid)
		{
			return true;
		}
		else if (fieldKind == Reflection::MetaType::Class)
		{
			return streamKind == Reflection::MetaType::Class or streamKind == Reflection::MetaType::Array;
		}
		else
		{
			return streamKind == fieldKind;
		}
	}

	TArray<Member> mMembers;
};

#pragma endregion NodeFraming

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
	}

	// Custom deserializers
	if constexpr (Reflection::Traits::IsReflected<TString>())
	{
		mCustomDeserializers[Reflection::GetClass<TString>().ResolveQualifiedName()] = [](FileStream& stream,
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

			const auto payloadEnd = stream.tellg() + static_cast<std::streamoff>(header.payloadSize);

			uint32_t elementCount = 0;
			stream.read(reinterpret_cast<char*>(&elementCount), sizeof(uint32_t));

			const auto& element = templateParams[0];
			const auto elementSize = ResolveElementSize(element.GetType(), element.TypeHash());

			auto& arr = Reflection::Get<TArray<uint8_t>>(obj);
			arr.resize(elementCount * elementSize);

			auto arrDesc = Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());
			DeserializeArrayValues(stream, elementCount, arrDesc, arr.data());

			stream.seekg(payloadEnd);
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

	uint32_t magic = 0;
	uint32_t formatVersion = 0;
	stream.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&formatVersion), sizeof(uint32_t));

	if (magic != BinaryFormatMagic)
	{
		GLEAM_ASSERT(false, "BinarySerializer: stream is not a binary asset.");
		return header;
	}

	if (formatVersion != BinaryFormatVersion)
	{
		GLEAM_ASSERT(false, "BinarySerializer: unsupported binary format version.");
		return header;
	}

	DeserializeHeader(stream, header);
	return header;
}

void BinarySerializer::Serialize(const void* obj, const Reflection::ClassDescription& classDesc, FileStream& stream)
{
	stream.write(reinterpret_cast<const char*>(&BinaryFormatMagic), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(&BinaryFormatVersion), sizeof(uint32_t));

	SerializeClass(obj, classDesc, stream);
}

void BinarySerializer::Deserialize(FileStream& stream, const Reflection::ClassDescription& classDesc, void* obj)
{
	uint32_t magic = 0;
	uint32_t formatVersion = 0;
	stream.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&formatVersion), sizeof(uint32_t));

	if (magic != BinaryFormatMagic or formatVersion != BinaryFormatVersion)
	{
		GLEAM_ASSERT(false, "BinarySerializer: unsupported binary asset.");
		return;
	}

	DeserializeClass(stream, classDesc, obj);
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

#pragma region mark SerializeFunctionsImpl

#pragma region mark SerializeHeaders
void SerializeHeader(const BinaryHeader& header, FileStream& stream)
{
	stream.write(reinterpret_cast<const char*>(&header.kind), sizeof(Reflection::MetaType));
	stream.write(reinterpret_cast<const char*>(&header.typeHash), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(header.guid.mBytes), sizeof(header.guid.mBytes));
	stream.write(reinterpret_cast<const char*>(&header.version), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(&header.payloadSize), sizeof(uint32_t));
}

void SerializeMemberGuid(const Reflection::Attribute::Guid& guid, FileStream& stream)
{
	stream.write(reinterpret_cast<const char*>(guid.mBytes), sizeof(guid.mBytes));
}

void SerializePrimitiveHeader(Reflection::PrimitiveType type, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Primitive;
	header.typeHash = Reflection::PrimitiveDescription(type).TypeHash();
	SerializeHeader(header, stream);
}

void SerializeEnumHeader(const Reflection::EnumDescription& enumDesc, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Enum;
	header.typeHash = enumDesc.TypeHash();
	header.guid = enumDesc.Guid();

	if (enumDesc.HasAttribute<Reflection::Attribute::Version>())
	{
		header.version = enumDesc.GetAttribute<Reflection::Attribute::Version>()->version;
	}
	SerializeHeader(header, stream);
}

void SerializeClassHeader(const Reflection::ClassDescription& classDesc, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Class;
	header.typeHash = classDesc.TypeHash();
	header.guid = classDesc.Guid();

	if (classDesc.HasAttribute<Reflection::Attribute::Version>())
	{
		header.version = classDesc.GetAttribute<Reflection::Attribute::Version>()->version;
	}
	SerializeHeader(header, stream);
}

void SerializeArrayHeader(const Reflection::ArrayDescription& arrayDesc, FileStream& stream)
{
	BinaryHeader header;
	header.kind = Reflection::MetaType::Array;
	header.typeHash = arrayDesc.ElementHash();

	if (arrayDesc.ElementType() == Reflection::MetaType::Class)
	{
		const auto classDesc = Reflection::GetClass(arrayDesc.ElementHash());
		header.guid = classDesc->Guid();
		if (classDesc->HasAttribute<Reflection::Attribute::Version>())
		{
			header.version = classDesc->GetAttribute<Reflection::Attribute::Version>()->version;
		}
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Enum)
	{
		const auto enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());
		header.guid = enumDesc->Guid();
		if (enumDesc->HasAttribute<Reflection::Attribute::Version>())
		{
			header.version = enumDesc->GetAttribute<Reflection::Attribute::Version>()->version;
		}
	}
	SerializeHeader(header, stream);
}
#pragma endregion SerializeHeaders

#pragma region mark SerializeValues
void SerializePrimitiveValue(const void* obj,
							 Reflection::PrimitiveType type,
							 FileStream& stream)
{
	const auto size = Reflection::PrimitiveDescription(type).GetSize();
	if (size > 0)
	{
		stream.write(reinterpret_cast<const char*>(obj), size);
	}
	else
	{
		GLEAM_ASSERT(false, "BinarySerializer: Unknown primitive type");
	}
}

void SerializeEnumValue(const void* obj,
						const Reflection::EnumDescription& enumDesc,
						FileStream& stream)
{
	SerializeMemberGuid(ReflectionUtils::ResolveCaseGuid(obj, enumDesc), stream);
}

void SerializeClassFields(const void* obj,
						  const Reflection::ClassDescription& classDesc,
						  FileStream& stream)
{
	const auto baseClasses = classDesc.ResolveBaseClasses();
	auto baseCount = static_cast<uint32_t>(baseClasses.size());
	stream.write(reinterpret_cast<const char*>(&baseCount), sizeof(uint32_t));

	for (const auto& baseClass : baseClasses)
	{
		SerializeMemberGuid(baseClass.Guid(), stream);
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
					SerializeMemberGuid(field.Guid(), stream);
					SerializeClass(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream);
					break;
				}
				case Reflection::MetaType::Array:
				{
					const auto fieldDesc = Reflection::GetArray(field.TypeHash());
					SerializeMemberGuid(field.Guid(), stream);
					SerializeArray(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream);
					break;
				}
				case Reflection::MetaType::Enum:
				{
					const auto fieldDesc = Reflection::GetEnum(field.TypeHash());
					SerializeMemberGuid(field.Guid(), stream);
					SerializeEnum(OffsetPointer(obj, field.GetOffset()), *fieldDesc, stream);
					break;
				}
				case Reflection::MetaType::Primitive:
				{
					auto primitiveType = Reflection::GetPrimitiveType(field.TypeHash());
					SerializeMemberGuid(field.Guid(), stream);
					SerializePrimitive(OffsetPointer(obj, field.GetOffset()), primitiveType, stream);
					break;
				}
				default:
				{
					GLEAM_ASSERT(false, "BinarySerializer: Unknown object kind");
					break;
				}
			}
		}
	}
}

void SerializeArrayElements(const void* obj,
							const Reflection::ArrayDescription& arrayDesc,
							FileStream& stream)
{
	const auto elementSize = ResolveElementSize(arrayDesc.ElementType(), arrayDesc.ElementHash());
	auto elementCount = elementSize > 0 ? static_cast<uint32_t>(arrayDesc.GetSize() / elementSize) : 0u;
	stream.write(reinterpret_cast<const char*>(&elementCount), sizeof(uint32_t));

	SerializeArrayValues(obj, arrayDesc, stream);
}

void SerializeArrayValues(const void* obj,
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
		const auto innerDesc = Reflection::GetClass(arrayDesc.ElementHash());
		for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += innerDesc->GetSize())
		{
			SerializePayloadSize(stream);
			BinarySizeScope scope(stream);
			if (BinarySerializer::TryCustomSerializer(OffsetPointer(obj, elementOffset), *innerDesc, stream) == false)
			{
				SerializeClassFields(OffsetPointer(obj, elementOffset), *innerDesc, stream);
			}
		}
	}
	else if (arrayDesc.ElementType() == Reflection::MetaType::Enum)
	{
		const auto enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());
		for (size_t elementOffset = 0; elementOffset < arrayDesc.GetSize(); elementOffset += enumDesc->GetSize())
		{
			SerializeEnumValue(OffsetPointer(obj, elementOffset), *enumDesc, stream);
		}
	}
	else
	{
		GLEAM_ASSERT(false, "BinarySerializer: Unknown object kind");
	}
}
#pragma endregion SerializeValues

#pragma region mark SerializeObjects
void SerializePrimitive(const void* obj,
						Reflection::PrimitiveType type,
						FileStream& stream)
{
	SerializePrimitiveHeader(type, stream);
	BinarySizeScope scope(stream);
	SerializePrimitiveValue(obj, type, stream);
}

void SerializeEnum(const void* obj,
				   const Reflection::EnumDescription& enumDesc,
				   FileStream& stream)
{
	SerializeEnumHeader(enumDesc, stream);
	BinarySizeScope scope(stream);
	SerializeEnumValue(obj, enumDesc, stream);
}

void SerializeClass(const void* obj,
					const Reflection::ClassDescription& classDesc,
					FileStream& stream)
{
	SerializeClassHeader(classDesc, stream);
	BinarySizeScope scope(stream);
	if (BinarySerializer::TryCustomSerializer(obj, classDesc, stream) == false)
	{
		SerializeClassFields(obj, classDesc, stream);
	}
}

void SerializeArray(const void* obj,
					const Reflection::ArrayDescription& arrayDesc,
					FileStream& stream)
{
	SerializeArrayHeader(arrayDesc, stream);
	BinarySizeScope scope(stream);
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

	const auto payloadEnd = stream.tellg() + static_cast<std::streamoff>(header.payloadSize);
	const auto size = Reflection::PrimitiveDescription(type).GetSize();
	if (size > 0)
	{
		stream.read(reinterpret_cast<char*>(obj), size);
	}
	else
	{
		GLEAM_ASSERT(false, "BinarySerializer: Unknown primitive type");
	}
	stream.seekg(payloadEnd);
}

void DeserializeEnum(FileStream& stream,
					 const Reflection::EnumDescription& enumDesc,
					 void* obj)
{
	BinaryHeader header;
	DeserializeHeader(stream, header);

	const auto payloadEnd = stream.tellg() + static_cast<std::streamoff>(header.payloadSize);
	DeserializeEnumValue(stream, enumDesc, obj);
	stream.seekg(payloadEnd);
}

void DeserializeEnumValue(FileStream& stream,
						  const Reflection::EnumDescription& enumDesc,
						  void* obj)
{
	Reflection::Attribute::Guid caseGuid;
	DeserializeMemberGuid(stream, caseGuid);

	const auto enumCase = ReflectionUtils::FindCase(enumDesc, caseGuid);
	if (enumCase != nullptr)
	{
		ReflectionUtils::WriteEnumValue(obj, enumDesc, enumCase->Value());
	}
}

void DeserializeClass(FileStream& stream,
					  const Reflection::ClassDescription& classDesc,
					  void* obj)
{
	BinaryHeader header;
	DeserializeHeader(stream, header);

	const auto payloadEnd = stream.tellg() + static_cast<std::streamoff>(header.payloadSize);
	if (BinarySerializer::TryCustomDeserializer(stream, classDesc, obj) == false)
	{
		DeserializeClassPayload(stream, payloadEnd, classDesc, obj);
	}
	stream.seekg(payloadEnd);
}

static void DeserializeClassMembers(FileStream& stream,
									const BinaryMemberDictionary& dictionary,
									const Reflection::ClassDescription& classDesc,
									void* obj)
{
	for (const auto& base : classDesc.ResolveBaseClasses())
	{
		DeserializeClassMembers(stream, dictionary, base, obj);
	}

	for (const auto& fieldDesc : classDesc.ResolveFields())
	{
		if (fieldDesc.HasAttribute<Reflection::Attribute::Serializable>() == false)
		{
			continue;
		}

		const auto member = dictionary.Find(fieldDesc.Guid(), fieldDesc.GetType());
		if (member == nullptr)
		{
			continue;
		}

		stream.seekg(member->nodeStart);
		if (fieldDesc.GetType() == Reflection::MetaType::Primitive)
		{
			if (member->typeHash == fieldDesc.TypeHash())
			{
				auto primitiveType = Reflection::GetPrimitiveType(fieldDesc.TypeHash());
				DeserializePrimitive(stream, primitiveType, OffsetPointer(obj, fieldDesc.GetOffset()));
			}
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Array)
		{
			const auto desc = Reflection::GetArray(fieldDesc.TypeHash());
			DeserializeArray(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
		}
		else if (fieldDesc.GetType() == Reflection::MetaType::Class)
		{
			const auto desc = Reflection::GetClass(fieldDesc.TypeHash());
			DeserializeClass(stream, *desc, OffsetPointer(obj, fieldDesc.GetOffset()));
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

void DeserializeClassPayload(FileStream& stream,
							 std::streampos payloadEnd,
							 const Reflection::ClassDescription& classDesc,
							 void* obj)
{
	const BinaryMemberDictionary dictionary(stream, payloadEnd);
	DeserializeClassMembers(stream, dictionary, classDesc, obj);
}

void DeserializeArray(FileStream& stream,
					  const Reflection::ArrayDescription& arrayDesc,
					  void* obj)
{
	BinaryHeader header;
	DeserializeHeader(stream, header);

	const auto payloadEnd = stream.tellg() + static_cast<std::streamoff>(header.payloadSize);
	if (arrayDesc.ElementType() != Reflection::MetaType::Primitive or header.typeHash == arrayDesc.ElementHash())
	{
		DeserializeArrayElements(stream, arrayDesc, obj);
	}
	stream.seekg(payloadEnd);
}

void DeserializeArrayElements(FileStream& stream,
							  const Reflection::ArrayDescription& arrayDesc,
							  void* obj)
{
	uint32_t elementCount = 0;
	stream.read(reinterpret_cast<char*>(&elementCount), sizeof(uint32_t));

	DeserializeArrayValues(stream, elementCount, arrayDesc, obj);
}

void DeserializeArrayValues(FileStream& stream,
							uint32_t elementCount,
							const Reflection::ArrayDescription& arrayDesc,
							void* obj)
{
	const auto capacity = arrayDesc.GetSize();
	switch (arrayDesc.ElementType())
	{
		case Reflection::MetaType::Primitive:
		{
			const auto primitiveDesc = Reflection::GetPrimitive(arrayDesc.ElementHash());
			const auto streamBytes = static_cast<size_t>(elementCount) * primitiveDesc.GetSize();
			stream.read(reinterpret_cast<char*>(obj), streamBytes < capacity ? streamBytes : capacity);
			return;
		}
		case Reflection::MetaType::Array:
		{
			const auto innerDesc = Reflection::GetArray(arrayDesc.ElementHash());

			size_t offset = 0;
			for (uint32_t index = 0; index < elementCount and offset + innerDesc->GetSize() <= capacity; ++index)
			{
				DeserializeArrayElements(stream, *innerDesc, OffsetPointer(obj, offset));
				offset += innerDesc->GetSize();
			}
			return;
		}
		case Reflection::MetaType::Class:
		{
			const auto classDesc = Reflection::GetClass(arrayDesc.ElementHash());

			size_t offset = 0;
			for (uint32_t index = 0; index < elementCount; ++index)
			{
				const auto payloadSize = DeserializePayloadSize(stream);
				const auto elementEnd = stream.tellg() + static_cast<std::streamoff>(payloadSize);

				if (offset + classDesc->GetSize() <= capacity)
				{
					if (BinarySerializer::TryCustomDeserializer(stream, *classDesc, OffsetPointer(obj, offset)) == false)
					{
						DeserializeClassPayload(stream, elementEnd, *classDesc, OffsetPointer(obj, offset));
					}
					offset += classDesc->GetSize();
				}
				stream.seekg(elementEnd);
			}
			return;
		}
		case Reflection::MetaType::Enum:
		{
			const auto enumDesc = Reflection::GetEnum(arrayDesc.ElementHash());

			size_t offset = 0;
			for (uint32_t index = 0; index < elementCount; ++index)
			{
				if (offset + enumDesc->GetSize() <= capacity)
				{
					DeserializeEnumValue(stream, *enumDesc, OffsetPointer(obj, offset));
					offset += enumDesc->GetSize();
				}
				else
				{
					Reflection::Attribute::Guid discarded;
					DeserializeMemberGuid(stream, discarded);
				}
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

void DeserializeHeader(FileStream& stream, BinaryHeader& header)
{
	stream.read(reinterpret_cast<char*>(&header.kind), sizeof(Reflection::MetaType));
	stream.read(reinterpret_cast<char*>(&header.typeHash), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(header.guid.mBytes), sizeof(header.guid.mBytes));
	stream.read(reinterpret_cast<char*>(&header.version), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&header.payloadSize), sizeof(uint32_t));
}

void DeserializeMemberGuid(FileStream& stream, Reflection::Attribute::Guid& guid)
{
	stream.read(reinterpret_cast<char*>(guid.mBytes), sizeof(guid.mBytes));
}
#pragma endregion DeserializeFunctionsImpl
