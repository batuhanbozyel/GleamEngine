#include "InputActionsEditor.h"
#include "EAssets/EAssetManager.h"
#include "View/ViewStack.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"
#include "View/Widgets/EditorWidgets.h"
#include "View/Widgets/PropertyDrawer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"

#include "Input/InputSystem.h"
#include "Serialization/ReflectionUtils.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <Runtime.Reflection.generated.h>

#include <imgui.h>

#include <algorithm>
#include <cstring>

using namespace GEditor;

static constexpr float kColumnWidth = 110.0f;
static constexpr float kListWidth = 200.0f;
static constexpr float kDeviceComboWidth = 100.0f;
static constexpr float kListenButtonWidth = 80.0f;
static constexpr ImVec2 kPanelPadding = ImVec2(8.0f, 8.0f);

template<typename Container>
static Gleam::TString MakeUniqueName(const Container& items, const char* base)
{
	auto name = Gleam::TString(base);
	for (uint32_t suffix = 1; std::any_of(items.begin(), items.end(), [&name](const auto& item) { return item.name == name; }); ++suffix)
	{
		name = Gleam::TString(base);
		name.append(" ");
		name.append(std::to_string(suffix).c_str());
	}
	return name;
}

template<size_t N>
static const char* ToCString(Gleam::TStringView text, char (&buffer)[N])
{
	const auto length = Gleam::Math::Min(text.size(), N - 1);
	std::memcpy(buffer, text.data(), length);
	buffer[length] = '\0';
	return buffer;
}

static const char* DisplayName(const Gleam::TString& name)
{
	return name.empty() ? "<unnamed>" : name.c_str();
}

InputActionsEditor::InputActionsEditor(EAssetManager* assetManager)
	: mAssetManager(assetManager)
{

}

void InputActionsEditor::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
}

void InputActionsEditor::Open(const AssetItem& asset, const Gleam::Path& file)
{
	if (mAssetManager->LoadAsset<Gleam::InputActionAssetDescriptor>(file, mDescriptor) == false)
	{
		return;
	}

	mName = asset.name;
	mAsset = asset;
	mFile = file;
	mEditingName = false;
	mDirty = false;
	mIsOpen = true;
	SelectMap(mDescriptor.maps.empty() ? -1 : 0);
}

void InputActionsEditor::OnAssetRenamed(const AssetItem& asset)
{
	if (mIsOpen and mAsset.reference == asset.reference)
	{
		mAsset = asset;
		mName = asset.name;
	}
}

void InputActionsEditor::OnAssetRemoved(const Gleam::Guid& guid)
{
	if (mIsOpen and mAsset.reference.guid == guid)
	{
		StopListening();
		mIsOpen = false;
		mDirty = false;
	}
}

