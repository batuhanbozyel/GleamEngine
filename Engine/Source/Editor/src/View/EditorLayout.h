#pragma once
#include <imgui.h>
#include <imgui_internal.h>

namespace GEditor {

inline void EditorLayout(ImGuiID dockspaceID)
{
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);

	ImGuiID center = dockspaceID;
	const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.17f, nullptr, &center);
	const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.2f, nullptr, &center);
	const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.3f, nullptr, &center);

	ImGui::DockBuilderDockWindow("World Outliner", left);
	ImGui::DockBuilderDockWindow("Entity Inspector", right);
	ImGui::DockBuilderDockWindow("Project Settings", right);
	ImGui::DockBuilderDockWindow("Content Browser", bottom);
	ImGui::DockBuilderDockWindow("Viewport", center);
	ImGui::DockBuilderFinish(dockspaceID);
}

} // namespace GEditor
