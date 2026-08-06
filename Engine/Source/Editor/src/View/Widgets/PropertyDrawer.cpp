#include "PropertyDrawer.h"
#include "AssetIcon.h"

#include "EAssets/EAssetManager.h"

#include "Core/Globals.h"
#include "Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <Runtime.Reflection.generated.h>

using namespace GEditor;

static Gleam::TStringView QualifiedNameWithoutTemplateDeclaration(const Gleam::TStringView name)
{
	auto pos = name.find_first_of('<');
	if (pos == Gleam::TStringView::npos)
	{
		return name;
	}
	return name.substr(0, pos);
}

static size_t ArrayElementSize(const Gleam::Reflection::ArrayDescription& arrayDesc)
{
	switch (arrayDesc.ElementType())
	{
		case Gleam::Reflection::MetaType::Primitive:
			return Gleam::Reflection::GetPrimitive(arrayDesc.ElementHash()).GetSize();
		case Gleam::Reflection::MetaType::Enum:
			return Gleam::Reflection::GetEnum(arrayDesc.ElementHash())->GetSize();
		case Gleam::Reflection::MetaType::Class:
			return Gleam::Reflection::GetClass(arrayDesc.ElementHash())->GetSize();
		case Gleam::Reflection::MetaType::Array:
			return Gleam::Reflection::GetArray(arrayDesc.ElementHash())->GetSize();
		default:
			return 0;
	}
}

static bool IsMultiEditable(Gleam::Reflection::MetaType type, uint32_t typeHash);

static bool IsMultiEditableClass(const Gleam::Reflection::ClassDescription& classDesc)
{
	// TArray and TString own heap storage, mirroring them onto another instance needs a real copy
	if (classDesc.IsTemplate())
	{
		return false;
	}

	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		if (IsMultiEditableClass(baseClass) == false)
		{
			return false;
		}
	}

	for (const auto& field : classDesc.ResolveFields())
	{
		if (IsMultiEditable(field.GetType(), field.TypeHash()) == false)
		{
			return false;
		}
	}
	return true;
}

static bool IsMultiEditable(Gleam::Reflection::MetaType type, uint32_t typeHash)
{
	switch (type)
	{
		case Gleam::Reflection::MetaType::Primitive:
		case Gleam::Reflection::MetaType::Enum:
			return true;
		case Gleam::Reflection::MetaType::Array:
		{
			const auto arrayDesc = Gleam::Reflection::GetArray(typeHash);
			return IsMultiEditable(arrayDesc->ElementType(), arrayDesc->ElementHash());
		}
		case Gleam::Reflection::MetaType::Class:
			return IsMultiEditableClass(*Gleam::Reflection::GetClass(typeHash));
		default:
			return false;
	}
}

static bool HasReflectedFields(const Gleam::Reflection::ClassDescription& classDesc)
{
	if (classDesc.ResolveFields().size() > 0)
	{
		return true;
	}

	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		if (HasReflectedFields(baseClass))
		{
			return true;
		}
	}
	return false;
}

static bool AreValuesEqual(Gleam::Reflection::MetaType type, uint32_t typeHash, size_t size, const void* lhs, const void* rhs);

static bool AreClassValuesEqual(const Gleam::Reflection::ClassDescription& classDesc, const void* lhs, const void* rhs)
{
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		if (AreClassValuesEqual(baseClass, lhs, rhs) == false)
		{
			return false;
		}
	}

	for (const auto& field : classDesc.ResolveFields())
	{
		if (AreValuesEqual(field.GetType(), field.TypeHash(), field.GetSize(),
						   Gleam::OffsetPointer(lhs, field.GetOffset()),
						   Gleam::OffsetPointer(rhs, field.GetOffset())) == false)
		{
			return false;
		}
	}
	return true;
}

static bool AreValuesEqual(Gleam::Reflection::MetaType type, uint32_t typeHash, size_t size, const void* lhs, const void* rhs)
{
	// Walking the reflected fields keeps struct padding out of the comparison, types whose
	// storage reflection does not describe fall back to a raw compare of the whole value
	if (type == Gleam::Reflection::MetaType::Class)
	{
		const auto classDesc = Gleam::Reflection::GetClass(typeHash);
		if (HasReflectedFields(*classDesc))
		{
			return AreClassValuesEqual(*classDesc, lhs, rhs);
		}
	}
	return std::memcmp(lhs, rhs, size) == 0;
}

