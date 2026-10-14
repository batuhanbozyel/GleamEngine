#include "Console.h"
#include "View/ViewStack.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "View/Widgets/EditorWidgets.h"

#include "Core/Application.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <imgui_internal.h>

#include <cstdio>
#include <cstring>
#include <ctime>
#include <iterator>

using namespace GEditor;

static constexpr uint32_t kMaxEntries = 4096;
static constexpr uint32_t kTrimCount = 1024;
static constexpr float kPadding = 10.0f;
static constexpr float kToolbarHeight = 44.0f;
static constexpr float kButtonHeight = 28.0f;
static constexpr float kSearchWidth = 220.0f;
static constexpr float kRowHeight = 24.0f;
static constexpr float kDetailsHeight = 140.0f;

static const char* GetLevelIcon(Gleam::Logger::Level level)
{
	switch (level)
	{
		case Gleam::Logger::Level::Warn: return ICON_LC_TRIANGLE_ALERT;
		case Gleam::Logger::Level::Error: return ICON_LC_CIRCLE_X;
		default: return ICON_LC_INFO;
	}
}

static const ImVec4& GetLevelColor(Gleam::Logger::Level level)
{
	switch (level)
	{
		case Gleam::Logger::Level::Trace: return Gleam::Theme::TextDim;
		case Gleam::Logger::Level::Warn: return Gleam::Theme::Warning;
		case Gleam::Logger::Level::Error: return Gleam::Theme::Error;
		default: return Gleam::Theme::Info;
	}
}

void Console::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void Console::Update()
{
	Gleam::Logger::ReadHistory(mHistoryCursor, [this](const Gleam::Logger::Entry& entry)
	{
		AddEntry(entry);
	});

	if (mEntries.size() > kMaxEntries)
	{
		TrimEntries();
	}
}

void Console::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Gleam::Theme::Panel);
		const bool visible = ImGui::Begin("Console");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if (visible)
		{
			DrawToolbar();
			if (mFilterDirty)
			{
				RebuildFilter();
			}

			if (mSelectedEntry >= 0)
			{
				DrawEntries(ImMax(ImGui::GetContentRegionAvail().y - kDetailsHeight, kRowHeight));
				DrawDetails();
			}
			else
			{
				DrawEntries(0.0f);
			}
		}
		ImGui::End();
	});
}

void Console::AddEntry(const Gleam::Logger::Entry& entry)
{
	char time[16] = {};
	std::strftime(time, sizeof(time), "%H:%M:%S", std::localtime(&entry.time));

	const auto& added = mEntries.emplace_back(Entry{
		.level = entry.level,
		.time = time,
		.source = Gleam::TString(entry.source.data(), entry.source.size()),
		.message = Gleam::TString(entry.message.data(), entry.message.size())
	});
	++mCounts[GetCategory(added.level)];

	if (PassesFilter(added))
	{
		mFilteredEntries.push_back(static_cast<uint32_t>(mEntries.size() - 1));
	}
}

void Console::TrimEntries()
{
	const auto removeCount = static_cast<int32_t>(mEntries.size() - kMaxEntries + kTrimCount);
	mEntries.erase(mEntries.begin(), mEntries.begin() + removeCount);
	mSelectedEntry = mSelectedEntry >= removeCount ? mSelectedEntry - removeCount : -1;

	for (auto& count : mCounts)
	{
		count = 0;
	}
	for (const auto& entry : mEntries)
	{
		++mCounts[GetCategory(entry.level)];
	}
	mFilterDirty = true;
}

void Console::Clear()
{
	mEntries.clear();
	mFilteredEntries.clear();
	for (auto& count : mCounts)
	{
		count = 0;
	}
	mSelectedEntry = -1;
	mRenderedEntryCount = 0;
}

void Console::RebuildFilter()
{
	mFilteredEntries.clear();
	for (uint32_t i = 0; i < mEntries.size(); ++i)
	{
		if (PassesFilter(mEntries[i]))
		{
			mFilteredEntries.push_back(i);
		}
	}
	mRenderedEntryCount = mFilteredEntries.size();
	mFilterDirty = false;
}

