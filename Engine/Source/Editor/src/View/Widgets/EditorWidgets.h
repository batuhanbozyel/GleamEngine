#pragma once
#include "Container/String.h"
#include "World/Entity.h"

#include <imgui.h>

namespace Gleam {
class EntityManager;
} // namespace Gleam

namespace GEditor {

struct EditorFonts;

struct EntityKind
{
	Gleam::TStringView name;
	ImVec4 color;
};

namespace Widgets {

ImU32 ColorU32(const ImVec4& color, float alpha = 1.0f);

void DrawIcon(ImDrawList* drawList, const char* icon, const ImVec2& center, const ImVec4& color);

void DrawDashedRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 color, float dash = 4.0f, float gap = 3.0f);

bool SearchField(const char* id, const char* hint, char* buffer, size_t bufferSize, float width, float height = 30.0f);

void CaptionRow(const EditorFonts& fonts, const char* label, uint32_t count, float height, float paddingX);

bool OverlayButton(const char* id, const char* icon, const ImVec2& size, bool active = false);

EntityKind GetEntityKind(const Gleam::EntityManager& entityManager, Gleam::EntityHandle handle);

} // namespace Widgets

} // namespace GEditor
