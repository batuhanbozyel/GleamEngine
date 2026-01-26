#include "PropertyDrawer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <Runtime.Reflection.generated.h>

using namespace GEditor;

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

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
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
	ImGui::SameLine();

	switch (type)
	{
		case Gleam::Reflection::PrimitiveType::Bool:
		{
			ImGui::Checkbox("##X", static_cast<bool*>(value));
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int8:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S8, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt8:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U8, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int16:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S16, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt16:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U16, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int32:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S32, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt32:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U32, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int64:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S64, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt64:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U64, value);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Float:
		{
			// TODO: use Range attribute from reflection
			constexpr float min = 0.0f;
			constexpr float max = 0.0f;
			ImGui::DragScalar("##X", ImGuiDataType_Float, value, 0.05f, &min, &max, "%.2f");
			break;
		}
		case Gleam::Reflection::PrimitiveType::Double:
		{
			// TODO: use Range attribute from reflection
			constexpr double min = 0.0;
			constexpr double max = 0.0;
			ImGui::DragScalar("##X", ImGuiDataType_Double, value, 0.05f, &min, &max, "%.2f");
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

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
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
    ImGui::DragFloat("##X", &values.x, 0.05f, 0.0f, 0.0f, "%.2f");
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
    ImGui::DragFloat("##Y", &values.y, 0.05f, 0.0f, 0.0f, "%.2f");
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
    ImGui::DragFloat("##Z", &values.z, 0.05f, 0.0f, 0.0f, "%.2f");
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

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushItemWidth(ImGui::CalcItemWidth() + buttonSize.x);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	char previewBuffer[64];
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
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		DrawClassFields(obj, baseClass);
	}

	for (const auto& field : classDesc.ResolveFields())
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

		switch (field.GetType())
		{
			case Gleam::Reflection::MetaType::Class:
			{
				const auto fieldDesc = Gleam::Reflection::GetClass(field.TypeHash());
				if (fieldDesc->ResolveQualifiedName() == Gleam::Reflection::GetClass<Gleam::Color>().ResolveQualifiedName())
				{
					DrawColorControl(fieldName, *static_cast<Gleam::Color*>(Gleam::OffsetPointer(obj, field.GetOffset())), columnWidth);
				}
				else if (fieldDesc->ResolveQualifiedName() == Gleam::Reflection::GetClass<Gleam::Float3>().ResolveQualifiedName())
				{
					DrawVec3Control(fieldName, *static_cast<Gleam::Float3*>(Gleam::OffsetPointer(obj, field.GetOffset())), columnWidth);
				}
				else if (fieldDesc->ResolveQualifiedName() == Gleam::Reflection::GetClass<Gleam::AssetReference>().ResolveQualifiedName())
				{
					DrawAsset(fieldName, *static_cast<Gleam::AssetReference*>(Gleam::OffsetPointer(obj, field.GetOffset())), columnWidth);
				}
				else
				{
					DrawClass(fieldName, Gleam::OffsetPointer(obj, field.GetOffset()), *fieldDesc);
				}
				break;
			}
			case Gleam::Reflection::MetaType::Array:
			{
				break;
			}
			case Gleam::Reflection::MetaType::Enum:
			{
				const auto enumDesc = Gleam::Reflection::GetEnum(field.TypeHash());
				DrawEnumOptions(fieldName, *enumDesc, Gleam::OffsetPointer(obj, field.GetOffset()), columnWidth);
				break;
			}
			case Gleam::Reflection::MetaType::Primitive:
			{
				constexpr uint64_t defaultValue = 0;
				const auto primitiveDesc = Gleam::Reflection::GetPrimitive(field.TypeHash());
				DrawScalarControl(fieldName, primitiveDesc.Type(), primitiveDesc.GetSize(), Gleam::OffsetPointer(obj, field.GetOffset()), &defaultValue, columnWidth);
				break;
			}
			default:
				continue;
		}
	}
}

void PropertyDrawer::DrawClass(const Gleam::TStringView label, void* component, const Gleam::Reflection::ClassDescription& classDesc)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
    ImGui::Separator();

	size_t hash = classDesc.TypeHash();

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

    bool open = ImGui::TreeNodeEx((void*)hash, treeNodeFlags, "%s", buffer);
    ImGui::PopStyleVar();

    if (open)
    {
		auto panelWidth = ImGui::GetContentRegionAvail().x;
		auto labelWidth = panelWidth * 0.3f;
		DrawClassFields(component, classDesc, labelWidth);
        ImGui::TreePop();
    }
}

void PropertyDrawer::DrawAsset(const Gleam::TStringView label, Gleam::AssetReference& assetRef, float columnWidth)
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

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	ImGui::Text("%s", assetRef.guid.ToString().c_str());

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawCustom(const Gleam::TStringView label, size_t hash, UIFunction&& uiFunction)
{
	const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
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