void InputActionsEditor::Render(Gleam::ImGuiRenderer* imgui)
{
	if (mIsOpen == false)
	{
		return;
	}

	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		UpdateListening();

		ImGui::SetNextWindowSize(ImVec2(960.0f, 560.0f), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Input Actions###InputActionsEditor", &mIsOpen))
		{
			DrawToolbar();

			if (ImGui::BeginTable("##InputActionsLayout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
			{
				ImGui::TableSetupColumn("Maps", ImGuiTableColumnFlags_WidthFixed, kListWidth);
				ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, kListWidth);
				ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, kPanelPadding);

				ImGui::TableSetColumnIndex(0);
				ImGui::BeginChild("##Maps", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
				DrawMaps();
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(1);
				ImGui::BeginChild("##Actions", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
				DrawActions();
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(2);
				ImGui::BeginChild("##Properties", ImVec2(0.0f, 0.0f), ImGuiChildFlags_AlwaysUseWindowPadding);
				DrawProperties();
				ImGui::EndChild();

				ImGui::PopStyleVar();
				ImGui::EndTable();
			}
		}
		ImGui::End();

		if (mIsOpen == false)
		{
			StopListening();
		}
	});
}

void InputActionsEditor::Save()
{
	auto item = mAsset;
	item.name = mName;
	if (mAssetManager->SaveAsset(mFile.Parent(), mDescriptor, item))
	{
		mAsset = item;
		mDirty = false;
	}
}

void InputActionsEditor::DrawToolbar()
{
	if (mEditingName == false)
	{
		const auto length = Gleam::Math::Min(mName.size(), sizeof(mNameBuffer) - 1);
		std::memcpy(mNameBuffer, mName.c_str(), length);
		mNameBuffer[length] = '\0';
	}

	constexpr float kNameWidth = 280.0f;
	ImGui::PushFont(mFonts->semiBold);
	ImGui::SetNextItemWidth(kNameWidth);
	ImGui::InputText("##AssetName", mNameBuffer, sizeof(mNameBuffer), ImGuiInputTextFlags_AutoSelectAll);
	ImGui::PopFont();
	mEditingName = ImGui::IsItemActive();
	if (ImGui::IsItemDeactivatedAfterEdit() and mName != Gleam::TStringView(mNameBuffer))
	{
		mName = Gleam::TString(static_cast<const char*>(mNameBuffer));
		mDirty = true;
	}

	const bool validName = mAssetManager->IsValidAssetName(mFile.Parent(), mAsset, mEditingName ? Gleam::TStringView(mNameBuffer) : Gleam::TStringView(mName));
	if (validName == false)
	{
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextColored(Gleam::Theme::Error, "Name is empty, contains a path separator or is already taken");
	}

	if (mDirty)
	{
		ImGui::SameLine();
		ImGui::TextColored(Gleam::Theme::TextDim, "(modified)");
	}

	constexpr float kSaveWidth = 90.0f;
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - kSaveWidth);
	ImGui::BeginDisabled(mDirty == false or validName == false);
	if (Widgets::AccentButton(ICON_LC_SAVE "  Save", ImVec2(kSaveWidth, ImGui::GetFrameHeight())))
	{
		Save();
	}
	ImGui::EndDisabled();
	ImGui::Separator();
}

bool InputActionsEditor::DrawListHeader(const char* label)
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextColored(Gleam::Theme::TextDim, "%s", label);

	const float buttonSize = ImGui::GetFrameHeight();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonSize);
	ImGui::PushID(label);
	const bool clicked = Widgets::IconButton("##Add", ICON_LC_PLUS, ImVec2(buttonSize, buttonSize));
	ImGui::PopID();
	ImGui::Separator();
	return clicked;
}

