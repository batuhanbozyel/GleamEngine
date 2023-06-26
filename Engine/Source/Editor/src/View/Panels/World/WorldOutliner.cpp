//
//  WorldOutliner.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "WorldOutliner.h"
#include "WorldViewport.h"
#include "WorldOutlineController.h"

#include "View/ViewStack.h"

#include <imgui.h>

using namespace GEditor;

WorldOutliner::WorldOutliner()
{
    mController = Gleam::World::active->AddSystem<WorldOutlineController>();
}

void WorldOutliner::Render()
{
    if (!ImGui::Begin("World Outliner")) return;
    
    bool entityDeleted = false;
    auto& entityManager = Gleam::World::active->GetEntityManager();
    entityManager.ForEach([&](Gleam::Entity entity)
    {
        ImGuiTreeNodeFlags flags = ((mSelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        
        uint32_t id = static_cast<uint32_t>(entity);
        auto idString = std::to_string(id);
        
        if (!ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", idString.c_str())) { return; }
        
        if (ImGui::IsItemClicked())
        {
            mSelectedEntity = entity;
        }
        
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Destroy Entity"))
            {
                entityDeleted = true;
            }
            ImGui::EndPopup();
        }
        
        ImGui::TreePop();
    });
    
    if (mSelectedEntity == Gleam::Entity{})
    {
        if (ImGui::BeginPopupContextWindow("Create Entity"))
        {
            if (ImGui::MenuItem("Import Model"))
            {
                mController->ImportAsset(Gleam::FileType::Model);
            }
            
            ImGui::EndPopup();
        }
    }
    else if (entityDeleted)
    {
        entityManager.DestroyEntity(mSelectedEntity);
    }
    
    ImGui::End();
}
