#pragma once
#include "Container/Array.h"
#include "Container/String.h"

#include <imgui.h>

namespace GEditor {

struct EditorFonts;

struct ThumbnailItem
{
	Gleam::TStringView label;
	const char* typeText = nullptr;
	ImVec4 color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	bool isFolder = false;
	bool selected = false;
	const char* payloadType = nullptr;
	const void* payload = nullptr;
	size_t payloadSize = 0;
};

struct ThumbnailGridEvents
{
	int32_t clicked = -1;
	int32_t doubleClicked = -1;
};

class ThumbnailGrid
{
public:

	static ThumbnailGridEvents Draw(Gleam::TArrayView<const ThumbnailItem> items, const EditorFonts& fonts);

private:

	static void DrawItem(const ThumbnailItem& item, uint32_t index, const EditorFonts& fonts, ThumbnailGridEvents& events);

};

} // namespace GEditor
