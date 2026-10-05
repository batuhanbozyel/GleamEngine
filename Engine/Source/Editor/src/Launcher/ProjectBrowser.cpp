#include "ProjectBrowser.h"
#include "GleamLauncher.h"
#include "LauncherState.h"
#include "Core/ConfigSystem.h"
#include "View/ViewStack.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "View/Widgets/EditorWidgets.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Process.h"
#include "Core/Application.h"
#include "Core/EngineDefines.h"
#include "Core/Events/Event.h"
#include "Core/Events/ApplicationEvent.h"
#include "IO/FileDialog.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <Editor.Reflection.generated.h>

#include <imgui.h>

#include <chrono>
#include <ctime>
#include <cctype>
#include <algorithm>

using namespace GEditor;

#if defined(USE_DIRECTX_RENDERER)
static constexpr const char* kBackendName = "D3D12";
#elif defined(USE_METAL_RENDERER)
static constexpr const char* kBackendName = "Metal";
#endif

static constexpr float kSidebarWidth = 232.0f;
static constexpr float kDetailsWidth = 340.0f;
static constexpr float kTableHeaderHeight = 34.0f;
static constexpr float kTablePadding = 16.0f;
static constexpr float kRowHeight = 64.0f;
static constexpr float kAvatarSize = 44.0f;
static constexpr float kNameColumnOffset = kTablePadding + kAvatarSize + 16.0f;
static constexpr float kLastOpenedColumnWidth = 110.0f;

static constexpr ImVec4 kAvatarColors[] = {
	Gleam::Theme::Hex(0x2A3445),
	Gleam::Theme::Hex(0x3A2E2B),
	Gleam::Theme::Hex(0x2E3A30),
	Gleam::Theme::Hex(0x342E40),
	Gleam::Theme::Hex(0x3A3328)
};

static ImU32 ColorU32(const ImVec4& color, float alpha = 1.0f)
{
	return ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * alpha));
}

static void DrawIcon(ImDrawList* drawList, const char* icon, const ImVec2& center, const ImVec4& color)
{
	const ImVec2 size = ImGui::CalcTextSize(icon);
	drawList->AddText(ImVec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f), ColorU32(color), icon);
}

static void AddVerticalGap(float gap)
{
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y + gap);
}

static void DrawBadge(const char* text)
{
	const ImVec2 padding(8.0f, 3.0f);
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + textSize.x + padding.x * 2.0f, min.y + textSize.y + padding.y * 2.0f);

	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, ColorU32(Gleam::Theme::Control), 4.0f);
	drawList->AddRect(min, max, ColorU32(Gleam::Theme::Border), 4.0f);
	drawList->AddText(ImVec2(min.x + padding.x, min.y + padding.y), ColorU32(Gleam::Theme::TextBadge), text);
	ImGui::Dummy(ImVec2(max.x - min.x, max.y - min.y));
}

static void DrawHorizontalDivider()
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	ImGui::GetWindowDrawList()->AddLine(min, ImVec2(min.x + ImGui::GetContentRegionAvail().x, min.y), ColorU32(Gleam::Theme::Divider));
	ImGui::Dummy(ImVec2(0.0f, 1.0f));
}

static bool ContainsInsensitive(const Gleam::TString& text, const char* query)
{
	const auto queryEnd = query + std::strlen(query);
	const auto it = std::search(text.begin(), text.end(), query, queryEnd, [](char lhs, char rhs)
	{
		return std::tolower(static_cast<unsigned char>(lhs)) == std::tolower(static_cast<unsigned char>(rhs));
	});
	return query == queryEnd or it != text.end();
}

static Gleam::TString GetInitials(const Gleam::TString& name)
{
	Gleam::TString initials;
	bool wordStart = true;
	for (char c : name)
	{
		const bool space = std::isspace(static_cast<unsigned char>(c)) != 0;
		if (not space and wordStart and initials.size() < 2)
		{
			initials.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
		}
		wordStart = space;
	}

	if (initials.size() == 1)
	{
		const auto second = name.find_first_not_of(" \t", name.find_first_not_of(" \t") + 1);
		if (second != Gleam::TString::npos)
		{
			initials.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(name[second]))));
		}
	}
	if (initials.empty())
	{
		initials = "?";
	}
	return initials;
}

