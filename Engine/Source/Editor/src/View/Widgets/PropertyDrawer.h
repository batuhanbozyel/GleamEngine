#pragma once
#include "Math/Color.h"
#include "Math/Vector3.h"
#include "Container/Array.h"
#include "Container/Hash.h"
#include "Container/String.h"
#include "Reflection/Reflection.h"
#include "Assets/AssetReference.h"

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

	static void DrawStringControl(const Gleam::TStringView label, Gleam::TString& value, float columnWidth = 100.0f);

	static void DrawTextControl(const Gleam::TStringView label, const Gleam::TStringView value, float columnWidth = 100.0f);

	static void DrawEnumOptions(const Gleam::TStringView label, const Gleam::Reflection::EnumDescription& enumDesc, void* value, float columnWidth = 100.0f);

	static void DrawClassFields(void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth = 100.0f);

	static void DrawClassFields(Gleam::TArrayView<void*> instances, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth = 100.0f);

	static void DrawClass(const Gleam::TStringView label, void* component, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth = 0.0f);

	static void DrawClass(const Gleam::TStringView label, Gleam::TArrayView<void*> instances, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth = 0.0f);

	static void DrawArrayElements(void* obj, const Gleam::Reflection::ArrayDescription& arrayDesc, float columnWidth = 100.0f);

	static void DrawArray(const Gleam::TStringView label, void* obj, const Gleam::Reflection::ArrayDescription& arrayDesc, float columnWidth = 0.0f);

	static void DrawAsset(const Gleam::TStringView label, Gleam::AssetReference& assetRef, float columnWidth = 100.0f);

	using UIFunction = std::function<void()>;
	static void DrawCustom(const Gleam::TStringView label, size_t hash, UIFunction&& uiFunction);

	// Edit tracking, covering the controls drawn since the last BeginEditTracking call
	static void BeginEditTracking();

	static bool EditStarted();

	static bool EditCommitted();

private:

	static void TrackEdit();

	static void MarkEditCommitted();

	static inline bool mEditStarted = false;

	static inline bool mEditCommitted = false;

	using DrawFunction = std::function<void(const Gleam::TStringView label,
											void* obj,
											const Gleam::Reflection::ClassDescription& classDesc,
											float columnWidth)>;

	static const Gleam::HashMap<Gleam::TStringView, DrawFunction>& GetCustomDrawers();

	static bool HasCustomDrawer(const Gleam::Reflection::ClassDescription& classDesc);

	static bool TryCustomDrawer(const Gleam::TStringView label,
								void* obj,
								const Gleam::Reflection::ClassDescription& classDesc,
								float columnWidth);

	static void DrawField(const Gleam::Reflection::FieldDescription& field,
						  Gleam::TArrayView<void*> instances,
						  float columnWidth,
						  Gleam::TArray<uint8_t>& scratch);
};


} // namespace GEditor