void InputActionsEditor::DrawMaps()
{
	if (DrawListHeader("Action Maps"))
	{
		Gleam::InputActionMapDescriptor map;
		map.id = Gleam::Guid::NewGuid();
		map.name = MakeUniqueName(mDescriptor.maps, "New Map");
		mDescriptor.maps.push_back(map);
		SelectMap(static_cast<int32_t>(mDescriptor.maps.size()) - 1);
		mDirty = true;
	}

	int32_t removed = -1;
	for (int32_t i = 0; i < static_cast<int32_t>(mDescriptor.maps.size()); ++i)
	{
		ImGui::PushID(i);
		if (ImGui::Selectable(DisplayName(mDescriptor.maps[i].name), mSelectedMap == i))
		{
			SelectMap(i);
		}
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				removed = i;
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	if (removed >= 0)
	{
		mDescriptor.maps.erase(mDescriptor.maps.begin() + removed);
		SelectMap(mDescriptor.maps.empty() ? -1 : Gleam::Math::Min(removed, static_cast<int32_t>(mDescriptor.maps.size()) - 1));
		mDirty = true;
	}
}

void InputActionsEditor::DrawActions()
{
	auto map = GetSelectedMap();
	if (map == nullptr)
	{
		ImGui::TextDisabled("No action map selected");
		return;
	}

	if (DrawListHeader("Actions"))
	{
		Gleam::InputActionDescriptor action;
		action.id = Gleam::Guid::NewGuid();
		action.name = MakeUniqueName(map->actions, "New Action");
		map->actions.push_back(action);
		SelectAction(static_cast<int32_t>(map->actions.size()) - 1);
		mDirty = true;
	}

	int32_t removed = -1;
	for (int32_t i = 0; i < static_cast<int32_t>(map->actions.size()); ++i)
	{
		ImGui::PushID(i);
		if (ImGui::Selectable(DisplayName(map->actions[i].name), mSelectedAction == i))
		{
			SelectAction(i);
		}
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				removed = i;
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	if (removed >= 0)
	{
		map->actions.erase(map->actions.begin() + removed);
		SelectAction(map->actions.empty() ? -1 : Gleam::Math::Min(removed, static_cast<int32_t>(map->actions.size()) - 1));
		mDirty = true;
	}
}

void InputActionsEditor::DrawProperties()
{
	PropertyDrawer::BeginEditTracking();

	if (auto action = GetSelectedAction())
	{
		DrawActionProperties(*action);
	}
	else if (auto map = GetSelectedMap())
	{
		ImGui::SeparatorText("Action Map");
		PropertyDrawer::DrawStringControl("Name", map->name, kColumnWidth);
		PropertyDrawer::DrawScalarControl("Priority", map->priority, 0, kColumnWidth);
	}
	else
	{
		ImGui::TextDisabled("Select an action map or an action");
	}

	if (PropertyDrawer::EditDirty() or PropertyDrawer::EditCommitted())
	{
		mDirty = true;
	}
}

void InputActionsEditor::DrawActionProperties(Gleam::InputActionDescriptor& action)
{
	ImGui::SeparatorText("Action");
	PropertyDrawer::DrawStringControl("Name", action.name, kColumnWidth);
	PropertyDrawer::DrawEnumOptions("Value Type", Gleam::Reflection::GetEnum<Gleam::InputValueType>(), &action.valueType, kColumnWidth);

	ImGui::SeparatorText("Bindings");
	int32_t removed = -1;
	for (uint32_t i = 0; i < action.bindings.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));
		if (DrawBinding(action.bindings[i], i) == false)
		{
			removed = static_cast<int32_t>(i);
		}
		ImGui::PopID();
	}

	if (removed >= 0)
	{
		action.bindings.erase(action.bindings.begin() + removed);
		StopListening();
		mDirty = true;
	}

	if (ImGui::Button(ICON_LC_PLUS "  Add Binding"))
	{
		Gleam::InputBindingDescriptor binding;
		binding.controls.push_back(Gleam::InputControl{});
		action.bindings.push_back(binding);
		mDirty = true;
	}

	ImGui::SeparatorText("Processors");
	DrawProcessors(action.processors);

	ImGui::SeparatorText("Interactions");
	DrawInteractions(action.interactions);
}

