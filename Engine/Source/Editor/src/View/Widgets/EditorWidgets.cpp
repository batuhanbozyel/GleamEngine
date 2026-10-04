#include "EditorWidgets.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "Utils/ReflectionUtils.h"

#include "World/EntityManager.h"
#include "World/Components/Camera.h"
#include "World/Components/MeshRenderer.h"
#include "World/Components/RigidBody.h"
#include "World/Components/SkyAtmosphere.h"
#include "World/Components/ReflectionProbe.h"

#include <Runtime.Reflection.generated.h>

#include <imgui_internal.h>

#include <cstdio>

using namespace GEditor;

static ImVec4 GetComponentColor(uint32_t typeHash)
{
	static const auto meshRenderer = Gleam::Reflection::GetClass<Gleam::MeshRenderer>().TypeHash();
	static const auto camera = Gleam::Reflection::GetClass<Gleam::Camera>().TypeHash();
	static const auto skyAtmosphere = Gleam::Reflection::GetClass<Gleam::SkyAtmosphere>().TypeHash();
	static const auto reflectionProbe = Gleam::Reflection::GetClass<Gleam::ReflectionProbe>().TypeHash();
	static const auto rigidBody = Gleam::Reflection::GetClass<Gleam::RigidBody>().TypeHash();

	if (typeHash == meshRenderer)
	{
		return Gleam::Theme::Info;
	}
	else if (typeHash == camera)
	{
		return Gleam::Theme::TextSecondary;
	}
	else if (typeHash == skyAtmosphere)
	{
		return Gleam::Theme::Warning;
	}
	else if (typeHash == reflectionProbe)
	{
		return Gleam::Theme::Purple;
	}
	else if (typeHash == rigidBody)
	{
		return Gleam::Theme::Success;
	}
	else
	{
		return Gleam::Theme::TextDim;
	}
}

ImU32 Widgets::ColorU32(const ImVec4& color, float alpha)
{
	return ImGui::GetColorU32(ImVec4(color.x, color.y, color.z, color.w * alpha));
}

void Widgets::DrawIcon(ImDrawList* drawList, const char* icon, const ImVec2& center, const ImVec4& color)
{
	const ImVec2 size = ImGui::CalcTextSize(icon);
	drawList->AddText(ImVec2(IM_ROUND(center.x - size.x * 0.5f), IM_ROUND(center.y - size.y * 0.5f)), ColorU32(color), icon);
}

void Widgets::DrawDashedRect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, ImU32 color, float dash, float gap)
{
	auto drawDashedLine = [&](const ImVec2& from, const ImVec2& to)
	{
		const float length = ImMax(ImFabs(to.x - from.x), ImFabs(to.y - from.y));
		const ImVec2 direction((to.x - from.x) / length, (to.y - from.y) / length);
		for (float offset = 0.0f; offset < length; offset += dash + gap)
		{
			const float end = ImMin(offset + dash, length);
			drawList->AddLine(ImVec2(from.x + direction.x * offset, from.y + direction.y * offset), ImVec2(from.x + direction.x * end, from.y + direction.y * end), color);
		}
	};

	const ImVec2 innerMin(min.x + 0.5f, min.y + 0.5f);
	const ImVec2 innerMax(max.x - 0.5f, max.y - 0.5f);
	drawDashedLine(innerMin, ImVec2(innerMax.x, innerMin.y));
	drawDashedLine(ImVec2(innerMax.x, innerMin.y), innerMax);
	drawDashedLine(innerMax, ImVec2(innerMin.x, innerMax.y));
	drawDashedLine(ImVec2(innerMin.x, innerMax.y), innerMin);
}

bool Widgets::SearchField(const char* id, const char* hint, char* buffer, size_t bufferSize, float width, float height)
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const float paddingY = (height - ImGui::GetFontSize()) * 0.5f;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(32.0f, paddingY));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, Gleam::Theme::Background);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Gleam::Theme::Background);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Gleam::Theme::Background);
	ImGui::PushStyleColor(ImGuiCol_Border, Gleam::Theme::Border);
	ImGui::SetNextItemWidth(width);
	const bool changed = ImGui::InputTextWithHint(id, hint, buffer, bufferSize);
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(3);

	DrawIcon(ImGui::GetWindowDrawList(), ICON_LC_SEARCH, ImVec2(min.x + 17.0f, min.y + height * 0.5f), Gleam::Theme::TextDim);
	return changed;
}

