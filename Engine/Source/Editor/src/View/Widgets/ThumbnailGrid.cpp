#include "ThumbnailGrid.h"

#include <imgui.h>

using namespace GEditor;

ThumbnailGridEvents ThumbnailGrid::Draw(Gleam::TArrayView<const ThumbnailItem> items, float iconSize, float padding)
{
	ThumbnailGridEvents events;

	float cellSize = iconSize + padding;
	float panelWidth = ImGui::GetContentRegionAvail().x;
	uint32_t columnCount = Gleam::Math::Max((uint32_t)(panelWidth / cellSize), 1u);
	uint32_t itemCount = static_cast<uint32_t>(items.size());
	uint32_t rowCount = (itemCount + columnCount - 1u) / columnCount;

	float labelHeight = ImGui::GetTextLineHeight() * 2.0f;
	float rowHeight = iconSize + labelHeight + ImGui::GetStyle().ItemSpacing.y * 2.0f;

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rowCount), rowHeight);
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			for (uint32_t column = 0u; column < columnCount; ++column)
			{
				uint32_t index = static_cast<uint32_t>(row) * columnCount + column;
				if (index >= itemCount)
				{
					break;
				}

				if (column > 0u)
				{
					ImGui::SameLine();
				}

				const auto itemEvents = DrawItem(items[index], index, iconSize);
				if (itemEvents.doubleClicked)
				{
					events.doubleClicked = static_cast<int32_t>(index);
				}
				if (itemEvents.contextMenu)
				{
					events.contextMenu = static_cast<int32_t>(index);
				}
			}
		}
	}
	clipper.End();

	return events;
}

ThumbnailGrid::ItemEvents ThumbnailGrid::DrawItem(const ThumbnailItem& item, uint32_t index, float iconSize)
{
	ImVec4 itemColor = ImVec4(item.color.r, item.color.g, item.color.b, item.color.a);

	ImGui::PushID(static_cast<int>(index));

	if (item.dimmed)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.4f);
	}
	ImGui::BeginGroup();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

	if (item.filled)
	{
		ImVec4 hoverColor = ImVec4(itemColor.x * 1.1f, itemColor.y * 1.1f, itemColor.z * 1.1f, 1.0f);
		ImVec4 activeColor = ImVec4(itemColor.x * 1.3f, itemColor.y * 1.3f, itemColor.z * 1.3f, 1.0f);

		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##filled", ImVec2(iconSize, iconSize));

		ImVec4 currentColor = itemColor;
		if (ImGui::IsItemActive())
		{
			currentColor = activeColor;
		}
		else if (ImGui::IsItemHovered())
		{
			currentColor = hoverColor;
		}

		ImGui::GetWindowDrawList()->AddRectFilled(
			cursorPos,
			ImVec2(cursorPos.x + iconSize, cursorPos.y + iconSize),
			ImGui::ColorConvertFloat4ToU32(currentColor)
		);
	}
	else
	{
		ImGui::Button(item.iconText, ImVec2(iconSize, iconSize));
	}

	ImGui::PopStyleColor(3);

	ItemEvents events;
	events.doubleClicked = ImGui::IsItemHovered() and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
	events.contextMenu = ImGui::IsItemClicked(ImGuiMouseButton_Right);
	if (item.tooltip.empty() == false and ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%.*s", static_cast<int>(item.tooltip.size()), item.tooltip.data());
	}

	if (item.payloadType and ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		ImGui::SetDragDropPayload(item.payloadType, item.payload, item.payloadSize);
		ImGui::Text("%.*s", static_cast<int>(item.label.size()), item.label.data());
		ImGui::EndDragDropSource();
	}

	if (not item.filled)
	{
		ImVec2 separatorStart = ImGui::GetCursorScreenPos();
		ImVec2 separatorEnd = ImVec2(separatorStart.x + iconSize, separatorStart.y);
		ImGui::GetWindowDrawList()->AddLine(separatorStart, separatorEnd, ImGui::ColorConvertFloat4ToU32(itemColor), 3.0f);
	}
	ImGui::Spacing();

	// Clip the label to a fixed two-line box so every cell is the same height,
	// keeping the list clipper's row estimate exact
	float labelHeight = ImGui::GetTextLineHeight() * 2.0f;
	ImVec2 labelPos = ImGui::GetCursorScreenPos();

	ImGui::PushClipRect(labelPos, ImVec2(labelPos.x + iconSize, labelPos.y + labelHeight), true);
	ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + iconSize);
	ImGui::TextWrapped("%.*s", static_cast<int>(item.label.size()), item.label.data());
	ImGui::PopTextWrapPos();
	ImGui::PopClipRect();

	ImGui::SetCursorScreenPos(labelPos);
	ImGui::Dummy(ImVec2(iconSize, labelHeight));

	ImGui::EndGroup();
	if (item.dimmed)
	{
		ImGui::PopStyleVar();
	}

	ImGui::PopID();

	return events;
}
