#pragma once
#include "View/View.h"
#include "IO/Log.h"
#include "Container/Array.h"
#include "Container/String.h"

#include <imgui.h>

namespace GEditor {

struct EditorFonts;

class Console final : public View
{
public:

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Update() override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	enum Category : uint32_t
	{
		Info,
		Warning,
		Error,
		Count
	};

	struct Entry
	{
		Gleam::Logger::Level level;
		Gleam::TString time;
		Gleam::TString source;
		Gleam::TString message;
	};

	void AddEntry(const Gleam::Logger::Entry& entry);

	void TrimEntries();

	void Clear();

	void RebuildFilter();

	bool PassesFilter(const Entry& entry) const;

	void DrawToolbar();

	void DrawFilterToggle(const char* id, const char* icon, const ImVec4& color, Category category);

	void DrawEntries(float height);

	void DrawEntry(uint32_t index, float width);

	void DrawDetails();

	void CopyEntry(const Entry& entry) const;

	static Category GetCategory(Gleam::Logger::Level level);

	const EditorFonts* mFonts = nullptr;

	Gleam::TArray<Entry> mEntries;

	Gleam::TArray<uint32_t> mFilteredEntries;

	uint64_t mHistoryCursor = 0;

	uint32_t mCounts[Category::Count] = {};

	bool mShowCategory[Category::Count] = { true, true, true };

	int32_t mSelectedEntry = -1;

	size_t mRenderedEntryCount = 0;

	bool mFilterDirty = true;

	char mSearch[128] = "";

};

} // namespace GEditor