bool Console::PassesFilter(const Entry& entry) const
{
	const char* searchEnd = mSearch + std::strlen(mSearch);
	const bool matchesSearch = mSearch[0] == '\0'
		or ImStristr(entry.message.c_str(), entry.message.c_str() + entry.message.size(), mSearch, searchEnd) != nullptr
		or ImStristr(entry.source.c_str(), entry.source.c_str() + entry.source.size(), mSearch, searchEnd) != nullptr;
	return mShowCategory[GetCategory(entry.level)] and matchesSearch;
}

void Console::DrawToolbar()
{
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float startY = ImGui::GetCursorPosY();

	ImGui::SetCursorPos(ImVec2(kPadding, startY + (kToolbarHeight - kButtonHeight) * 0.5f));
	if (Widgets::OverlayButton("##ConsoleClear", ICON_LC_TRASH_2, ImVec2(kButtonHeight, kButtonHeight)))
	{
		Clear();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Clear");
	}

	ImGui::SameLine(0.0f, 8.0f);
	DrawFilterToggle("##ConsoleInfo", ICON_LC_INFO, Gleam::Theme::Info, Category::Info);
	ImGui::SameLine(0.0f, 4.0f);
	DrawFilterToggle("##ConsoleWarning", ICON_LC_TRIANGLE_ALERT, Gleam::Theme::Warning, Category::Warning);
	ImGui::SameLine(0.0f, 4.0f);
	DrawFilterToggle("##ConsoleError", ICON_LC_CIRCLE_X, Gleam::Theme::Error, Category::Error);

	ImGui::SameLine(0.0f, 8.0f);
	const float searchX = ImMax(ImGui::GetCursorPosX(), width - kPadding - kSearchWidth);
	ImGui::SetCursorPosX(searchX);
	if (Widgets::SearchField("##ConsoleSearch", "Search", mSearch, sizeof(mSearch), width - kPadding - searchX, kButtonHeight))
	{
		mFilterDirty = true;
	}

	const float dividerY = windowPos.y + startY + kToolbarHeight - ImGui::GetScrollY();
	ImGui::GetWindowDrawList()->AddLine(ImVec2(windowPos.x, dividerY), ImVec2(windowPos.x + ImGui::GetWindowWidth(), dividerY), Widgets::ColorU32(Gleam::Theme::Divider));
	ImGui::SetCursorPos(ImVec2(0.0f, startY + kToolbarHeight + 1.0f));
}

void Console::DrawFilterToggle(const char* id, const char* icon, const ImVec4& color, Category category)
{
	char countText[16] = {};
	std::snprintf(countText, sizeof(countText), "%u", mCounts[category]);

	ImGui::PushFont(mFonts->mono);
	const ImVec2 countSize = ImGui::CalcTextSize(countText);
	ImGui::PopFont();

	const ImVec2 size(kButtonHeight + countSize.x + 6.0f, kButtonHeight);
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + size.x, min.y + size.y);
	bool& enabled = mShowCategory[category];
	if (ImGui::InvisibleButton(id, size))
	{
		enabled = not enabled;
		mFilterDirty = true;
	}
	const bool hovered = ImGui::IsItemHovered();

	auto drawList = ImGui::GetWindowDrawList();
	const auto& background = enabled ? Gleam::Theme::Control : Gleam::Theme::Background;
	drawList->AddRectFilled(min, max, Widgets::ColorU32(hovered ? Gleam::Theme::ControlHover : background), 6.0f);
	drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::BorderStrong), 6.0f);

	const float centerY = (min.y + max.y) * 0.5f;
	Widgets::DrawIcon(drawList, icon, ImVec2(min.x + kButtonHeight * 0.5f, centerY), enabled ? color : Gleam::Theme::TextDim);

	ImGui::PushFont(mFonts->mono);
	drawList->AddText(ImVec2(min.x + kButtonHeight - 2.0f, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(enabled ? Gleam::Theme::Text : Gleam::Theme::TextDim), countText);
	ImGui::PopFont();
}

