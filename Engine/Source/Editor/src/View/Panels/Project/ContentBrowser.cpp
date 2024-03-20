//
//  ContentBrowser.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 14.11.2023.
//

#include "ContentBrowser.h"
#include "Gleam.h"

#include "Assets/AssetImporter.h"
#include "Assets/MeshSource.h"

#include <imgui.h>

using namespace GEditor;

ContentBrowser::ContentBrowser()
	: mAssetDirectory(GameInstance->GetDefaultAssetPath())
{

}

void ContentBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this]()
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
                    auto model = MeshSource();
                    auto settings = MeshSource::ImportSettings();
					bool success = model.Import(path, settings);
                    // TODO: Create mesh asset and write to current directory
				}
			}
		}

		ImGui::End();
	});
}
