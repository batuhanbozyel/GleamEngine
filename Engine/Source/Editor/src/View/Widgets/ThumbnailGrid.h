#pragma once
#include "Container/Array.h"
#include "Container/String.h"
#include "Math/Color.h"

namespace GEditor {

struct ThumbnailItem
{
	Gleam::TStringView label;
	const char* iconText = nullptr;
	Gleam::Color color = Gleam::Color(0.5f, 0.5f, 0.5f, 1.0f);
	bool filled = false;
	const char* payloadType = nullptr;
	const void* payload = nullptr;
	size_t payloadSize = 0;
	Gleam::TStringView tooltip;
	bool dimmed = false;
};

struct ThumbnailGridEvents
{
	int32_t doubleClicked = -1;
	int32_t contextMenu = -1;
};

class ThumbnailGrid
{
public:

	static ThumbnailGridEvents Draw(Gleam::TArrayView<const ThumbnailItem> items, float iconSize = 80.0f, float padding = 10.0f);

private:

	struct ItemEvents
	{
		bool doubleClicked = false;
		bool contextMenu = false;
	};

	static ItemEvents DrawItem(const ThumbnailItem& item, uint32_t index, float iconSize);

};

} // namespace GEditor
