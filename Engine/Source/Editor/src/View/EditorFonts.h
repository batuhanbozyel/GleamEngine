#pragma once

struct ImFont;

namespace GEditor {

struct EditorFonts
{
	ImFont* regular = nullptr;
	ImFont* compact = nullptr;
	ImFont* medium = nullptr;
	ImFont* semiBold = nullptr;
	ImFont* label = nullptr;
	ImFont* brand = nullptr;
	ImFont* heading = nullptr;
	ImFont* title = nullptr;
	ImFont* caption = nullptr;
	ImFont* micro = nullptr;
	ImFont* microBold = nullptr;
	ImFont* footnote = nullptr;
	ImFont* subheading = nullptr;
	ImFont* numeric = nullptr;
	ImFont* mono = nullptr;
	ImFont* detail = nullptr;
	ImFont* iconLarge = nullptr;
};

} // namespace GEditor