static bool IsFieldMixed(const Gleam::Reflection::FieldDescription& field, Gleam::TArrayView<void*> instances)
{
	const auto lhs = Gleam::OffsetPointer(instances[0], field.GetOffset());
	for (size_t i = 1; i < instances.size(); ++i)
	{
		const auto rhs = Gleam::OffsetPointer(instances[i], field.GetOffset());
		if (AreValuesEqual(field.GetType(), field.TypeHash(), field.GetSize(), lhs, rhs) == false)
		{
			return true;
		}
	}
	return false;
}

static bool IsMixedValue()
{
	return (GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0;
}

const Gleam::HashMap<Gleam::TStringView, PropertyDrawer::DrawFunction>& PropertyDrawer::GetCustomDrawers()
{
	static const auto drawers = []()
	{
		Gleam::HashMap<Gleam::TStringView, DrawFunction> customDrawers;

		customDrawers[Gleam::Reflection::GetClass<Gleam::Color>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawColorControl(label, Gleam::Reflection::Get<Gleam::Color>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::Float3>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawVec3Control(label, Gleam::Reflection::Get<Gleam::Float3>(obj), 0.0f, columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::AssetReference>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawAsset(label, Gleam::Reflection::Get<Gleam::AssetReference>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::TString>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawStringControl(label, Gleam::Reflection::Get<Gleam::TString>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::Guid>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawTextControl(label, Gleam::Reflection::Get<Gleam::Guid>(obj).ToString(), columnWidth);
		};

		const auto arrayName = QualifiedNameWithoutTemplateDeclaration(Gleam::Reflection::GetClass<Gleam::TArray<uint8_t>>().ResolveQualifiedName());
		customDrawers[arrayName] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			auto templateParams = classDesc.ResolveTemplateParameters();
			GLEAM_ASSERT(templateParams.size() == 1, "PropertyDrawer: TArray must have exactly one template parameter for element type.");

			const auto& element = templateParams[0];
			auto& arr = Gleam::Reflection::Get<Gleam::TArray<uint8_t>>(obj);
			auto arrayDesc = Gleam::Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());
			DrawArray(label, arr.data(), arrayDesc, columnWidth);
		};

		return customDrawers;
	}();
	return drawers;
}

bool PropertyDrawer::HasCustomDrawer(const Gleam::Reflection::ClassDescription& classDesc)
{
	const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName());
	const auto& drawers = GetCustomDrawers();
	return drawers.find(qualifiedName) != drawers.end();
}

bool PropertyDrawer::TryCustomDrawer(const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	const auto qualifiedName = QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName());
	const auto& drawers = GetCustomDrawers();
	auto it = drawers.find(qualifiedName);
	if (it != drawers.end())
	{
		it->second(label, obj, classDesc, columnWidth);
		return true;
	}
	return false;
}

void PropertyDrawer::DrawScalarControl(const Gleam::TStringView label, const Gleam::Reflection::PrimitiveType type, size_t size, void* value, const void* defaultValue, float columnWidth)
{
	GLEAM_ASSERT(value, "Value can not be null.");

	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushFont(boldFont);

	if (type != Gleam::Reflection::PrimitiveType::Bool)
	{
		if (ImGui::Button("X", buttonSize))
		{
			if (defaultValue)
			{
				memcpy(value, defaultValue, size);
			}
		}
	}

	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	if (type != Gleam::Reflection::PrimitiveType::Bool)
	{
		ImGui::SameLine();
	}

	// Checkbox draws its own dash for a mixed value, the drags need a format without a conversion
	const char* mixedFormat = IsMixedValue() ? "-" : nullptr;

	switch (type)
	{
		case Gleam::Reflection::PrimitiveType::Bool:
		{
			ImGui::Checkbox("##X", static_cast<bool*>(value));
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int8:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S8, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt8:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U8, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int16:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S16, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt16:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U16, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int32:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S32, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt32:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U32, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int64:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S64, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt64:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U64, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Float:
		{
			// TODO: use Range attribute from reflection
			constexpr float min = 0.0f;
			constexpr float max = 0.0f;
			ImGui::DragScalar("##X", ImGuiDataType_Float, value, 0.05f, &min, &max, mixedFormat ? mixedFormat : "%.2f");
			break;
		}
		case Gleam::Reflection::PrimitiveType::Double:
		{
			// TODO: use Range attribute from reflection
			constexpr double min = 0.0;
			constexpr double max = 0.0;
			ImGui::DragScalar("##X", ImGuiDataType_Double, value, 0.05f, &min, &max, mixedFormat ? mixedFormat : "%.2f");
			break;
		}
		default:
			break;
	}
	
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawVec3Control(const Gleam::TStringView label, Gleam::Float3& values, float resetValue, float columnWidth)
{
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

    ImGui::PushID(buffer);

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", buffer);
    ImGui::NextColumn();

    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	const char* format = IsMixedValue() ? "-" : "%.2f";

    float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
    ImGui::PushFont(boldFont);

	if (ImGui::Button("X", buttonSize))
	{
		values.x = resetValue;
	}

    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.05f, 0.0f, 0.0f, format);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);

    if (ImGui::Button("Y", buttonSize))
    {
		values.y = resetValue;
	}

    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.05f, 0.0f, 0.0f, format);
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);

	if (ImGui::Button("Z", buttonSize))
	{
		values.z = resetValue;
	}

    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.05f, 0.0f, 0.0f, format);
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);

    ImGui::PopID();
}

