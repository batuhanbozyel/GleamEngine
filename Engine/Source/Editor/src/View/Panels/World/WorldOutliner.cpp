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
	imgui->PushView([this]()
	{
		if (!ImGui::Begin("World Outliner")) return;
    
		auto& entityManager = Gleam::World::active->GetEntityManager();
		entityManager.ForEach([&](Gleam::EntityHandle entity)
		{
			ImGuiTreeNodeFlags flags = ((mSelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			
			uint32_t id = static_cast<uint32_t>(entity);
			Gleam::TStringStream ss;
			ss << "Entity " << id;
			
			if (!ImGui::TreeNodeEx((void*)(uint64_t)id, flags, "%s", ss.str().c_str())) { return; }
			
			if (ImGui::IsItemClicked())
			{
				Gleam::EventDispatcher<EntitySelectedEvent>::Publish(EntitySelectedEvent(entity));
				mSelectedEntity = entity;
			}
			
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Destroy Entity"))
				{
					// TODO: 
				}
				ImGui::EndPopup();
			}
			
			ImGui::TreePop();
		});
		
		ImGui::End();
	});
}