void Widgets::CaptionRow(const EditorFonts& fonts, const char* label, uint32_t count, float height, float paddingX)
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	auto drawList = ImGui::GetWindowDrawList();

	ImGui::PushFont(fonts.caption);
	drawList->AddText(ImVec2(min.x + paddingX, min.y + (height - ImGui::GetFontSize()) * 0.5f), ColorU32(Gleam::Theme::TextDim), label);
	ImGui::PopFont();

	char countText[16] = {};
	std::snprintf(countText, sizeof(countText), "%u", count);
	ImGui::PushFont(fonts.mono);
	const ImVec2 countSize = ImGui::CalcTextSize(countText);
	drawList->AddText(ImVec2(min.x + width - paddingX - countSize.x, min.y + (height - ImGui::GetFontSize()) * 0.5f), ColorU32(Gleam::Theme::TextDim), countText);
	ImGui::PopFont();

	ImGui::Dummy(ImVec2(width, height));
}

static bool DrawOverlayButton(const char* id, const char* icon, const ImVec2& size, const ImVec4* iconColor, bool active)
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + size.x, min.y + size.y);
	const bool pressed = ImGui::InvisibleButton(id, size);
	const bool hovered = ImGui::IsItemHovered();

	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, hovered ? Widgets::ColorU32(Gleam::Theme::Control, 0.9f) : Widgets::ColorU32(Gleam::Theme::Background, 0.72f), 6.0f);
	drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::BorderStrong), 6.0f);
	const auto& color = iconColor ? *iconColor : (active ? Gleam::Theme::Accent : (hovered ? Gleam::Theme::Text : Gleam::Theme::TextSecondary));
	Widgets::DrawIcon(drawList, icon, ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f), color);
	return pressed;
}

bool Widgets::OverlayButton(const char* id, const char* icon, const ImVec2& size, bool active)
{
	return DrawOverlayButton(id, icon, size, nullptr, active);
}

bool Widgets::OverlayButton(const char* id, const char* icon, const ImVec2& size, const ImVec4& iconColor)
{
	return DrawOverlayButton(id, icon, size, &iconColor, false);
}

bool Widgets::AccentButton(const char* label, const ImVec2& size)
{
	ImGui::PushStyleColor(ImGuiCol_Button, Gleam::Theme::Accent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Gleam::Theme::AccentHover);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, Gleam::Theme::AccentActive);
	ImGui::PushStyleColor(ImGuiCol_Border, Gleam::Theme::Accent);
	ImGui::PushStyleColor(ImGuiCol_Text, Gleam::Theme::OnAccent);
	const bool pressed = ImGui::Button(label, size);
	ImGui::PopStyleColor(5);
	return pressed;
}

bool Widgets::OutlineButton(const char* label, const ImVec2& size)
{
	ImGui::PushStyleColor(ImGuiCol_Button, Gleam::Theme::Hex(0x000000, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, Gleam::Theme::BorderStrong);
	ImGui::PushStyleColor(ImGuiCol_Text, Gleam::Theme::TextSecondary);
	const bool pressed = ImGui::Button(label, size);
	ImGui::PopStyleColor(3);
	return pressed;
}

EntityKind Widgets::GetEntityKind(const Gleam::EntityManager& entityManager, Gleam::EntityHandle handle)
{
	static const auto entityHash = Gleam::Reflection::GetClass<Gleam::Entity>().TypeHash();
	static const auto transformHash = Gleam::Reflection::GetClass<Gleam::Transform>().TypeHash();

	EntityKind kind{ .name = {}, .color = Gleam::Theme::TextDim };
	bool found = false;
	entityManager.Visit(handle, [&](const void* component, const Gleam::Reflection::ClassDescription& classDesc)
	{
		const auto typeHash = classDesc.TypeHash();
		if (found or typeHash == entityHash or typeHash == transformHash)
		{
			return;
		}

		if (classDesc.HasAttribute<Gleam::Reflection::Attribute::EntityComponent>() and classDesc.HasAttribute<Gleam::Reflection::Attribute::EditorOnly>() == false)
		{
			kind.name = ReflectionUtils::ResolveDisplayName(classDesc);
			kind.color = GetComponentColor(typeHash);
			found = true;
		}
	});
	return kind;
}
