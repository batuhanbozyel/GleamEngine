#include "ThumbnailGrid.h"
#include "EditorWidgets.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"

#include <imgui_internal.h>

#include <cstring>

using namespace GEditor;

static constexpr ImVec2 kCellSize(84.0f, 96.0f);
static constexpr float kColumnGap = 12.0f;
static constexpr float kRowGap = 10.0f;
static constexpr float kTileSize = 60.0f;

ThumbnailGridEvents ThumbnailGrid::Draw(Gleam::TArrayView<const ThumbnailItem> items, const EditorFonts& fonts)
{
	ThumbnailGridEvents events;

	const float panelWidth = ImGui::GetContentRegionAvail().x;
	const uint32_t columnCount = Gleam::Math::Max(static_cast<uint32_t>((panelWidth + kColumnGap) / (kCellSize.x + kColumnGap)), 1u);
	const uint32_t itemCount = static_cast<uint32_t>(items.size());
	const uint32_t rowCount = (itemCount + columnCount - 1u) / columnCount;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(kColumnGap, kRowGap));

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rowCount), kCellSize.y + kRowGap);
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			for (uint32_t column = 0u; column < columnCount; ++column)
			{
				const uint32_t index = static_cast<uint32_t>(row) * columnCount + column;
				if (index >= itemCount)
				{
					break;
				}

				if (column > 0u)
				{
					ImGui::SameLine();
				}
				DrawItem(items[index], index, fonts, events);
			}
		}
	}
	clipper.End();

	ImGui::PopStyleVar();
	return events;
}

void ThumbnailGrid::DrawItem(const ThumbnailItem& item, uint32_t index, const EditorFonts& fonts, ThumbnailGridEvents& events)
{
	ImGui::PushID(static_cast<int>(index));

	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + kCellSize.x, min.y + kCellSize.y);
	ImGui::InvisibleButton("##Item", kCellSize);
	const bool hovered = ImGui::IsItemHovered();

	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		events.clicked = static_cast<int32_t>(index);
	}
	if (hovered and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		events.doubleClicked = static_cast<int32_t>(index);
	}

	if (item.payloadType and ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
	{
		ImGui::SetDragDropPayload(item.payloadType, item.payload, item.payloadSize);
		ImGui::Text("%.*s", static_cast<int>(item.label.size()), item.label.data());
		ImGui::EndDragDropSource();
	}

	auto drawList = ImGui::GetWindowDrawList();
	if (item.selected)
	{
		drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Selected), 6.0f);
		drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::Accent), 6.0f);
	}
	else if (hovered)
	{
		drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::SectionHeader), 6.0f);
	}

	const ImVec2 tileMin(min.x + (kCellSize.x - kTileSize) * 0.5f, min.y + 5.0f);
	const ImVec2 tileMax(tileMin.x + kTileSize, tileMin.y + kTileSize);
	const ImVec2 tileCenter((tileMin.x + tileMax.x) * 0.5f, (tileMin.y + tileMax.y) * 0.5f);
	drawList->AddRectFilled(tileMin, tileMax, Widgets::ColorU32(Gleam::Theme::Control), 4.0f);

	if (item.isFolder)
	{
		ImGui::PushFont(fonts.iconLarge);
		Widgets::DrawIcon(drawList, ICON_LC_FOLDER, tileCenter, Gleam::Theme::Accent);
		ImGui::PopFont();
	}
	else
	{
		drawList->AddRectFilled(ImVec2(tileMin.x, tileMax.y - 3.0f), tileMax, Widgets::ColorU32(item.color), 4.0f, ImDrawFlags_RoundCornersBottom);
		if (item.typeText)
		{
			ImGui::PushFont(fonts.micro);
			const float lineHeight = ImGui::GetFontSize();
			const ImVec2 textSize = ImGui::CalcTextSize(item.typeText);
			float lineY = tileCenter.y - textSize.y * 0.5f;
			for (const char* line = item.typeText; line != nullptr and *line != '\0';)
			{
				const char* lineEnd = std::strchr(line, '\n');
				const char* end = lineEnd ? lineEnd : line + std::strlen(line);
				const float lineWidth = ImGui::CalcTextSize(line, end).x;
				drawList->AddText(ImVec2(IM_ROUND(tileCenter.x - lineWidth * 0.5f), IM_ROUND(lineY)), Widgets::ColorU32(Gleam::Theme::TextMuted), line, end);
				lineY += lineHeight;
				line = lineEnd ? lineEnd + 1 : nullptr;
			}
			ImGui::PopFont();
		}
	}

	ImGui::PushFont(fonts.caption);
	ImGui::PushStyleColor(ImGuiCol_Text, Gleam::Theme::TextBadge);
	const char* labelBegin = item.label.data();
	const char* labelEnd = labelBegin + item.label.size();
	const ImVec2 labelSize = ImGui::CalcTextSize(labelBegin, labelEnd);
	const ImVec2 labelMin(min.x + 4.0f, tileMax.y + 6.0f);
	const ImVec2 labelMax(max.x - 4.0f, labelMin.y + ImGui::GetFontSize());
	if (labelSize.x <= labelMax.x - labelMin.x)
	{
		drawList->AddText(ImVec2(min.x + (kCellSize.x - labelSize.x) * 0.5f, labelMin.y), ImGui::GetColorU32(ImGuiCol_Text), labelBegin, labelEnd);
	}
	else
	{
		ImGui::RenderTextEllipsis(drawList, labelMin, labelMax, labelMax.x, labelBegin, labelEnd, &labelSize);
	}
	ImGui::PopStyleColor();
	ImGui::PopFont();

	if (hovered and labelSize.x > labelMax.x - labelMin.x)
	{
		ImGui::SetTooltip("%.*s", static_cast<int>(item.label.size()), labelBegin);
	}

	ImGui::PopID();
}
