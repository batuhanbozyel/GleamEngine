#pragma once
#include "Meta.h"
#include "Primitives.h"
#include "Core/Subsystem.h"

namespace Gleam::Reflection {

template <typename T, typename R, R T::*M>
static constexpr std::size_t OffsetOf()
{
    return reinterpret_cast<size_t>(&(((T*)0)->*M));
}

class Database : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    static PrimitiveType GetPrimitiveType(size_t hash);
    
    static const TStringView GetPrimitiveName(PrimitiveType type);
    
    template<typename T>
    static ClassDescription& CreateClassIfNotExist()
    {
        auto hash = typeid(T).hash_code();
        auto it = mClasses.find(hash);
        if (it != mClasses.end())
        {
            return it->second;
        }
        
        auto type = refl::reflect<T>();
		mTypeNameToHash.insert(mTypeNameToHash.end(), { type.name.c_str(), hash});
        return mClasses.insert(mClasses.end(), { hash, CreateClassDescription(type) })->second;
    }
    
    template<typename T, std::enable_if_t<Traits::IsEnum<T>::value, bool> = true>
    static EnumDescription& CreateEnumIfNotExist()
    {
        auto hash = typeid(T).hash_code();
        auto it = mEnums.find(hash);
        if (it != mEnums.end())
        {
            return it->second;
        }
        
        auto type = refl::reflect<T>();
		mTypeNameToHash.insert(mTypeNameToHash.end(), { type.name.c_str(), hash });
        return mEnums.insert(mEnums.end(), { hash, CreateEnumDescription(type) })->second;
    }
    
    template<typename T, std::enable_if_t<Traits::IsArray<T>::value, bool> = true>
    static ArrayDescription& CreateArrayIfNotExist()
    {
        auto hash = typeid(T).hash_code();
        auto it = mArrays.find(hash);
        if (it != mArrays.end())
        {
            return it->second;
        }

        return mArrays.insert(mArrays.end(), { hash, CreateArrayDescription<T>() })->second;
    }
    
    static const ClassDescription& GetClass(size_t hash);
    
    static const EnumDescription& GetEnum(size_t hash);
    
    static const ArrayDescription& GetArray(size_t hash);

	static size_t GetTypeHash(const TStringView name);
    
private:

	template<typename T>
	static constexpr ClassDescription CreateClassDescription(const refl::type_descriptor<T>& type)
	{
		ClassDescription desc;
        desc.mSize = sizeof(T);
		desc.mName = type.name.c_str();
		desc.mGuid = refl::descriptor::get_attribute<Reflection::Attribute::Guid>(type);

		// resolve fields
        auto fields = refl::util::filter(type.members, [&](auto member) { return refl::descriptor::is_field(member); });
        refl::util::for_each(fields, [&](auto member)
        {
            using ValueType = typename decltype(member)::value_type;
            
            size_t fieldSize = sizeof(ValueType);
            size_t fieldOffset = OffsetOf<T, ValueType, member.pointer>();
            if constexpr (Traits::IsPrimitive<ValueType>::value)
            {
                auto hash = typeid(ValueType).hash_code();
                auto field = PrimitiveField(GetPrimitiveType(hash));
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
            }
            else if constexpr (Traits::IsEnum<ValueType>::value)
            {
                auto hash = typeid(ValueType).hash_code();
                auto field = EnumField(hash);
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
                CreateEnumIfNotExist<ValueType>();
            }
            else if constexpr (Traits::IsArray<ValueType>::value)
            {
                auto hash = typeid(ValueType).hash_code();
                auto field = ArrayField(hash);
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
                CreateArrayIfNotExist<ValueType>();
            }
            else if constexpr (Traits::IsClass<ValueType>::value)
            {
				auto hash = typeid(ValueType).hash_code();
                auto field = ClassField(hash);
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
                auto& classDesc = CreateClassIfNotExist<ValueType>();
                
                if constexpr (Traits::IsContainer<ValueType>::value)
                {
                    using ElementType = ValueType::value_type;
                    CreateArrayIfNotExist<ElementType[1]>();
                    classDesc.mContainerHash = typeid(ElementType[1]).hash_code();
                }
            }
        });
        
        // resolve base classes
        refl::util::for_each(type.bases, [&](auto base)
        {
            desc.mBaseClasses.emplace_back(base);
        });
        
        // resolve attributes
        std::apply([&](auto attrib)
        {
            desc.mAttributes.push_back({ .description = attrib.description,
										 .value = std::make_any<decltype(attrib)>(attrib) });
        }, type.attributes);
		
		return desc;
	}

	template<typename T, size_t N>
    static constexpr FieldDescription CreateFieldDescription(const refl::field_descriptor<T, N>& fieldDesc, const Field& field)
    {
		FieldDescription desc;
        desc.mName = fieldDesc.name.c_str();
        desc.mType = static_cast<FieldType>(field.index());
        desc.mField = field;

        // resolve attributes
        std::apply([&](auto attrib)
        {
            desc.mAttributes.push_back({ .description = attrib.description,
                                    	 .value = std::make_any<decltype(attrib)>(attrib) });
        }, fieldDesc.attributes);

		return desc;
    }
    
    template<typename T, std::enable_if_t<Traits::IsEnum<T>::value, bool> = true>
    static constexpr EnumDescription CreateEnumDescription(const refl::type_descriptor<T>& type)
    {
        EnumDescription desc;
        desc.mName = type.name.c_str();
        desc.mGuid = refl::descriptor::get_attribute<Reflection::Attribute::Guid>(type);
        
        // resolve attributes
        std::apply([&](auto attrib)
        {
            desc.mAttributes.push_back({ .description = attrib.description,
                                         .value = std::make_any<decltype(attrib)>(attrib) });
        }, type.attributes);
        
        return desc;
    }
    
    template<typename T, std::enable_if_t<Traits::IsArray<T>::value, bool> = true>
    static constexpr ArrayDescription CreateArrayDescription()
    {
        using ElementType = std::remove_reference_t<decltype(std::declval<T>()[0])>;
        auto hash = typeid(ElementType).hash_code();
        auto type = refl::reflect<ElementType>();
        
        ArrayDescription desc;
        desc.mName = type.name.c_str();
        desc.mSize = sizeof(T);
        desc.mStride = sizeof(ElementType);
        desc.mHash = hash;
        
        if constexpr (Traits::IsPrimitive<ElementType>::value)
        {
            desc.mType = FieldType::Primitive;
        }
        else if constexpr (Traits::IsEnum<ElementType>::value)
        {
            desc.mType = FieldType::Enum;
			CreateEnumIfNotExist<ElementType>();
        }
        else if constexpr (Traits::IsArray<ElementType>::value)
        {
            desc.mType = FieldType::Array;
			CreateArrayIfNotExist<ElementType>();
        }
        else if constexpr (Traits::IsClass<ElementType>::value)
        {
            desc.mType = FieldType::Class;
			CreateClassIfNotExist<ElementType>();
        }
        return desc;
    }

	static inline HashMap<TStringView, size_t> mTypeNameToHash;
    static inline HashMap<size_t, EnumDescription> mEnums;
    static inline HashMap<size_t, ArrayDescription> mArrays;
    static inline HashMap<size_t, ClassDescription> mClasses;
    static inline HashMap<size_t, PrimitiveType> mPrimitiveTypes;
    static inline HashMap<PrimitiveType, TStringView, EnumClassHash> mPrimitiveNames;
    
};

} // namespace Gleam::Reflection