static const ImVec4& GetAvatarColor(const Gleam::TString& name)
{
	uint32_t hash = 2166136261u;
	for (char c : name)
	{
		hash = (hash ^ static_cast<uint8_t>(c)) * 16777619u;
	}
	return kAvatarColors[hash % (sizeof(kAvatarColors) / sizeof(kAvatarColors[0]))];
}

static std::tm ToLocalTime(std::time_t time)
{
	std::tm result = {};
#if defined(PLATFORM_WINDOWS)
	localtime_s(&result, &time);
#else
	localtime_r(&time, &result);
#endif
	return result;
}

static Gleam::TString FormatLastOpened(uint64_t lastOpened)
{
	static constexpr const char* kMonths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	constexpr uint64_t kMinute = 60;
	constexpr uint64_t kHour = 60 * kMinute;
	constexpr uint64_t kDay = 24 * kHour;

	const auto now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	const uint64_t elapsed = now > lastOpened ? now - lastOpened : 0;

	char buffer[64] = {};
	if (lastOpened == 0)
	{
		return "Never";
	}
	else if (elapsed < kMinute)
	{
		return "Just now";
	}
	else if (elapsed < kHour)
	{
		const auto minutes = elapsed / kMinute;
		std::snprintf(buffer, sizeof(buffer), "%llu minute%s ago", static_cast<unsigned long long>(minutes), minutes == 1 ? "" : "s");
	}
	else if (elapsed < kDay)
	{
		const auto hours = elapsed / kHour;
		std::snprintf(buffer, sizeof(buffer), "%llu hour%s ago", static_cast<unsigned long long>(hours), hours == 1 ? "" : "s");
	}
	else if (elapsed < 2 * kDay)
	{
		return "Yesterday";
	}
	else if (elapsed < 7 * kDay)
	{
		std::snprintf(buffer, sizeof(buffer), "%llu days ago", static_cast<unsigned long long>(elapsed / kDay));
	}
	else if (elapsed < 14 * kDay)
	{
		return "Last week";
	}
	else
	{
		const auto openedTime = ToLocalTime(static_cast<std::time_t>(lastOpened));
		const auto currentTime = ToLocalTime(static_cast<std::time_t>(now));
		if (openedTime.tm_year == currentTime.tm_year)
		{
			std::snprintf(buffer, sizeof(buffer), "%s %d", kMonths[openedTime.tm_mon], openedTime.tm_mday);
		}
		else
		{
			std::snprintf(buffer, sizeof(buffer), "%s %d, %d", kMonths[openedTime.tm_mon], openedTime.tm_mday, openedTime.tm_year + 1900);
		}
	}
	return buffer;
}

void ProjectBrowser::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void ProjectBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
		const bool visible = ImGui::Begin("Projects", nullptr, flags);
		ImGui::PopStyleVar(2);

		if (visible)
		{
			DrawSidebar();
			ImGui::SameLine(0.0f, 0.0f);
			DrawProjectList();
			ImGui::SameLine(0.0f, 0.0f);
			DrawDetails();
			DrawNewProjectDialog();
		}
		ImGui::End();
	});
}

