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

GENUM(MaterialPropertyType, "C8CF1F83-7A80-4156-BA7B-947208CB69B6", Serializable)
{
	GITEM(Scalar, "D14F5C28-A0E6-4B84-9E18-DF62A3E85C91"),
	GITEM(Float2, "BD64E2F3-AC39-44F9-B87E-C92D62DB5E87"),
	GITEM(Float3, "13F7E85B-6734-4C5F-A984-A0D5E7B9B354"),
	GITEM(Float4, "5CF9AE01-5B85-4BDF-AC71-9BD6F4C8DB2E"),
	GITEM(Texture2D, "A8E0D167-47CC-4D27-9B92-41E6850CFE5D")
};

GSTRUCT(MaterialPropertyValue, "FBEA0802-00F8-479F-9E59-2A157C8A8EF8", Serializable)
{
	union
	{
		float scalar;
		Float2 float2;
		Float3 float3;
		Float4 float4;
		AssetReference texture;
		
		GFIELD("7C4BE305-9D93-482A-B1A5-6E39F24DA017", Serializable)
		uint32_t value[4];
	};

	MaterialPropertyValue()
	{
		memset(value, 0, sizeof(value));
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

GSTRUCT(MaterialProperty, "A69E7110-6B1B-41B9-ACFB-AA363C9A0943", Serializable)
{
	GFIELD("F25D1E13-8C2A-4B46-8A97-1EDF61F75A34", Serializable)
	TString name;
	
	GFIELD("0C3DB9A7-A283-4D5F-981E-5A4FCAEE3D62", Serializable)
	MaterialPropertyType type;
	
	GFIELD("B729ED18-F647-4A65-95E2-E8B30D47FD2A", Serializable)
	MaterialPropertyValue value;
};

static_assert(sizeof(MaterialPropertyValue) == sizeof(MaterialPropertyValue::value), "Material property value is greater than serialized value");

} // namespace Gleam