void PropertyDrawer::DrawColorControl(const Gleam::TStringView label, Gleam::Color& color, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview;
	ImGui::ColorEdit4("##Color", &color.r, flags);

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawStringControl(const Gleam::TStringView label, Gleam::TString& value, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	char valueBuffer[256];
	auto length = Gleam::Math::Min(value.length(), sizeof(valueBuffer) - 1);
	std::memcpy(valueBuffer, value.c_str(), length);
	valueBuffer[length] = '\0';

	if (ImGui::InputText("##X", valueBuffer, sizeof(valueBuffer)))
	{
		value.assign(valueBuffer);
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawTextControl(const Gleam::TStringView label, const Gleam::TStringView value, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", buffer);
	ImGui::NextColumn();

	ImGui::Text("%.*s", static_cast<int>(value.size()), value.data());

	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawEnumOptions(const Gleam::TStringView label, const Gleam::Reflection::EnumDescription& enumDesc, void* value, float columnWidth)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", buffer);
	ImGui::NextColumn();

	float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushItemWidth(ImGui::CalcItemWidth() + buttonSize.x);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	char previewBuffer[64] = "-";
	if (IsMixedValue() == false)
	{
		int currentValue = *static_cast<int*>(value);
		for (const auto& item : enumDesc.Cases())
		{
			if (item.Value() == currentValue)
			{
				if (item.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
				{
					auto prettyName = item.GetAttribute<Gleam::Reflection::Attribute::PrettyName>();
					auto nameLength = strlen(prettyName->name);
					std::memcpy(previewBuffer, prettyName->name, nameLength);
					previewBuffer[nameLength] = '\0';
				}
				else
				{
					auto itemLabel = item.ResolveName();
					std::memcpy(previewBuffer, itemLabel.data(), itemLabel.size());
					previewBuffer[itemLabel.size()] = '\0';
				}
				break;
			}
		}
	}

	if (ImGui::BeginCombo("##", previewBuffer))
	{
		for (const auto& item : enumDesc.Cases())
		{
			bool isSelected = *static_cast<int*>(value) == item.Value();

			char itemBuffer[64];
			if (item.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
			{
				auto prettyName = item.GetAttribute<Gleam::Reflection::Attribute::PrettyName>();
				auto nameLength = strlen(prettyName->name);
				std::memcpy(itemBuffer, prettyName->name, nameLength);
				itemBuffer[nameLength] = '\0';
			}
			else
			{
				auto itemLabel = item.ResolveName();
				std::memcpy(itemBuffer, itemLabel.data(), itemLabel.size());
				itemBuffer[itemLabel.size()] = '\0';
			}

			if (ImGui::Selectable(itemBuffer, isSelected))
			{
				if (enumDesc.GetSize() == sizeof(int64_t))
				{
					*static_cast<int64_t*>(value) = item.Value();
				}
				else
				{
					*static_cast<int*>(value) = static_cast<int>(item.Value());
				}
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawClassFields(void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	DrawClassFields(Gleam::TArrayView<void*>(&obj, 1), classDesc, columnWidth);
}

void PropertyDrawer::DrawClassFields(Gleam::TArrayView<void*> instances, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		DrawClassFields(instances, baseClass, columnWidth);
	}

	Gleam::TArray<uint8_t> scratch;
	for (const auto& field : classDesc.ResolveFields())
	{
		DrawField(field, instances, columnWidth, scratch);
	}
}

void PropertyDrawer::DrawField(const Gleam::Reflection::FieldDescription& field, Gleam::TArrayView<void*> instances, float columnWidth, Gleam::TArray<uint8_t>& scratch)
{
	Gleam::TStringView fieldName;
	if (field.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
	{
		auto prettyName = field.GetAttribute<Gleam::Reflection::Attribute::PrettyName>();
		fieldName = prettyName->name;
	}
	else
	{
		fieldName = field.ResolveName();
	}

	// Nested structs recurse with every instance so the mixed state stays per leaf field
	if (field.GetType() == Gleam::Reflection::MetaType::Class)
	{
		const auto fieldDesc = Gleam::Reflection::GetClass(field.TypeHash());
		if (HasCustomDrawer(*fieldDesc) == false)
		{
			Gleam::TArray<void*> nested(instances.size());
			for (size_t i = 0; i < instances.size(); ++i)
			{
				nested[i] = Gleam::OffsetPointer(instances[i], field.GetOffset());
			}
			DrawClass(fieldName, nested, *fieldDesc, columnWidth);
			return;
		}
	}

	const bool multiEdit = instances.size() > 1;
	const bool editable = multiEdit == false || IsMultiEditable(field.GetType(), field.TypeHash());
	auto fieldPtr = Gleam::OffsetPointer(instances[0], field.GetOffset());

	if (multiEdit)
	{
		scratch.resize(field.GetSize());
		std::memcpy(scratch.data(), fieldPtr, field.GetSize());
		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, IsFieldMixed(field, instances));
		ImGui::BeginDisabled(editable == false);
	}

	switch (field.GetType())
	{
		case Gleam::Reflection::MetaType::Class:
		{
			const auto fieldDesc = Gleam::Reflection::GetClass(field.TypeHash());
			TryCustomDrawer(fieldName, fieldPtr, *fieldDesc, columnWidth);
			break;
		}
		case Gleam::Reflection::MetaType::Array:
		{
			const auto arrayDesc = Gleam::Reflection::GetArray(field.TypeHash());
			DrawArray(fieldName, fieldPtr, *arrayDesc, columnWidth);
			break;
		}
		case Gleam::Reflection::MetaType::Enum:
		{
			const auto enumDesc = Gleam::Reflection::GetEnum(field.TypeHash());
			DrawEnumOptions(fieldName, *enumDesc, fieldPtr, columnWidth);
			break;
		}
		case Gleam::Reflection::MetaType::Primitive:
		{
			constexpr uint64_t defaultValue = 0;
			const auto primitiveDesc = Gleam::Reflection::GetPrimitive(field.TypeHash());
			DrawScalarControl(fieldName, primitiveDesc.Type(), primitiveDesc.GetSize(), fieldPtr, &defaultValue, columnWidth);
			break;
		}
		default:
			break;
	}

	if (multiEdit)
	{
		ImGui::EndDisabled();
		ImGui::PopItemFlag();

		if (editable && std::memcmp(scratch.data(), fieldPtr, field.GetSize()) != 0)
		{
			for (size_t i = 1; i < instances.size(); ++i)
			{
				std::memcpy(Gleam::OffsetPointer(instances[i], field.GetOffset()), fieldPtr, field.GetSize());
			}
		}
	}
}

void PropertyDrawer::DrawClass(const Gleam::TStringView label, void* component, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	DrawClass(label, Gleam::TArrayView<void*>(&component, 1), classDesc, columnWidth);
}

void PropertyDrawer::DrawClass(const Gleam::TStringView label, Gleam::TArrayView<void*> instances, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
    ImGui::Separator();

	size_t hash = classDesc.TypeHash();

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	float outerWidth = ImGui::GetContentRegionAvail().x;
    bool open = ImGui::TreeNodeEx((void*)hash, treeNodeFlags, "%s", buffer);
    ImGui::PopStyleVar();

    if (open)
    {
		float innerWidth = ImGui::GetContentRegionAvail().x;
		// Subtract the tree node indent so nested value columns line up with the parent's.
		float fieldsWidth = columnWidth > 0.0f ? columnWidth - (outerWidth - innerWidth) : innerWidth * 0.3f;
		DrawClassFields(instances, classDesc, fieldsWidth);
        ImGui::TreePop();
    }

	ImGui::PopID();
}

void PropertyDrawer::DrawArrayElements(void* obj, const Gleam::Reflection::ArrayDescription& arrayDesc, float columnWidth)
{
	size_t elementSize = ArrayElementSize(arrayDesc);
	if (elementSize == 0)
	{
		return;
	}

	uint32_t index = 0;
	for (size_t offset = 0; offset < arrayDesc.GetSize(); offset += elementSize, ++index)
	{
		char elementLabel[24];
		snprintf(elementLabel, sizeof(elementLabel), "Element %u", index);

		auto element = Gleam::OffsetPointer(obj, offset);

		ImGui::PushID(static_cast<int>(index));
		switch (arrayDesc.ElementType())
		{
			case Gleam::Reflection::MetaType::Primitive:
			{
				constexpr uint64_t defaultValue = 0;
				const auto primitiveDesc = Gleam::Reflection::GetPrimitive(arrayDesc.ElementHash());
				DrawScalarControl(elementLabel, primitiveDesc.Type(), primitiveDesc.GetSize(), element, &defaultValue, columnWidth);
				break;
			}
			case Gleam::Reflection::MetaType::Enum:
			{
				const auto enumDesc = Gleam::Reflection::GetEnum(arrayDesc.ElementHash());
				DrawEnumOptions(elementLabel, *enumDesc, element, columnWidth);
				break;
			}
			case Gleam::Reflection::MetaType::Class:
			{
				const auto classDesc = Gleam::Reflection::GetClass(arrayDesc.ElementHash());
				if (TryCustomDrawer(elementLabel, element, *classDesc, columnWidth) == false)
				{
					DrawClass(elementLabel, element, *classDesc, columnWidth);
				}
				break;
			}
			case Gleam::Reflection::MetaType::Array:
			{
				const auto innerDesc = Gleam::Reflection::GetArray(arrayDesc.ElementHash());
				DrawArray(elementLabel, element, *innerDesc, columnWidth);
				break;
			}
			default:
				break;
		}
		ImGui::PopID();
	}
}

void PropertyDrawer::DrawArray(const Gleam::TStringView label, void* obj, const Gleam::Reflection::ArrayDescription& arrayDesc, float columnWidth)
{
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
	ImGui::Separator();

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	size_t elementSize = ArrayElementSize(arrayDesc);
	uint32_t elementCount = elementSize > 0 ? static_cast<uint32_t>(arrayDesc.GetSize() / elementSize) : 0u;

	float outerWidth = ImGui::GetContentRegionAvail().x;
	bool open = ImGui::TreeNodeEx(buffer, treeNodeFlags, "%s", buffer);
	ImGui::PopStyleVar();

	if (open)
	{
		float innerWidth = ImGui::GetContentRegionAvail().x;
		float fieldsWidth = columnWidth > 0.0f ? columnWidth - (outerWidth - innerWidth) : innerWidth * 0.3f;
		DrawArrayElements(obj, arrayDesc, fieldsWidth);
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void PropertyDrawer::DrawAsset(const Gleam::TStringView label, Gleam::AssetReference& assetRef, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	const bool mixed = IsMixedValue();

	const AssetItem* item = nullptr;
	if (assetRef.guid != Gleam::Guid::InvalidGuid())
	{
		auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<EAssetManager>();
		item = assetManager->FindAsset(assetRef.guid);
	}

	auto assetType = item ? item->type : Gleam::Guid(Gleam::Guid::InvalidGuid());
	const auto icon = GetAssetIcon(assetType);

	float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	float iconSize = lineHeight * 3.0f;
	float textOffset = (iconSize - ImGui::GetTextLineHeight()) * 0.5f;

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset);
	ImGui::Text("%s", buffer);
	ImGui::NextColumn();

	ImVec2 iconPos = ImGui::GetCursorScreenPos();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::Button(icon.text, ImVec2(iconSize, iconSize));
	ImGui::PopStyleColor(3);

	if (assetRef.guid != Gleam::Guid::InvalidGuid() && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", assetRef.guid.ToString().c_str());
	}

	ImVec2 barStart = ImVec2(iconPos.x, iconPos.y + iconSize + 1.5f);
	auto barColor = ImGui::ColorConvertFloat4ToU32(ImVec4(icon.color.r, icon.color.g, icon.color.b, icon.color.a));
	ImGui::GetWindowDrawList()->AddLine(barStart, ImVec2(barStart.x + iconSize, barStart.y), barColor, 3.0f);

	ImGui::SameLine();
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + textOffset);
	ImGui::Text("%s", mixed ? "-" : (item ? item->name.c_str() : "None"));

	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawCustom(const Gleam::TStringView label, size_t hash, UIFunction&& uiFunction)
{
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
	ImGui::Separator();

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	bool open = ImGui::TreeNodeEx((void*)hash, treeNodeFlags, "%s", buffer);
	ImGui::PopStyleVar();

	if (open)
	{
		auto panelWidth = ImGui::GetContentRegionAvail().x;
		auto labelWidth = panelWidth * 0.3f;
		uiFunction();
		ImGui::TreePop();
	}
}