void ProjectBrowser::DrawSidebar()
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Gleam::Theme::Panel);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 24.0f));
	ImGui::BeginChild("Sidebar", ImVec2(kSidebarWidth, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	auto drawList = ImGui::GetWindowDrawList();
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const float dividerX = windowPos.x + windowSize.x - 1.0f;
	drawList->AddLine(ImVec2(dividerX, windowPos.y), ImVec2(dividerX, windowPos.y + windowSize.y), ColorU32(Gleam::Theme::Divider));

	// Brand
	constexpr float kLogoSize = 36.0f;
	const ImVec2 brandMin = ImGui::GetCursorScreenPos();
	const ImVec2 logoMin(brandMin.x + 8.0f, brandMin.y);
	const ImVec2 logoMax(logoMin.x + kLogoSize, logoMin.y + kLogoSize);
	drawList->AddRectFilled(logoMin, logoMax, ColorU32(Gleam::Theme::Control), 8.0f);
	drawList->AddRect(logoMin, logoMax, ColorU32(Gleam::Theme::Border), 8.0f);
	ImGui::PushFont(mFonts->heading);
	DrawIcon(drawList, ICON_LC_SPARKLE, ImVec2((logoMin.x + logoMax.x) * 0.5f, (logoMin.y + logoMax.y) * 0.5f), Gleam::Theme::Accent);
	ImGui::PopFont();

	const float brandTextHeight = mFonts->brand->LegacySize + 2.0f + mFonts->mono->LegacySize;
	const float brandTextY = logoMin.y + (kLogoSize - brandTextHeight) * 0.5f;
	ImGui::PushFont(mFonts->brand);
	drawList->AddText(ImVec2(logoMax.x + 12.0f, brandTextY), ColorU32(Gleam::Theme::Text), "GleamEngine");
	ImGui::PopFont();

	char version[32] = {};
	std::snprintf(version, sizeof(version), "v%d.%d.%d", GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION);
	ImGui::PushFont(mFonts->mono);
	drawList->AddText(ImVec2(logoMax.x + 12.0f, brandTextY + mFonts->brand->LegacySize + 2.0f), ColorU32(Gleam::Theme::TextMuted), version);
	ImGui::PopFont();

	// Navigation
	constexpr float kNavHeight = 40.0f;
	ImGui::SetCursorScreenPos(ImVec2(brandMin.x, logoMax.y + 28.0f));
	const float navWidth = ImGui::GetContentRegionAvail().x;
	const ImVec2 navMin = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton("##ProjectsNav", ImVec2(navWidth, kNavHeight));
	drawList->AddRectFilled(navMin, ImVec2(navMin.x + navWidth, navMin.y + kNavHeight), ColorU32(Gleam::Theme::Control), 6.0f);
	ImGui::PushFont(mFonts->medium);
	DrawIcon(drawList, ICON_LC_FOLDER, ImVec2(navMin.x + 21.0f, navMin.y + kNavHeight * 0.5f), Gleam::Theme::Accent);
	drawList->AddText(ImVec2(navMin.x + 42.0f, navMin.y + (kNavHeight - ImGui::GetFontSize()) * 0.5f), ColorU32(Gleam::Theme::Text), "Projects");
	ImGui::PopFont();

	// Available backends
	constexpr float kFooterPadding = 14.0f;
	constexpr float kFooterGap = 10.0f;
	const float badgeHeight = mFonts->mono->LegacySize + 6.0f;
	const float footerHeight = kFooterPadding + mFonts->caption->LegacySize + kFooterGap + badgeHeight;
	const float footerY = windowPos.y + windowSize.y - 18.0f - footerHeight;
	drawList->AddLine(ImVec2(brandMin.x, footerY), ImVec2(brandMin.x + navWidth, footerY), ColorU32(Gleam::Theme::Divider));

	ImGui::PushFont(mFonts->caption);
	drawList->AddText(ImVec2(brandMin.x + 12.0f, footerY + kFooterPadding), ColorU32(Gleam::Theme::TextDim), "AVAILABLE BACKENDS");
	ImGui::PopFont();

	ImGui::SetCursorScreenPos(ImVec2(brandMin.x + 12.0f, footerY + kFooterPadding + mFonts->caption->LegacySize + kFooterGap));
	ImGui::PushFont(mFonts->mono);
	DrawBadge(kBackendName);
	ImGui::PopFont();

	ImGui::EndChild();
}

void ProjectBrowser::DrawProjectList()
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Gleam::Theme::Background);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 28.0f));
	ImGui::BeginChild("ProjectList", ImVec2(-kDetailsWidth, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	const float contentWidth = ImGui::GetContentRegionAvail().x;
	const ImVec2 headerPos = ImGui::GetCursorPos();

	// Title
	ImGui::PushFont(mFonts->title);
	ImGui::TextUnformatted("Projects");
	ImGui::PopFont();
	AddVerticalGap(4.0f);
	ImGui::PushFont(mFonts->compact);
	ImGui::TextColored(Gleam::Theme::TextMuted, "Recently opened on this machine");
	ImGui::PopFont();
	const float headerBottom = ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y;

	// Actions
	constexpr float kButtonHeight = 40.0f;
	constexpr float kButtonPadding = 32.0f;
	constexpr const char* kOpenLabel = ICON_LC_FOLDER_OPEN "  Open...";
	constexpr const char* kNewLabel = ICON_LC_PLUS "  New project";
	ImGui::PushFont(mFonts->medium);
	const float openWidth = ImGui::CalcTextSize(kOpenLabel).x + kButtonPadding;
	ImGui::PopFont();
	ImGui::PushFont(mFonts->semiBold);
	const float newWidth = ImGui::CalcTextSize(kNewLabel).x + kButtonPadding;
	ImGui::PopFont();

	ImGui::SetCursorPos(ImVec2(headerPos.x + contentWidth - openWidth - newWidth - 10.0f, headerBottom - kButtonHeight));
	ImGui::PushFont(mFonts->medium);
	if (ImGui::Button(kOpenLabel, ImVec2(openWidth, kButtonHeight)))
	{
		auto files = Gleam::FileDialog::Open(L"Gleam Project", L"*.gproj");
		if (files.empty() == false)
		{
			OpenProject(files.front());
		}
	}
	ImGui::PopFont();
	ImGui::SameLine(0.0f, 10.0f);
	ImGui::PushFont(mFonts->semiBold);
	if (Widgets::AccentButton(kNewLabel, ImVec2(newWidth, kButtonHeight)))
	{
		mOpenNewProjectDialog = true;
	}
	ImGui::PopFont();

	// Search and sort
	constexpr float kFieldHeight = 38.0f;
	constexpr float kSortComboWidth = 140.0f;
	ImGui::SetCursorPos(ImVec2(headerPos.x, headerBottom + 20.0f));
	ImGui::PushFont(mFonts->compact);
	const float sortLabelWidth = ImGui::CalcTextSize("Sort").x;
	ImGui::PopFont();

	const float fieldPaddingY = (kFieldHeight - ImGui::GetFontSize()) * 0.5f;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, fieldPaddingY));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, Gleam::Theme::Panel);
	ImGui::PushStyleColor(ImGuiCol_Border, Gleam::Theme::Border);
	const ImVec2 searchMin = ImGui::GetCursorScreenPos();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(38.0f, fieldPaddingY));
	ImGui::SetNextItemWidth(contentWidth - kSortComboWidth - sortLabelWidth - 24.0f);
	ImGui::InputTextWithHint("##Search", "Search by name or path", mSearch, sizeof(mSearch));
	ImGui::PopStyleVar();
	DrawIcon(ImGui::GetWindowDrawList(), ICON_LC_SEARCH, ImVec2(searchMin.x + 20.0f, searchMin.y + ImGui::GetItemRectSize().y * 0.5f), Gleam::Theme::TextDim);

	ImGui::SameLine(0.0f, 12.0f);
	ImGui::PushFont(mFonts->compact);
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, searchMin.y + (kFieldHeight - ImGui::GetFontSize()) * 0.5f));
	ImGui::TextColored(Gleam::Theme::TextMuted, "Sort");
	ImGui::PopFont();

	ImGui::SameLine(0.0f, 12.0f);
	ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, searchMin.y));
	ImGui::SetNextItemWidth(kSortComboWidth);
	const char* kSortModes[] = { "Last opened", "Name" };
	ImGui::Combo("##Sort", &mSortMode, kSortModes, IM_ARRAYSIZE(kSortModes));
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();
	ImGui::SetCursorScreenPos(ImVec2(searchMin.x, searchMin.y + kFieldHeight + 20.0f));

	// Filter and sort the recent projects
	auto configSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::ConfigSystem>();
	const auto& recentProjects = configSystem->Get<Gleam::LauncherState>().recentProjects;

	Gleam::TArray<uint32_t> visibleProjects;
	for (uint32_t i = 0; i < static_cast<uint32_t>(recentProjects.size()); ++i)
	{
		const auto& project = recentProjects[i];
		if (ContainsInsensitive(project.name, mSearch) or ContainsInsensitive(project.path.String(), mSearch))
		{
			visibleProjects.push_back(i);
		}
	}

	if (mSortMode == 1)
	{
		std::stable_sort(visibleProjects.begin(), visibleProjects.end(), [&](uint32_t lhs, uint32_t rhs)
		{
			return recentProjects[lhs].name < recentProjects[rhs].name;
		});
	}

	const bool selectionExists = eastl::any_of(recentProjects.begin(), recentProjects.end(), [this](const Gleam::RecentProject& project)
	{
		return project.path == mSelectedProject;
	});
	if (selectionExists == false)
	{
		mSelectedProject = visibleProjects.empty() ? Gleam::Path() : recentProjects[visibleProjects.front()].path;
	}

	DrawProjectTable(recentProjects, visibleProjects);

	ImGui::EndChild();
}

