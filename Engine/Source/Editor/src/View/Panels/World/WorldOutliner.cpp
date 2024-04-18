//
//  WorldOutliner.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#include "WorldOutliner.h"
#include "WorldViewport.h"

#include <imgui.h>

using namespace GEditor;

WorldOutliner::WorldOutliner()
{
    
}

void WorldOutliner::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("World Outliner")) return;
    
		auto& entityManager = Gleam::World::active->GetEntityManager();
		entityManager.ForEach([&](Gleam::EntityHandle entity)
		{
			uint32_t id = static_cast<uint32_t>(entity);
			Gleam::TStringStream ss;
			ss << "Entity " << id;
			
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
            if (entity == mSelectedEntity)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            
            ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
			if (ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", ss.str().c_str()))
            {
                if (ImGui::IsItemClicked())
                {
                    Gleam::EventDispatcher<EntitySelectedEvent>::Publish(EntitySelectedEvent(entity));
                    mSelectedEntity = entity;
                }
                DrawEntityPopupMenu();
                
                ImGui::TreePop();
            }
            ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		});
		
		ImGui::End();
	});
}

void WorldOutliner::DrawEntityPopupMenu()
{
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Destroy Entity"))
        {
            // TODO:
        }
        ImGui::EndPopup();
    }
}
