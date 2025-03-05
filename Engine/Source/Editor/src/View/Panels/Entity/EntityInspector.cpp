//
//  EntityInspector.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "EntityInspector.h"

#include <imgui.h>
#include <imgui_internal.h>

using namespace GEditor;

static void DrawScalarControl(const Gleam::TStringView label, const Gleam::Reflection::PrimitiveField field, void* value, uint64_t resetValue = 0u, float columnWidth = 100.0f)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGui::PushID(label.data());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", label.data());
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushFont(boldFont);

	if (field.primitive != Gleam::Reflection::PrimitiveType::Bool)
	{
		if (ImGui::Button("X", buttonSize))
		{
			memcpy(value, &resetValue, field.size);
		}
	}

	ImGui::PopFont();
	ImGui::PopStyleColor(3);
	ImGui::SameLine();


	switch (field.primitive)
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

static void DrawFloatControl(const Gleam::TString& label, float& value, float resetValue = 0.0f, float columnWidth = 100.0f)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text("%s", label.c_str());
	ImGui::NextColumn();

	ImGui::PushItemWidth(ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushFont(boldFont);

	if (ImGui::Button("X", buttonSize))
	{
		value = resetValue;
	}

	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##X", &value, 0.05f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

static void DrawVec3Control(const Gleam::TString& label, Gleam::Float3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
{
    ImGuiIO& io = ImGui::GetIO();
    auto boldFont = io.Fonts->Fonts[0];

    ImGui::PushID(label.c_str());

    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label.c_str());
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

static void DrawClassFields(void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth = 100.0f)
{
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		DrawClassFields(obj, baseClass);
	}

	for (const auto& field : classDesc.ResolveFields())
	{
		switch (field.GetType())
		{
			case Gleam::Reflection::FieldType::Class:
			{
				const auto& classField = field.GetField<Gleam::Reflection::ClassField>();
				const auto& fieldDesc = Gleam::Reflection::GetClass(classField.hash);
				DrawClassFields(Gleam::OffsetPointer(obj, classField.offset), fieldDesc, columnWidth);
				break;
			}
			case Gleam::Reflection::FieldType::Array:
			{

				break;
			}
			case Gleam::Reflection::FieldType::Enum:
			{

				break;
			}
			case Gleam::Reflection::FieldType::Primitive:
			{
				const auto& primitiveField = field.GetField<Gleam::Reflection::PrimitiveField>();
				constexpr uint64_t defaultValue = 0;
				DrawScalarControl(field.ResolveName(), primitiveField, Gleam::OffsetPointer(obj, primitiveField.offset), defaultValue, columnWidth);
				break;
			}
			default:
				continue;
		}
	}
}

using ComponentUIFunction = std::function<void()>;
static void DrawComponent(const Gleam::TStringView label, void* component, const Gleam::Reflection::ClassDescription& classDesc, ComponentUIFunction&& uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });

    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImGui::Separator();

	size_t hash = Gleam::Reflection::Database::GetTypeHash(classDesc.ResolveName());
    bool open = ImGui::TreeNodeEx((void*)hash, treeNodeFlags, "%s", label.data());
    ImGui::PopStyleVar();

    if (open)
    {
        uiFunction();
        ImGui::TreePop();
    }
}

void EntityInspector::Init(Gleam::World* world)
{
	mEditWorld = world;
    Gleam::EventDispatcher<EntitySelectedEvent>::Subscribe([this](EntitySelectedEvent e)
    {
        mSelectedEntity = e.GetEntity();
    });
}

void EntityInspector::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("Entity Inspector")) return;
        
        if (mSelectedEntity == Gleam::InvalidEntity)
        {
            ImGui::End();
            return;
        }
        
        auto& entityManager = mEditWorld->GetEntityManager();
		auto& entity = entityManager.GetComponent<Gleam::Entity>(mSelectedEntity);
		DrawComponent("Local Transform", &entity, Gleam::Reflection::ClassDescription(), [&]()
		{
			auto panelWidth = ImGui::GetContentRegionAvail().x;
			auto labelWidth = panelWidth * 0.3f;

			auto localPosition = entity.GetLocalPosition();
			DrawVec3Control("Translation", localPosition, 0.0f, labelWidth);
			entity.SetTranslation(localPosition);

			auto localRotation = Gleam::Math::Rad2Deg(entity.GetLocalRotation().EulerAngles());
			DrawVec3Control("Rotation", localRotation, 0.0f, labelWidth);
			entity.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(localRotation)));

			auto localScale = entity.GetLocalScale();
			DrawFloatControl("Scale", localScale, 1.0f, labelWidth);
			entity.SetScale(localScale);
		});

		entityManager.Visit(mSelectedEntity, [](void* component, const Gleam::Reflection::ClassDescription& classDesc)
		{
			if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>())
			{
				DrawComponent(classDesc.ResolveName(), component, classDesc, [&]()
				{
					auto panelWidth = ImGui::GetContentRegionAvail().x;
					auto labelWidth = panelWidth * 0.3f;

					DrawClassFields(component, classDesc, labelWidth);
				});
				
			}
		});
        
		ImGui::End();
	});
}