void ProjectBrowser::DrawProjectTable(const Gleam::TArray<Gleam::RecentProject>& projects, const Gleam::TArray<uint32_t>& visibleProjects)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Gleam::Theme::Background);
	ImGui::PushStyleColor(ImGuiCol_Border, Gleam::Theme::Divider);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("ProjectTable", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);

	auto headerDrawList = ImGui::GetWindowDrawList();
	const float tableWidth = ImGui::GetContentRegionAvail().x;
	const ImVec2 headerMin = ImGui::GetCursorScreenPos();
	headerDrawList->AddRectFilled(headerMin, ImVec2(headerMin.x + tableWidth, headerMin.y + kTableHeaderHeight), ColorU32(Gleam::Theme::Panel));
	headerDrawList->AddLine(ImVec2(headerMin.x, headerMin.y + kTableHeaderHeight - 1.0f), ImVec2(headerMin.x + tableWidth, headerMin.y + kTableHeaderHeight - 1.0f), ColorU32(Gleam::Theme::Divider));

	ImGui::PushFont(mFonts->caption);
	const float headerTextY = headerMin.y + (kTableHeaderHeight - ImGui::GetFontSize()) * 0.5f;
	headerDrawList->AddText(ImVec2(headerMin.x + kNameColumnOffset, headerTextY), ColorU32(Gleam::Theme::TextDim), "NAME");
	headerDrawList->AddText(ImVec2(headerMin.x + tableWidth - kTablePadding - kLastOpenedColumnWidth, headerTextY), ColorU32(Gleam::Theme::TextDim), "LAST OPENED");
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(tableWidth, kTableHeaderHeight));

	ImGui::PushStyleColor(ImGuiCol_ChildBg, Gleam::Theme::Hex(0x000000, 0.0f));
	ImGui::BeginChild("ProjectRows", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None);
	ImGui::PopStyleColor();
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	if (visibleProjects.empty())
	{
		const char* message = projects.empty() ? "No recent projects. Open or create one to get started." : "No projects match your search.";
		const ImVec2 messageSize = ImGui::CalcTextSize(message);
		ImGui::SetCursorPos(ImVec2((ImGui::GetContentRegionAvail().x - messageSize.x) * 0.5f, 32.0f));
		ImGui::TextColored(Gleam::Theme::TextDim, "%s", message);
	}

	auto drawList = ImGui::GetWindowDrawList();
	Gleam::Path projectToOpen;
	for (uint32_t row = 0; row < static_cast<uint32_t>(visibleProjects.size()); ++row)
	{
		const auto& project = projects[visibleProjects[row]];
		const bool missing = Gleam::Filesystem::Exists(project.path) == false;
		const bool selected = project.path == mSelectedProject;

		ImGui::PushID(static_cast<int>(row));
		const float rowWidth = ImGui::GetContentRegionAvail().x;
		const ImVec2 rowMin = ImGui::GetCursorScreenPos();
		const ImVec2 rowMax(rowMin.x + rowWidth, rowMin.y + kRowHeight);
		ImGui::InvisibleButton("##Row", ImVec2(rowWidth, kRowHeight));
		const bool hovered = ImGui::IsItemHovered();
		if (ImGui::IsItemClicked())
		{
			mSelectedProject = project.path;
		}
		if (hovered and missing == false and ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			projectToOpen = project.path;
		}

		if (selected)
		{
			drawList->AddRectFilled(rowMin, rowMax, ColorU32(Gleam::Theme::Selected));
			drawList->AddRectFilled(rowMin, ImVec2(rowMin.x + 3.0f, rowMax.y), ColorU32(Gleam::Theme::Accent));
		}
		else if (hovered)
		{
			drawList->AddRectFilled(rowMin, rowMax, ColorU32(Gleam::Theme::Panel));
		}
		drawList->AddLine(ImVec2(rowMin.x, rowMax.y - 1.0f), ImVec2(rowMax.x, rowMax.y - 1.0f), ColorU32(Gleam::Theme::RowDivider));

		const float alpha = missing ? 0.45f : 1.0f;

		// Avatar
		const ImVec2 avatarMin(rowMin.x + kTablePadding, rowMin.y + (kRowHeight - kAvatarSize) * 0.5f);
		const ImVec2 avatarMax(avatarMin.x + kAvatarSize, avatarMin.y + kAvatarSize);
		drawList->AddRectFilled(avatarMin, avatarMax, ColorU32(GetAvatarColor(project.name), alpha), 6.0f);

		const auto initials = GetInitials(project.name);
		ImGui::PushFont(mFonts->semiBold);
		const ImVec2 initialsSize = ImGui::CalcTextSize(initials.c_str());
		drawList->AddText(ImVec2(avatarMin.x + (kAvatarSize - initialsSize.x) * 0.5f, avatarMin.y + (kAvatarSize - initialsSize.y) * 0.5f), ColorU32(Gleam::Theme::Text, alpha), initials.c_str());
		ImGui::PopFont();

		// Name and location
		const float nameX = rowMin.x + kNameColumnOffset;
		const float lastOpenedX = rowMax.x - kTablePadding - kLastOpenedColumnWidth;
		const float nameY = rowMin.y + (kRowHeight - (mFonts->label->LegacySize + 3.0f + mFonts->detail->LegacySize)) * 0.5f;
		drawList->PushClipRect(ImVec2(nameX, rowMin.y), ImVec2(lastOpenedX - 16.0f, rowMax.y), true);
		ImGui::PushFont(mFonts->label);
		drawList->AddText(ImVec2(nameX, nameY), ColorU32(Gleam::Theme::Text, alpha), project.name.c_str());
		ImGui::PopFont();

		ImGui::PushFont(mFonts->detail);
		const auto location = missing ? "Missing - " + project.path.Parent().String() : project.path.Parent().String();
		drawList->AddText(ImVec2(nameX, nameY + mFonts->label->LegacySize + 3.0f), ColorU32(missing ? Gleam::Theme::Warning : Gleam::Theme::TextMuted, alpha), location.c_str());
		ImGui::PopFont();
		drawList->PopClipRect();

		// Last opened
		const auto lastOpened = FormatLastOpened(project.lastOpened);
		ImGui::PushFont(mFonts->compact);
		drawList->AddText(ImVec2(lastOpenedX, rowMin.y + (kRowHeight - ImGui::GetFontSize()) * 0.5f), ColorU32(Gleam::Theme::TextMuted, alpha), lastOpened.c_str());
		ImGui::PopFont();

		ImGui::PopID();
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::EndChild();

	if (projectToOpen.Empty() == false)
	{
		OpenProject(projectToOpen);
	}
}

