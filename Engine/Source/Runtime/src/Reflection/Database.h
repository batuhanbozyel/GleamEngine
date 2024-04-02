#pragma once
#include "Meta.h"
#include "Primitives.h"
#include "Core/Subsystem.h"

namespace Gleam::Reflection {

class Database : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    static PrimitiveType GetPrimitiveType(size_t hash);
    
    static const TStringView GetPrimitiveName(PrimitiveType type);
    
    template<typename T>
    static const ClassDescription& GetClass()
    {
        auto hash = typeid(T).hash_code();
        auto it = mClasses.find(hash);
        if (it != mClasses.end())
        {
            return it->second;
        }
        
        auto type = refl::reflect<T>();
        return mClasses.insert(mClasses.end(), { hash, CreateClassDescription(type) })->second;
    }
    
    static const ClassDescription& GetClass(size_t hash);
    
private:

	template<typename T>
	static constexpr ClassDescription CreateClassDescription(const refl::type_descriptor<T>& type)
	{
		ClassDescription desc;
        desc.mSize = sizeof(T);
		desc.mName = type.name.c_str();
		desc.mGuid = refl::descriptor::get_attribute<Reflection::Attribute::Guid>(type);

		// resolve fields
        size_t fieldOffset = 0;
        auto fields = refl::util::filter(type.members, [&](auto member) { return refl::descriptor::is_field(member); });
        refl::util::for_each(fields, [&](auto member)
        {
            using ValueType = typename decltype(member)::value_type;
            
            size_t fieldSize = sizeof(ValueType);
            if constexpr (Traits::IsPrimitive<ValueType>::value)
            {
                auto hash = typeid(ValueType).hash_code();
                auto field = PrimitiveField(GetPrimitiveType(hash));
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
            }
            else if (Traits::IsEnum<ValueType>::value)
            {
                auto field = EnumField();
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
            }
            else if (Traits::IsArray<ValueType>::value)
            {
                auto field = ArrayField();
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
            }
            else if (Traits::IsClass<ValueType>::value)
            {
				auto hash = typeid(ValueType).hash_code();
                auto field = ClassField(hash);
                field.offset = fieldOffset;
                field.size = fieldSize;
                desc.mFields.emplace_back(CreateFieldDescription(member, field));
                
                auto classType = refl::reflect<ValueType>();
				mClasses[hash] = CreateClassDescription(classType);
            }
            fieldOffset += fieldSize;
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
    
    static inline HashMap<size_t, ClassDescription> mClasses;
    static inline HashMap<size_t, PrimitiveType> mPrimitiveTypes;
    static inline HashMap<PrimitiveType, TStringView, EnumClassHash> mPrimitiveNames;
    
};

} // namespace Gleam::Reflection