void Console::DrawEntries(float height)
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("ConsoleEntries", ImVec2(0.0f, height), ImGuiChildFlags_None);
	{
		const bool atBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
		const float width = ImGui::GetContentRegionAvail().x;

		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(mFilteredEntries.size()), kRowHeight);
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
			{
				DrawEntry(mFilteredEntries[row], width);
			}
		}
		clipper.End();

		if (atBottom and mFilteredEntries.size() > mRenderedEntryCount)
		{
			ImGui::SetScrollHereY(1.0f);
		}
		mRenderedEntryCount = mFilteredEntries.size();

		if (mSelectedEntry >= 0 and ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_C))
		{
			CopyEntry(mEntries[mSelectedEntry]);
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void Console::DrawEntry(uint32_t index, float width)
{
	const auto& entry = mEntries[index];

	ImGui::PushID(static_cast<int>(index));
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + width, min.y + kRowHeight);
	if (ImGui::InvisibleButton("##Entry", ImVec2(width, kRowHeight)))
	{
		mSelectedEntry = static_cast<int32_t>(index);
	}
	const bool hovered = ImGui::IsItemHovered();

	if (ImGui::BeginPopupContextItem("##EntryContext"))
	{
		if (ImGui::MenuItem("Copy"))
		{
			CopyEntry(entry);
		}
		ImGui::EndPopup();
	}

	auto drawList = ImGui::GetWindowDrawList();
	if (mSelectedEntry == static_cast<int32_t>(index))
	{
		drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Selected));
	}
	else if (hovered)
	{
		drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Selected, 0.5f));
	}
	drawList->AddLine(ImVec2(min.x, max.y - 1.0f), ImVec2(max.x, max.y - 1.0f), Widgets::ColorU32(Gleam::Theme::RowDivider));

	const float centerY = (min.y + max.y) * 0.5f;
	Widgets::DrawIcon(drawList, GetLevelIcon(entry.level), ImVec2(min.x + 18.0f, centerY), GetLevelColor(entry.level));

	drawList->PushClipRect(min, max, true);
	float x = min.x + 34.0f;

	ImGui::PushFont(mFonts->mono);
	drawList->AddText(ImVec2(x, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::TextDim), entry.time.c_str());
	x += ImGui::CalcTextSize("00:00:00").x + 12.0f;
	ImGui::PopFont();

	drawList->AddText(ImVec2(x, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::TextMuted), entry.source.c_str());
	x += ImGui::CalcTextSize(entry.source.c_str()).x + 10.0f;

	const char* messageBegin = entry.message.c_str();
	const char* messageEnd = std::strchr(messageBegin, '\n');
	const auto& textColor = entry.level == Gleam::Logger::Level::Trace ? Gleam::Theme::TextMuted : Gleam::Theme::Text;
	drawList->AddText(ImVec2(x, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(textColor), messageBegin, messageEnd);
	drawList->PopClipRect();

	ImGui::PopID();
}

void Console::DrawDetails()
{
	auto& entry = mEntries[mSelectedEntry];

	const ImVec2 min = ImGui::GetCursorScreenPos();
	ImGui::GetWindowDrawList()->AddLine(min, ImVec2(min.x + ImGui::GetWindowWidth(), min.y), Widgets::ColorU32(Gleam::Theme::Divider));

	ImGui::SetCursorPos(ImVec2(kPadding, ImGui::GetCursorPosY() + kPadding));
	const ImVec2 available = ImGui::GetContentRegionAvail();

	ImGui::PushFont(mFonts->detail);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, Gleam::Theme::Panel);
	ImGui::InputTextMultiline("##ConsoleDetails", entry.message.data(), entry.message.size() + 1, ImVec2(available.x - kPadding, available.y - kPadding), ImGuiInputTextFlags_ReadOnly);
	ImGui::PopStyleColor();
	ImGui::PopFont();
}

void Console::CopyEntry(const Entry& entry) const
{
	Gleam::TString text;
	fmt::format_to(std::back_inserter(text), "[{}] {}{}: {}", entry.time, Gleam::Logger::LogLevelToString(entry.level), entry.source, entry.message);
	ImGui::SetClipboardText(text.c_str());
}

Console::Category Console::GetCategory(Gleam::Logger::Level level)
{
	switch (level)
	{
		case Gleam::Logger::Level::Warn: return Category::Warning;
		case Gleam::Logger::Level::Error: return Category::Error;
		default: return Category::Info;
	}
}