void ProjectBrowser::DrawDetails()
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Gleam::Theme::Panel);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 24.0f));
	ImGui::BeginChild("Details", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	auto drawList = ImGui::GetWindowDrawList();
	const ImVec2 windowPos = ImGui::GetWindowPos();
	drawList->AddLine(windowPos, ImVec2(windowPos.x, windowPos.y + ImGui::GetWindowHeight()), ColorU32(Gleam::Theme::Divider));

	auto configSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::ConfigSystem>();
	const auto& recentProjects = configSystem->Get<Gleam::LauncherState>().recentProjects;
	const auto it = eastl::find_if(recentProjects.begin(), recentProjects.end(), [this](const Gleam::RecentProject& project)
	{
		return project.path == mSelectedProject;
	});

	if (it == recentProjects.end())
	{
		const char* message = "Select a project";
		const ImVec2 messageSize = ImGui::CalcTextSize(message);
		ImGui::SetCursorPos(ImVec2((ImGui::GetWindowWidth() - messageSize.x) * 0.5f, (ImGui::GetWindowHeight() - messageSize.y) * 0.5f));
		ImGui::TextColored(Gleam::Theme::TextDim, "%s", message);
	}
	else
	{
		const Gleam::Path projectFile = it->path;
		const Gleam::TString projectName = it->name;
		const uint64_t lastOpened = it->lastOpened;
		const bool missing = Gleam::Filesystem::Exists(projectFile) == false;
		const float width = ImGui::GetContentRegionAvail().x;

		// Capture placeholder
		constexpr float kCaptureHeight = 168.0f;
		const ImVec2 captureMin = ImGui::GetCursorScreenPos();
		const ImVec2 captureMax(captureMin.x + width, captureMin.y + kCaptureHeight);
		drawList->AddRectFilled(captureMin, captureMax, ColorU32(GetAvatarColor(projectName)), 8.0f);
		drawList->AddRect(captureMin, captureMax, ColorU32(Gleam::Theme::Border), 8.0f);

		ImGui::PushFont(mFonts->mono);
		constexpr const char* kCaptureLabel = "[LAST VIEWPORT CAPTURE]";
		const ImVec2 labelSize = ImGui::CalcTextSize(kCaptureLabel);
		const ImVec2 labelMax(captureMin.x + 12.0f + labelSize.x + 14.0f, captureMax.y - 12.0f);
		const ImVec2 labelMin(captureMin.x + 12.0f, labelMax.y - labelSize.y - 6.0f);
		drawList->AddRectFilled(labelMin, labelMax, ColorU32(Gleam::Theme::Background), 4.0f);
		drawList->AddText(ImVec2(labelMin.x + 7.0f, labelMin.y + 3.0f), ColorU32(Gleam::Theme::TextBadge), kCaptureLabel);
		ImGui::PopFont();
		ImGui::Dummy(ImVec2(width, kCaptureHeight));
		AddVerticalGap(20.0f);

		// Name and location
		ImGui::PushFont(mFonts->heading);
		ImGui::TextUnformatted(projectName.c_str());
		ImGui::PopFont();
		AddVerticalGap(6.0f);

		ImGui::PushFont(mFonts->detail);
		ImGui::PushTextWrapPos(0.0f);
		ImGui::TextColored(missing ? Gleam::Theme::Warning : Gleam::Theme::TextMuted, "%s%s", missing ? "Missing - " : "", projectFile.Parent().String().c_str());
		ImGui::PopTextWrapPos();
		ImGui::PopFont();
		AddVerticalGap(20.0f);

		DrawHorizontalDivider();
		AddVerticalGap(14.0f);

		// Last opened
		const auto lastOpenedText = FormatLastOpened(lastOpened);
		ImGui::PushFont(mFonts->compact);
		ImGui::TextColored(Gleam::Theme::TextMuted, "Last opened");
		ImGui::SameLine(width - ImGui::CalcTextSize(lastOpenedText.c_str()).x);
		ImGui::TextUnformatted(lastOpenedText.c_str());
		ImGui::PopFont();
		AddVerticalGap(14.0f);

		DrawHorizontalDivider();

		// Actions
		constexpr float kPrimaryHeight = 44.0f;
		constexpr float kSecondaryHeight = 40.0f;
		constexpr float kSpacing = 8.0f;
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 24.0f - kPrimaryHeight - kSpacing - kSecondaryHeight);

		ImGui::PushFont(mFonts->semiBold);
		ImGui::BeginDisabled(missing);
		if (Widgets::AccentButton("Open project", ImVec2(width, kPrimaryHeight)))
		{
			OpenProject(projectFile);
		}
		ImGui::EndDisabled();
		ImGui::PopFont();

		const float halfWidth = (width - kSpacing) * 0.5f;
		AddVerticalGap(kSpacing);
		ImGui::PushFont(mFonts->compact);
		ImGui::BeginDisabled(missing);
		if (ImGui::Button("Show in folder", ImVec2(halfWidth, kSecondaryHeight)))
		{
			Gleam::Process::RevealInFileBrowser(projectFile);
		}
		ImGui::EndDisabled();
		ImGui::SameLine(0.0f, kSpacing);
		if (Widgets::OutlineButton("Remove from list", ImVec2(halfWidth, kSecondaryHeight)))
		{
			GleamLauncher::RemoveRecentProject(projectFile);
		}
		ImGui::PopFont();
	}

	ImGui::EndChild();
}

