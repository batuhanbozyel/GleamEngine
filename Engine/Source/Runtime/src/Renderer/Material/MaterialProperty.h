//
//  MaterialProperty.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 3.04.2023.
//

#pragma once
#include "Assets/AssetReference.h"

#include <variant>

namespace Gleam {

enum class MaterialPropertyType
{
    Scalar,
    Float2,
    Float3,
    Float4,
    Texture2D
};

struct MaterialPropertyValue
{
	union
	{
		float scalar;
		Float2 float2;
		Float3 float3;
		Float4 float4;
		AssetReference texture;
		uint32_t value[4];
	};

	MaterialPropertyValue()
		: value()
	{

	}

	MaterialPropertyValue(float scalar)
	{
		operator=(scalar);
	}

	MaterialPropertyValue(const Float2& float2)
	{
		operator=(float2);
	}

	MaterialPropertyValue(const Float3& float3)
	{
		operator=(float3);
	}

	MaterialPropertyValue(const Float4& float4)
	{
		operator=(float4);
	}

	MaterialPropertyValue(const AssetReference& texture)
	{
		operator=(texture);
	}

	MaterialPropertyValue& operator=(float v)
	{
		memset(value, 0, sizeof(value));
		scalar = v;
		return *this;
	}

	MaterialPropertyValue& operator=(const Float2& v)
	{
		memset(value, 0, sizeof(value));
		float2 = v;
		return *this;
	}

	MaterialPropertyValue& operator=(const Float3& v)
	{
		memset(value, 0, sizeof(value));
		float3 = v;
		return *this;
	}

	MaterialPropertyValue& operator=(const Float4& v)
	{
		memset(value, 0, sizeof(value));
		float4 = v;
		return *this;
	}

	MaterialPropertyValue& operator=(const AssetReference& v)
	{
		memset(value, 0, sizeof(value));
		texture = v;
		return *this;
	}
};

struct MaterialProperty
{
	TString name;
	MaterialPropertyType type;
	MaterialPropertyValue value;
};

static_assert(sizeof(MaterialPropertyValue) == sizeof(MaterialPropertyValue::value), "Material property value is greater than serialized value");

} // namespace Gleam

GLEAM_ENUM(Gleam::MaterialPropertyType, Guid("C8CF1F83-7A80-4156-BA7B-947208CB69B6"))

GLEAM_TYPE(Gleam::MaterialPropertyValue, Guid("FBEA0802-00F8-479F-9E59-2A157C8A8EF8"))
	GLEAM_FIELD(value, Serializable())
GLEAM_END

GLEAM_TYPE(Gleam::MaterialProperty, Guid("A69E7110-6B1B-41B9-ACFB-AA363C9A0943"))
	GLEAM_FIELD(name, Serializable())
	GLEAM_FIELD(type, Serializable())
	GLEAM_FIELD(value, Serializable())
GLEAM_END
