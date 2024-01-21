//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "Gleam.h"

#include "Assets/AssetImporter.h"
#include "Assets/Model.h"

#include <imgui.h>

using namespace GEditor;

ContentBrowser::ContentBrowser()
	: mAssetDirectory(GameInstance->GetDefaultAssetPath())
{

}

void ContentBrowser::Render()
{
    ImGui::Begin("Content Browser");

	if (ImGui::Button("Add"))
    {
        
    }

    ImGui::SameLine();

    if (ImGui::Button("Import"))
    {
		auto& entityManager = Gleam::World::active->GetEntityManager();
        auto files = Gleam::IOUtils::OpenFileDialog();
		for (const auto& path : files)
		{
			if (path.extension() == ".gltf")
			{
				auto model = AssetImporter<Model>::Import(path);
				auto mesh = Gleam::CreateRef<Gleam::Mesh>(model.GetMeshes());

				auto entity = entityManager.CreateEntity();
				entityManager.AddComponent<Gleam::MeshRenderer>(entity, mesh);
			}
		}
    }

	ImGui::End();
}
