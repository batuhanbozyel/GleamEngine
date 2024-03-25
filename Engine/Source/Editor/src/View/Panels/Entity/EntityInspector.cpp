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

static void DrawVec3Control(const std::string& label, Gleam::Float3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
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
        values.x = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Y", buttonSize))
        values.y = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
    ImGui::PushFont(boldFont);
    if (ImGui::Button("Z", buttonSize))
        values.z = resetValue;
    ImGui::PopFont();
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
    ImGui::PopItemWidth();

    ImGui::PopStyleVar();
    ImGui::Columns(1);

    ImGui::PopID();
}

template<typename T, typename UIFunction>
static void DrawComponent(const std::string& name, T& component, UIFunction uiFunction)
{
    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
    float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
    ImGui::Separator();
    bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
    ImGui::PopStyleVar();

    if (open)
    {
        uiFunction(component);
        ImGui::TreePop();
    }
}

EntityInspector::EntityInspector()
{
    Gleam::EventDispatcher<EntitySelectedEvent>::Subscribe([this](EntitySelectedEvent e)
    {
        mSelectedEntity = e.GetEntity();
    });
}

void EntityInspector::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this]()
	{
		if (!ImGui::Begin("Entity Inspector")) return;
        
        if (mSelectedEntity == Gleam::InvalidEntity)
        {
            ImGui::End();
            return;
        }
        
        auto& entityManager = Gleam::World::active->GetEntityManager();
        if (entityManager.HasComponent<Gleam::Camera>(mSelectedEntity))
        {
            auto& component = entityManager.GetComponent<Gleam::Camera>(mSelectedEntity);
            DrawComponent<Gleam::Camera>("Local Transform", component, [](auto& component)
            {
                auto localPosition = component.GetLocalPosition();
                DrawVec3Control("Translation", localPosition);
                component.SetTranslation(localPosition);
                
                auto localRotation = Gleam::Math::Rad2Deg(component.GetLocalRotation().EulerAngles());
                DrawVec3Control("Rotation", localRotation);
                component.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(localRotation)));
                
                auto localScale = component.GetLocalScale();
                DrawVec3Control("Scale", localScale, 1.0f);
                component.SetScale(localScale);
            });
        }
        else
        {
            auto& component = entityManager.GetComponent<Gleam::Transform>(mSelectedEntity);
            DrawComponent<Gleam::Transform>("Local Transform", component, [](auto& component)
            {
                auto localPosition = component.GetLocalPosition();
                DrawVec3Control("Translation", localPosition);
                component.SetTranslation(localPosition);
                
                auto localRotation = Gleam::Math::Rad2Deg(component.GetLocalRotation().EulerAngles());
                DrawVec3Control("Rotation", localRotation);
                component.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(localRotation)));
                
                auto localScale = component.GetLocalScale();
                DrawVec3Control("Scale", localScale, 1.0f);
                component.SetScale(localScale);
            });
        }
        
		ImGui::End();
	});
}