bool InputActionsEditor::DrawBinding(Gleam::InputBindingDescriptor& binding, uint32_t index)
{
	const auto summary = binding.GetDisplayName();
	const bool open = ImGui::TreeNodeEx("##Binding", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap, "%s", summary.c_str());

	const float buttonSize = ImGui::GetFrameHeight();
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonSize);
	const bool keep = Widgets::IconButton("##Remove", ICON_LC_TRASH_2, ImVec2(buttonSize, buttonSize)) == false;

	if (open)
	{
		const auto previousType = binding.type;
		PropertyDrawer::DrawEnumOptions("Type", Gleam::Reflection::GetEnum<Gleam::InputBindingType>(), &binding.type, kColumnWidth);
		if (previousType != binding.type)
		{
			StopListening();
		}

		const uint32_t partCount = Gleam::InputBindingDescriptor::GetPartCount(binding.type);
		if (binding.controls.size() != partCount)
		{
			binding.controls.resize(partCount);
		}

		for (uint32_t part = 0; part < partCount; ++part)
		{
			const auto label = Gleam::InputBindingDescriptor::GetPartName(binding.type, part);
			DrawControl(label.data(), binding.controls[part], index, part);
		}

		PropertyDrawer::DrawEnumFlagOptions("Modifiers", Gleam::Reflection::GetClass<Gleam::EnumFlag<Gleam::ModifierKeys>>(), &binding.modifiers, kColumnWidth);

		PropertyDrawer::DrawScalarControl("Match Layout", binding.layoutMapped, false, kColumnWidth);

		if (ImGui::TreeNode("Processors"))
		{
			DrawProcessors(binding.processors);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Interactions"))
		{
			DrawInteractions(binding.interactions);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
	return keep;
}

void InputActionsEditor::DrawControl(const char* label, Gleam::InputControl& control, uint32_t binding, uint32_t part)
{
	ImGui::PushID(static_cast<int>(part));
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, kColumnWidth);
	PropertyDrawer::DrawFieldLabel(label);
	ImGui::NextColumn();

	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float available = ImGui::GetContentRegionAvail().x;

	const auto& deviceEnum = Gleam::Reflection::GetEnum<Gleam::InputDeviceType>();
	const auto currentDevice = Gleam::ReflectionUtils::FindCase(deviceEnum, static_cast<int64_t>(control.device));
	char devicePreview[64];
	ToCString(currentDevice ? Gleam::ReflectionUtils::ResolveDisplayName(*currentDevice) : Gleam::TStringView("-"), devicePreview);
	ImGui::SetNextItemWidth(kDeviceComboWidth);
	if (ImGui::BeginCombo("##Device", devicePreview))
	{
		for (const auto& deviceCase : deviceEnum.Cases())
		{
			const auto device = static_cast<Gleam::InputDeviceType>(deviceCase.Value());
			char name[64];
			if (ImGui::Selectable(ToCString(Gleam::ReflectionUtils::ResolveDisplayName(deviceCase), name), device == control.device) and device != control.device)
			{
				control.device = device;
				control.code = Gleam::InputControl::GetCodes(device)[0];
				mDirty = true;
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(Gleam::Math::Max(available - kDeviceComboWidth - kListenButtonWidth - spacing * 2.0f, 60.0f));
	char preview[64];
	if (ImGui::BeginCombo("##Control", ToCString(control.GetName(), preview)))
	{
		for (const auto code : Gleam::InputControl::GetCodes(control.device))
		{
			const auto candidate = Gleam::InputControl{ .device = control.device, .code = code };
			const bool selected = candidate == control;
			char name[64];
			if (ImGui::Selectable(ToCString(candidate.GetName(), name), selected))
			{
				control = candidate;
				mDirty = true;
			}
			if (selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();
	const bool listening = mListening.binding == static_cast<int32_t>(binding) and mListening.part == part;
	if (ImGui::Button(listening ? "Press..." : "Listen", ImVec2(kListenButtonWidth, 0.0f)))
	{
		if (listening)
		{
			StopListening();
		}
		else
		{
			mListening = ListenTarget{ .binding = static_cast<int32_t>(binding), .part = part, .armed = false };
		}
	}

	ImGui::Columns(1);
	ImGui::PopID();
}

void InputActionsEditor::DrawProcessors(Gleam::TArray<Gleam::InputProcessor>& processors)
{
	static const Gleam::InputProcessor kDefaults{};
	int32_t removed = -1;
	for (uint32_t i = 0; i < processors.size(); ++i)
	{
		auto& processor = processors[i];
		ImGui::PushID(static_cast<int>(i));
		PropertyDrawer::DrawEnumOptions("Processor", Gleam::Reflection::GetEnum<Gleam::InputProcessorType>(), &processor.type, kColumnWidth);
		switch (processor.type)
		{
			case Gleam::InputProcessorType::DeadZone:
			{
				PropertyDrawer::DrawScalarControl("Min", processor.min, kDefaults.min, kColumnWidth);
				PropertyDrawer::DrawScalarControl("Max", processor.max, kDefaults.max, kColumnWidth);
				break;
			}
			case Gleam::InputProcessorType::Clamp:
			{
				PropertyDrawer::DrawScalarControl("Min", processor.min, -1.0f, kColumnWidth);
				PropertyDrawer::DrawScalarControl("Max", processor.max, 1.0f, kColumnWidth);
				break;
			}
			case Gleam::InputProcessorType::Scale:
			{
				PropertyDrawer::DrawVec3Control("Factor", processor.factor, kDefaults.factor.x, kColumnWidth);
				break;
			}
			default: break;
		}

		if (ImGui::SmallButton(ICON_LC_TRASH_2 "  Remove"))
		{
			removed = static_cast<int32_t>(i);
		}
		ImGui::Separator();
		ImGui::PopID();
	}

	if (removed >= 0)
	{
		processors.erase(processors.begin() + removed);
		mDirty = true;
	}

	if (ImGui::SmallButton(ICON_LC_PLUS "  Add Processor"))
	{
		processors.push_back(Gleam::InputProcessor{});
		mDirty = true;
	}
}

void InputActionsEditor::DrawInteractions(Gleam::TArray<Gleam::InputInteraction>& interactions)
{
	static const Gleam::InputInteraction kDefaults{};
	int32_t removed = -1;
	for (uint32_t i = 0; i < interactions.size(); ++i)
	{
		auto& interaction = interactions[i];
		ImGui::PushID(static_cast<int>(i));
		PropertyDrawer::DrawEnumOptions("Interaction", Gleam::Reflection::GetEnum<Gleam::InputInteractionType>(), &interaction.type, kColumnWidth);
		PropertyDrawer::DrawScalarControl("Press Point", interaction.pressPoint, kDefaults.pressPoint, kColumnWidth);
		if (interaction.type == Gleam::InputInteractionType::Hold or interaction.type == Gleam::InputInteractionType::Tap or interaction.type == Gleam::InputInteractionType::MultiTap)
		{
			PropertyDrawer::DrawScalarControl("Duration", interaction.duration, kDefaults.duration, kColumnWidth);
		}
		if (interaction.type == Gleam::InputInteractionType::MultiTap)
		{
			PropertyDrawer::DrawScalarControl("Tap Count", interaction.tapCount, kDefaults.tapCount, kColumnWidth);
		}

		if (ImGui::SmallButton(ICON_LC_TRASH_2 "  Remove"))
		{
			removed = static_cast<int32_t>(i);
		}
		ImGui::Separator();
		ImGui::PopID();
	}

	if (removed >= 0)
	{
		interactions.erase(interactions.begin() + removed);
		mDirty = true;
	}

	if (ImGui::SmallButton(ICON_LC_PLUS "  Add Interaction"))
	{
		interactions.push_back(Gleam::InputInteraction{});
		mDirty = true;
	}
}

void InputActionsEditor::UpdateListening()
{
	if (mListening.binding < 0)
	{
		return;
	}

	auto action = GetSelectedAction();
	if (action == nullptr or mListening.binding >= static_cast<int32_t>(action->bindings.size()))
	{
		StopListening();
		return;
	}
	else if (mListening.armed == false)
	{
		mListening.armed = true;
		return;
	}

	auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	Gleam::InputControl control;
	if (Gleam::InputSystem::FindActuatedControl(inputSystem->GetDevices(), control))
	{
		auto& controls = action->bindings[mListening.binding].controls;
		if (control != Gleam::InputControl::Make(Gleam::Key::Escape) and mListening.part < controls.size())
		{
			controls[mListening.part] = control;
			mDirty = true;
		}
		StopListening();
	}
}

void InputActionsEditor::StopListening()
{
	mListening = ListenTarget();
}

Gleam::InputActionMapDescriptor* InputActionsEditor::GetSelectedMap()
{
	if (mSelectedMap >= 0 and mSelectedMap < static_cast<int32_t>(mDescriptor.maps.size()))
	{
		return &mDescriptor.maps[mSelectedMap];
	}
	return nullptr;
}

Gleam::InputActionDescriptor* InputActionsEditor::GetSelectedAction()
{
	auto map = GetSelectedMap();
	if (map and mSelectedAction >= 0 and mSelectedAction < static_cast<int32_t>(map->actions.size()))
	{
		return &map->actions[mSelectedAction];
	}
	return nullptr;
}

void InputActionsEditor::SelectMap(int32_t index)
{
	mSelectedMap = index;
	auto map = GetSelectedMap();
	mSelectedAction = (map and map->actions.empty() == false) ? 0 : -1;
	StopListening();
}

void InputActionsEditor::SelectAction(int32_t index)
{
	mSelectedAction = index;
	StopListening();
}
