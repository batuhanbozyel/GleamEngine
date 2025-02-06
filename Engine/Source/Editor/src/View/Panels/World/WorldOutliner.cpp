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

void WorldOutliner::Init(Gleam::World* world)
{
	mEditWorld = world;
}

void WorldOutliner::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		if (!ImGui::Begin("World Outliner")) return;
    
		auto& entityManager = mEditWorld->GetEntityManager();
		entityManager.ForEach([&](Gleam::EntityHandle handle)
		{
			uint32_t id = static_cast<uint32_t>(handle);
			const auto& entity = entityManager.GetComponent<Gleam::Entity>(handle);
			
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
            if (handle == mSelectedEntity)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            
            ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
			if (ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", entity.GetName().c_str()))
            {
                if (ImGui::IsItemClicked())
                {
                    Gleam::EventDispatcher<EntitySelectedEvent>::Publish(EntitySelectedEvent(handle));
                    mSelectedEntity = handle;
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
