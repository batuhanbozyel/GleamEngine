#pragma once
#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Container/String.h"
#include "Reflection/Reflection.h"

namespace GEditor {

class PropertyDrawer
{
public:

	static void DrawScalarControl(const Gleam::TStringView label, const Gleam::Reflection::PrimitiveType type, size_t size, void* value, const void* defaultValue, float columnWidth = 100.0f);

	template<typename T, std::enable_if_t<Gleam::Reflection::Traits::IsPrimitive<T>::value, bool> = true>
	static void DrawScalarControl(const Gleam::TStringView label, T& value, T defaultValue = T(), float columnWidth = 100.0f)
	{
		auto primitiveDesc = Gleam::Reflection::GetPrimitive<T>();
		DrawScalarControl(label, primitiveDesc.Type(), primitiveDesc.GetSize(), &value, &defaultValue, columnWidth);
	}

	static void DrawVec3Control(const Gleam::TStringView label, Gleam::Float3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

	static void DrawColorControl(const Gleam::TStringView label, Gleam::Color& color, float columnWidth = 100.0f);

	static void DrawEnumOptions(const Gleam::TStringView label, const Gleam::Reflection::EnumDescription& enumDesc, void* value, float columnWidth = 100.0f);

	static void DrawClassFields(void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth = 100.0f);

	static void DrawClass(const Gleam::TStringView label, void* component, const Gleam::Reflection::ClassDescription& classDesc);

	using UIFunction = std::function<void()>;
	static void DrawCustom(const Gleam::TStringView label, size_t hash, UIFunction&& uiFunction);
};


} // namespace GEditor