void ProjectBrowser::DrawNewProjectDialog()
{
	if (mOpenNewProjectDialog)
	{
		mOpenNewProjectDialog = false;
		if (mNewProjectLocation.Empty())
		{
			auto configSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::ConfigSystem>();
			const auto& recentProjects = configSystem->Get<Gleam::LauncherState>().recentProjects;
			if (recentProjects.empty() == false)
			{
				mNewProjectLocation = recentProjects.front().path.Parent().Parent();
			}
		}
		ImGui::OpenPopup("New Project");
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));
	const bool open = ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleVar();
	if (open)
	{
		ImGui::PushFont(mFonts->caption);
		ImGui::TextColored(Gleam::Theme::TextDim, "NAME");
		ImGui::PopFont();
		ImGui::SetNextItemWidth(420.0f);
		ImGui::InputText("##Name", mNewProjectName, sizeof(mNewProjectName));

		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		ImGui::PushFont(mFonts->caption);
		ImGui::TextColored(Gleam::Theme::TextDim, "LOCATION");
		ImGui::PopFont();

		const auto location = mNewProjectLocation.String();
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(location.empty() ? Gleam::Theme::TextDim : Gleam::Theme::TextSecondary, "%s", location.empty() ? "No folder selected" : location.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Browse..."))
		{
			if (auto folder = Gleam::FileDialog::OpenFolder(); folder.Empty() == false)
			{
				mNewProjectLocation = folder;
			}
		}

		const Gleam::TString name = mNewProjectName;
		const auto error = GleamLauncher::ValidateNewProject(name, mNewProjectLocation);
		if (error.empty() == false)
		{
			ImGui::TextColored(Gleam::Theme::Error, "%s", error.c_str());
		}

		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		DrawHorizontalDivider();
		ImGui::Dummy(ImVec2(0.0f, 4.0f));

		ImGui::PushFont(mFonts->semiBold);
		ImGui::BeginDisabled(error.empty() == false);
		if (Widgets::AccentButton("Create", ImVec2(120.0f, 36.0f)))
		{
			if (auto projectFile = GleamLauncher::CreateProject(name, mNewProjectLocation); projectFile.Empty() == false)
			{
				ImGui::CloseCurrentPopup();
				OpenProject(projectFile);
			}
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (Widgets::OutlineButton("Cancel", ImVec2(120.0f, 36.0f)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::PopFont();
		ImGui::EndPopup();
	}
}

void ProjectBrowser::OpenProject(const Gleam::Path& projectFile)
{
	auto project = GleamLauncher::OpenProject(projectFile);
	auto arguments = Gleam::TArray<Gleam::TString>{ "-project=" + projectFile.String() };
	GleamLauncher::AddRecentProject(projectFile, project.name);

	if (Gleam::Process::Launch(Gleam::Process::ExecutablePath(), arguments, Gleam::Filesystem::WorkingDirectory()))
	{
		Gleam::EventDispatcher<Gleam::AppCloseEvent>::Publish(Gleam::AppCloseEvent());
	}
	else
	{
		GLEAM_ERROR("Failed to launch editor for project: {0}", arguments.front());
	}
}
