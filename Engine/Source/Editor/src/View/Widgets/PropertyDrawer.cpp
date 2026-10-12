#include "PropertyDrawer.h"
#include "AssetIcon.h"
#include "EditorWidgets.h"
#include "View/EditorFonts.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"

#include "EAssets/EAssetManager.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Container/EnumFlag.h"
#include "Physics/Collider.h"
#include "Serialization/ReflectionUtils.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cctype>
#include <cstdio>
#include <Runtime.Reflection.generated.h>

using namespace GEditor;

static constexpr float kLabelColumnWidth = 96.0f;
static constexpr float kFieldHeight = 26.0f;

static void DrawFieldLabel(const char* label)
{
	ImGui::AlignTextToFramePadding();
	ImGui::TextColored(Gleam::Theme::TextSecondary, "%s", label);
}

static Gleam::TStringView QualifiedNameWithoutTemplateDeclaration(const Gleam::TStringView name)
{
	auto pos = name.find_first_of('<');
	if (pos == Gleam::TStringView::npos)
	{
		return name;
	}
	return name.substr(0, pos);
}

static size_t ArrayElementSize(const Gleam::Reflection::ArrayDescription& arrayDesc)
{
	switch (arrayDesc.ElementType())
	{
		case Gleam::Reflection::MetaType::Primitive:
			return Gleam::Reflection::GetPrimitive(arrayDesc.ElementHash()).GetSize();
		case Gleam::Reflection::MetaType::Enum:
			return Gleam::Reflection::GetEnum(arrayDesc.ElementHash())->GetSize();
		case Gleam::Reflection::MetaType::Class:
			return Gleam::Reflection::GetClass(arrayDesc.ElementHash())->GetSize();
		case Gleam::Reflection::MetaType::Array:
			return Gleam::Reflection::GetArray(arrayDesc.ElementHash())->GetSize();
		default:
			return 0;
	}
}

static bool IsMultiEditable(Gleam::Reflection::MetaType type, uint32_t typeHash);

static bool IsMultiEditableClass(const Gleam::Reflection::ClassDescription& classDesc)
{
	// TArray and TString own heap storage, mirroring them onto another instance needs a real copy
	if (classDesc.IsTemplate())
	{
		const auto enumFlagName = QualifiedNameWithoutTemplateDeclaration(Gleam::Reflection::GetClass<Gleam::EnumFlag<Gleam::EnumFlagPlaceholder>>().ResolveQualifiedName());
		return QualifiedNameWithoutTemplateDeclaration(classDesc.ResolveQualifiedName()) == enumFlagName;
	}

	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		if (IsMultiEditableClass(baseClass) == false)
		{
			return false;
		}
	}

	for (const auto& field : classDesc.ResolveFields())
	{
		if (IsMultiEditable(field.GetType(), field.TypeHash()) == false)
		{
			return false;
		}
	}
	return true;
}

static bool IsMultiEditable(Gleam::Reflection::MetaType type, uint32_t typeHash)
{
	switch (type)
	{
		case Gleam::Reflection::MetaType::Primitive:
		case Gleam::Reflection::MetaType::Enum:
			return true;
		case Gleam::Reflection::MetaType::Array:
		{
			const auto arrayDesc = Gleam::Reflection::GetArray(typeHash);
			return IsMultiEditable(arrayDesc->ElementType(), arrayDesc->ElementHash());
		}
		case Gleam::Reflection::MetaType::Class:
			return IsMultiEditableClass(*Gleam::Reflection::GetClass(typeHash));
		default:
			return false;
	}
}

static bool HasReflectedFields(const Gleam::Reflection::ClassDescription& classDesc)
{
	if (classDesc.ResolveFields().size() > 0)
	{
		return true;
	}

	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		if (HasReflectedFields(baseClass))
		{
			return true;
		}
	}
	return false;
}

static bool AreValuesEqual(Gleam::Reflection::MetaType type, uint32_t typeHash, size_t size, const void* lhs, const void* rhs);

static bool AreClassValuesEqual(const Gleam::Reflection::ClassDescription& classDesc, const void* lhs, const void* rhs)
{
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		if (AreClassValuesEqual(baseClass, lhs, rhs) == false)
		{
			return false;
		}
	}

	for (const auto& field : classDesc.ResolveFields())
	{
		if (AreValuesEqual(field.GetType(), field.TypeHash(), field.GetSize(),
						   Gleam::OffsetPointer(lhs, field.GetOffset()),
						   Gleam::OffsetPointer(rhs, field.GetOffset())) == false)
		{
			return false;
		}
	}
	return true;
}

static bool AreValuesEqual(Gleam::Reflection::MetaType type, uint32_t typeHash, size_t size, const void* lhs, const void* rhs)
{
	// Walking the reflected fields keeps struct padding out of the comparison, types whose
	// storage reflection does not describe fall back to a raw compare of the whole value
	if (type == Gleam::Reflection::MetaType::Class)
	{
		const auto classDesc = Gleam::Reflection::GetClass(typeHash);
		if (HasReflectedFields(*classDesc))
		{
			return AreClassValuesEqual(*classDesc, lhs, rhs);
		}
	}
	return std::memcmp(lhs, rhs, size) == 0;
}

static bool IsFieldMixed(const Gleam::Reflection::FieldDescription& field, Gleam::TArrayView<void*> instances)
{
	const auto lhs = Gleam::OffsetPointer(instances[0], field.GetOffset());
	for (size_t i = 1; i < instances.size(); ++i)
	{
		const auto rhs = Gleam::OffsetPointer(instances[i], field.GetOffset());
		if (AreValuesEqual(field.GetType(), field.TypeHash(), field.GetSize(), lhs, rhs) == false)
		{
			return true;
		}
	}
	return false;
}

static bool IsMixedValue()
{
	return (GImGui->CurrentItemFlags & ImGuiItemFlags_MixedValue) != 0;
}

// Deduced from the container, TArray is an alias template and would not deduce the element type
template<typename Colliders>
static void DrawColliders(const char* prefix, Colliders& colliders, float columnWidth)
{
	using ColliderType = typename Colliders::value_type;

	for (uint32_t index = 0; index < colliders.size(); ++index)
	{
		char elementLabel[32];
		snprintf(elementLabel, sizeof(elementLabel), "%s %u", prefix, index);

		PropertyDrawer::DrawClass(elementLabel, &colliders[index], Gleam::Reflection::GetClass<ColliderType>(), columnWidth, [&colliders, index]()
		{
			const float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			if (PropertyDrawer::DrawSettingsButton(lineHeight))
			{
				ImGui::OpenPopup("ColliderSettings");
			}

			if (ImGui::BeginPopup("ColliderSettings"))
			{
				if (ImGui::MenuItem("Remove Collider"))
				{
					PropertyDrawer::QueueEdit([&colliders, index]()
					{
						colliders.erase(colliders.begin() + index);
					});
				}
				ImGui::EndPopup();
			}
		});
	}
}

static void DrawColliderSet(const Gleam::TStringView label, Gleam::ColliderSet& colliders, float columnWidth)
{
	struct ColliderOption
	{
		const char* name;
		void (*add)(Gleam::ColliderSet& colliders);
	};

	static const ColliderOption options[] =
	{
		{ "Box", [](Gleam::ColliderSet& set) { set.boxes.emplace_back(); } },
		{ "Sphere", [](Gleam::ColliderSet& set) { set.spheres.emplace_back(); } },
		{ "Capsule", [](Gleam::ColliderSet& set) { set.capsules.emplace_back(); } },
		{ "Convex Mesh", [](Gleam::ColliderSet& set) { set.convexMeshes.emplace_back(); } },
		{ "Triangle Mesh", [](Gleam::ColliderSet& set) { set.triangleMeshes.emplace_back(); } },
	};

	PropertyDrawer::DrawCustom(label, Gleam::Reflection::GetClass<Gleam::ColliderSet>().TypeHash(), [&colliders, columnWidth]()
	{
		DrawColliders("Box", colliders.boxes, columnWidth);
		DrawColliders("Sphere", colliders.spheres, columnWidth);
		DrawColliders("Capsule", colliders.capsules, columnWidth);
		DrawColliders("Convex Mesh", colliders.convexMeshes, columnWidth);
		DrawColliders("Triangle Mesh", colliders.triangleMeshes, columnWidth);

		if (ImGui::Button("Add Collider", ImVec2(-1.0f, 0.0f)))
		{
			ImGui::OpenPopup("AddCollider");
		}

		if (ImGui::BeginPopup("AddCollider"))
		{
			for (const auto& option : options)
			{
				if (ImGui::MenuItem(option.name))
				{
					PropertyDrawer::QueueEdit([&colliders, add = option.add]()
					{
						add(colliders);
					});
				}
			}
			ImGui::EndPopup();
		}
	});
}

const Gleam::HashMap<Gleam::TStringView, PropertyDrawer::DrawFunction>& PropertyDrawer::GetCustomDrawers()
{
	static const auto drawers = []()
	{
		Gleam::HashMap<Gleam::TStringView, DrawFunction> customDrawers;

		customDrawers[Gleam::Reflection::GetClass<Gleam::Color>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawColorControl(label, Gleam::Reflection::Get<Gleam::Color>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::Float3>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawVec3Control(label, Gleam::Reflection::Get<Gleam::Float3>(obj), 0.0f, columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::AssetReference>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawAsset(label, Gleam::Reflection::Get<Gleam::AssetReference>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::TString>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawStringControl(label, Gleam::Reflection::Get<Gleam::TString>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::Guid>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawTextControl(label, Gleam::Reflection::Get<Gleam::Guid>(obj).ToString(), columnWidth);
		};

		const auto enumFlagName = QualifiedNameWithoutTemplateDeclaration(Gleam::Reflection::GetClass<Gleam::EnumFlag<Gleam::EnumFlagPlaceholder>>().ResolveQualifiedName());
		customDrawers[enumFlagName] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawEnumFlagOptions(label, classDesc, obj, columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::Quaternion>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawRotationControl(label, Gleam::Reflection::Get<Gleam::Quaternion>(obj), columnWidth);
		};

		customDrawers[Gleam::Reflection::GetClass<Gleam::ColliderSet>().ResolveQualifiedName()] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			DrawColliderSet(label, Gleam::Reflection::Get<Gleam::ColliderSet>(obj), columnWidth);
		};

		const auto arrayName = QualifiedNameWithoutTemplateDeclaration(Gleam::Reflection::GetClass<Gleam::TArray<uint8_t>>().ResolveQualifiedName());
		customDrawers[arrayName] =
			[](const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
		{
			auto templateParams = classDesc.ResolveTemplateParameters();
			GLEAM_ASSERT(templateParams.size() == 1, "PropertyDrawer: TArray must have exactly one template parameter for element type.");

			const auto& element = templateParams[0];
			auto& arr = Gleam::Reflection::Get<Gleam::TArray<uint8_t>>(obj);
			auto arrayDesc = Gleam::Reflection::ArrayDescription(element.GetType(), element.TypeHash(), arr.size());
			DrawArray(label, arr.data(), arrayDesc, columnWidth);
		};

		return customDrawers;
	}();
	return drawers;
}

// Instantiations are keyed by their own name, the stripped name only matches template wide drawers
template<typename DrawerMap>
static auto FindCustomDrawer(const Gleam::Reflection::ClassDescription& classDesc, const DrawerMap& drawers)
{
	const auto resolved = classDesc.ResolveQualifiedName();
	const Gleam::TStringView qualifiedName(resolved.data(), resolved.size());

	auto it = drawers.find(qualifiedName);
	if (it != drawers.end())
	{
		return it;
	}
	return drawers.find(QualifiedNameWithoutTemplateDeclaration(qualifiedName));
}

bool PropertyDrawer::HasCustomDrawer(const Gleam::Reflection::ClassDescription& classDesc)
{
	const auto& drawers = GetCustomDrawers();
	return FindCustomDrawer(classDesc, drawers) != drawers.end();
}

bool PropertyDrawer::TryCustomDrawer(const Gleam::TStringView label, void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	const auto& drawers = GetCustomDrawers();
	auto it = FindCustomDrawer(classDesc, drawers);
	if (it != drawers.end())
	{
		it->second(label, obj, classDesc, columnWidth);
		return true;
	}
	return false;
}

void PropertyDrawer::BeginEditTracking()
{
	mEditStarted = false;
	mEditCommitted = false;
	mEditDirty = false;
	mPendingEdit = nullptr;

	// Controls that went off screen keep their euler cache for a frame, then it is dropped
	const int32_t frame = ImGui::GetFrameCount();
	for (auto it = mRotationCache.begin(); it != mRotationCache.end();)
	{
		if (it->second.frame < (frame - 1))
		{
			it = mRotationCache.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void PropertyDrawer::QueueEdit(UIFunction&& edit)
{
	mPendingEdit = std::move(edit);

	// Marks the owning component for the change tracker, the edit itself lands after the draw
	mEditDirty = true;
}

bool PropertyDrawer::HasPendingEdit()
{
	return mPendingEdit != nullptr;
}

void PropertyDrawer::ApplyPendingEdit()
{
	const auto edit = std::move(mPendingEdit);
	mPendingEdit = nullptr;

	if (edit)
	{
		edit();
	}
}

void PropertyDrawer::SetFonts(const EditorFonts* fonts)
{
	mFonts = fonts;
}

bool PropertyDrawer::DrawSettingsButton(float size)
{
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + size, min.y + size);
	const bool pressed = ImGui::InvisibleButton("##Settings", ImVec2(size, size));
	const bool hovered = ImGui::IsItemHovered();

	auto drawList = ImGui::GetWindowDrawList();
	if (hovered)
	{
		drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::ControlHover), 4.0f);
	}
	Widgets::DrawIcon(drawList, ICON_LC_ELLIPSIS_VERTICAL, ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f), hovered ? Gleam::Theme::Text : Gleam::Theme::TextMuted);
	return pressed;
}

bool PropertyDrawer::DrawSectionHeader(const char* label, const UIFunction& headerFunction)
{
	constexpr float kHeight = 32.0f;
	constexpr float kOptionsSize = 24.0f;

	auto window = ImGui::GetCurrentWindow();
	auto storage = ImGui::GetStateStorage();
	const ImGuiID openID = ImGui::GetID("##SectionOpen");
	bool open = storage->GetBool(openID, true);

	const ImVec2 cursor = ImGui::GetCursorScreenPos();
	const ImVec2 min(window->InnerRect.Min.x, cursor.y);
	const ImVec2 max(window->InnerRect.Max.x, cursor.y + kHeight);

	ImGui::SetCursorScreenPos(min);
	ImGui::SetNextItemAllowOverlap();
	if (ImGui::InvisibleButton("##SectionHeader", ImVec2(max.x - min.x, kHeight)))
	{
		open = not open;
		storage->SetBool(openID, open);
	}
	const bool hovered = ImGui::IsItemHovered();

	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, Widgets::ColorU32(hovered ? Gleam::Theme::Selected : Gleam::Theme::SectionHeader));
	drawList->AddLine(min, ImVec2(max.x, min.y), Widgets::ColorU32(Gleam::Theme::Divider));
	drawList->AddLine(ImVec2(min.x, max.y - 1.0f), ImVec2(max.x, max.y - 1.0f), Widgets::ColorU32(Gleam::Theme::Divider));

	const float centerY = min.y + kHeight * 0.5f;
	ImGui::PushFont(mFonts->microBold);
	Widgets::DrawIcon(drawList, open ? ICON_LC_CHEVRON_DOWN : ICON_LC_CHEVRON_RIGHT, ImVec2(cursor.x + 5.0f, centerY), Gleam::Theme::Text);
	ImGui::PopFont();

	ImGui::PushFont(mFonts->semiBold);
	drawList->AddText(ImVec2(cursor.x + 18.0f, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::Text), label);
	ImGui::PopFont();

	if (headerFunction)
	{
		ImGui::SetCursorScreenPos(ImVec2(max.x - 8.0f - kOptionsSize, min.y + (kHeight - kOptionsSize) * 0.5f));
		headerFunction();
	}

	ImGui::SetCursorScreenPos(min);
	ImGui::Dummy(ImVec2(max.x - min.x, kHeight));
	return open;
}

void PropertyDrawer::DrawAxisField(const char* axis, uint32_t color, float& value, float resetValue, const char* format, float width)
{
	constexpr float kTagWidth = 18.0f;

	ImGui::PushID(axis);
	const ImVec2 min = ImGui::GetCursorScreenPos();
	const ImVec2 max(min.x + width, min.y + kFieldHeight);
	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Background), 4.0f);

	if (ImGui::InvisibleButton("##Reset", ImVec2(kTagWidth, kFieldHeight)))
	{
		value = resetValue;
		MarkEditCommitted();
	}
	TrackEdit();
	ImGui::SetItemTooltip("Reset");

	drawList->AddRectFilled(min, ImVec2(min.x + kTagWidth, max.y), color, 4.0f, ImDrawFlags_RoundCornersLeft);
	ImGui::PushFont(mFonts->microBold);
	Widgets::DrawIcon(drawList, axis, ImVec2(min.x + kTagWidth * 0.5f, min.y + kFieldHeight * 0.5f), Gleam::Theme::Background);
	ImGui::PopFont();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::PushFont(mFonts->mono);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, (kFieldHeight - ImGui::GetFontSize()) * 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, Gleam::Theme::Hex(0x000000, 0.0f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Gleam::Theme::Control);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Gleam::Theme::ControlHover);
	ImGui::SetNextItemWidth(width - kTagWidth);
	ImGui::DragFloat("##Value", &value, 0.05f, 0.0f, 0.0f, format);
	TrackEdit();
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar(3);
	ImGui::PopFont();

	drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::Border), 4.0f);
	ImGui::PopID();
}

bool PropertyDrawer::EditStarted()
{
	return mEditStarted;
}

bool PropertyDrawer::EditCommitted()
{
	return mEditCommitted;
}

bool PropertyDrawer::EditDirty()
{
	return mEditDirty;
}

void PropertyDrawer::TrackEdit()
{
	mEditStarted |= ImGui::IsItemActivated();
	mEditCommitted |= ImGui::IsItemDeactivatedAfterEdit();
	mEditDirty |= ImGui::IsItemEdited();
}

void PropertyDrawer::MarkEditCommitted()
{
	mEditCommitted = true;
	mEditDirty = true;
}

void PropertyDrawer::DrawScalarControl(const Gleam::TStringView label, const Gleam::Reflection::PrimitiveType type, size_t size, void* value, const void* defaultValue, float columnWidth)
{
	GLEAM_ASSERT(value, "Value can not be null.");

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });


	// Checkbox draws its own dash for a mixed value, the drags need a format without a conversion
	const char* mixedFormat = IsMixedValue() ? "-" : nullptr;

	ImGui::PushFont(mFonts->mono);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, (kFieldHeight - ImGui::GetFontSize()) * 0.5f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, Gleam::Theme::Border);
	switch (type)
	{
		case Gleam::Reflection::PrimitiveType::Bool:
		{
			if (ImGui::Checkbox("##X", static_cast<bool*>(value)))
			{
				MarkEditCommitted();
			}
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int8:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S8, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt8:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U8, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int16:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S16, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt16:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U16, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int32:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S32, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt32:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U32, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Int64:
		{
			ImGui::DragScalar("##X", ImGuiDataType_S64, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::UInt64:
		{
			ImGui::DragScalar("##X", ImGuiDataType_U64, value, 1.0f, nullptr, nullptr, mixedFormat);
			break;
		}
		case Gleam::Reflection::PrimitiveType::Float:
		{
			// TODO: use Range attribute from reflection
			constexpr float min = 0.0f;
			constexpr float max = 0.0f;
			ImGui::DragScalar("##X", ImGuiDataType_Float, value, 0.05f, &min, &max, mixedFormat ? mixedFormat : "%.2f");
			break;
		}
		case Gleam::Reflection::PrimitiveType::Double:
		{
			// TODO: use Range attribute from reflection
			constexpr double min = 0.0;
			constexpr double max = 0.0;
			ImGui::DragScalar("##X", ImGuiDataType_Double, value, 0.05f, &min, &max, mixedFormat ? mixedFormat : "%.2f");
			break;
		}
		default:
			break;
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
	ImGui::PopFont();
	TrackEdit();

	if (type != Gleam::Reflection::PrimitiveType::Bool and defaultValue and ImGui::BeginPopupContextItem("##Reset"))
	{
		if (ImGui::MenuItem("Reset"))
		{
			memcpy(value, defaultValue, size);
			MarkEditCommitted();
		}
		ImGui::EndPopup();
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawVec3Control(const Gleam::TStringView label, Gleam::Float3& values, float resetValue, float columnWidth)
{
	constexpr float kFieldGap = 4.0f;

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	const char* format = IsMixedValue() ? "-" : "%.2f";
	const float fieldWidth = (ImGui::GetContentRegionAvail().x - kFieldGap * 2.0f) / 3.0f;

	DrawAxisField("X", Widgets::ColorU32(Gleam::Theme::AxisX), values.x, resetValue, format, fieldWidth);
	ImGui::SameLine(0.0f, kFieldGap);
	DrawAxisField("Y", Widgets::ColorU32(Gleam::Theme::AxisY), values.y, resetValue, format, fieldWidth);
	ImGui::SameLine(0.0f, kFieldGap);
	DrawAxisField("Z", Widgets::ColorU32(Gleam::Theme::AxisZ), values.z, resetValue, format, fieldWidth);

	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawRotationControl(const Gleam::TStringView label, Gleam::Quaternion& rotation, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	auto& cache = mRotationCache[ImGui::GetID(buffer)];

	// Anything but this control moving the rotation has to pull the euler triple back in sync
	if (cache.frame < 0 || cache.rotation != rotation)
	{
		cache.euler = Gleam::Math::Rad2Deg(Gleam::Math::EulerAngles(rotation));
	}
	cache.frame = ImGui::GetFrameCount();

	DrawVec3Control(label, cache.euler, 0.0f, columnWidth);

	rotation = Gleam::Quaternion(Gleam::Math::Deg2Rad(cache.euler));
	cache.rotation = rotation;
}

void PropertyDrawer::DrawColorControl(const Gleam::TStringView label, Gleam::Color& color, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview;
	ImGui::ColorEdit4("##Color", &color.r, flags);
	TrackEdit();

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawStringControl(const Gleam::TStringView label, Gleam::TString& value, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	char valueBuffer[256];
	auto length = Gleam::Math::Min(value.length(), sizeof(valueBuffer) - 1);
	std::memcpy(valueBuffer, value.c_str(), length);
	valueBuffer[length] = '\0';

	if (ImGui::InputText("##X", valueBuffer, sizeof(valueBuffer)))
	{
		value.assign(valueBuffer);
	}
	TrackEdit();

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawTextControl(const Gleam::TStringView label, const Gleam::TStringView value, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	ImGui::Text("%.*s", static_cast<int>(value.size()), value.data());

	ImGui::Columns(1);

	ImGui::PopID();
}

static Gleam::TStringView ResolveCaseName(const Gleam::Reflection::EnumCaseDescription& enumCase)
{
	if (enumCase.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
	{
		return enumCase.GetAttribute<Gleam::Reflection::Attribute::PrettyName>()->name;
	}
	return enumCase.ResolveName();
}

void PropertyDrawer::DrawEnumOptions(const Gleam::TStringView label, const Gleam::Reflection::EnumDescription& enumDesc, void* value, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	char previewBuffer[64] = "-";
	if (IsMixedValue() == false)
	{
		int currentValue = *static_cast<int*>(value);
		for (const auto& item : enumDesc.Cases())
		{
			if (item.Value() == currentValue)
			{
				const auto itemLabel = ResolveCaseName(item);
				std::memcpy(previewBuffer, itemLabel.data(), itemLabel.size());
				previewBuffer[itemLabel.size()] = '\0';
				break;
			}
		}
	}

	if (ImGui::BeginCombo("##", previewBuffer))
	{
		for (const auto& item : enumDesc.Cases())
		{
			bool isSelected = *static_cast<int*>(value) == item.Value();

			char itemBuffer[64];
			const auto itemLabel = ResolveCaseName(item);
			std::memcpy(itemBuffer, itemLabel.data(), itemLabel.size());
			itemBuffer[itemLabel.size()] = '\0';

			if (ImGui::Selectable(itemBuffer, isSelected))
			{
				if (enumDesc.GetSize() == sizeof(int64_t))
				{
					*static_cast<int64_t*>(value) = item.Value();
				}
				else
				{
					*static_cast<int*>(value) = static_cast<int>(item.Value());
				}
				MarkEditCommitted();
			}
			TrackEdit();

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawEnumFlagOptions(const Gleam::TStringView label,
										 const Gleam::Reflection::ClassDescription& classDesc,
										 void* value,
										 float columnWidth)
{
	const auto enumDesc = Gleam::ReflectionUtils::ResolveFlagEnum(classDesc);
	const auto size = classDesc.GetSize();
	auto mask = Gleam::ReflectionUtils::ReadFlagMask(value, size);

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	DrawFieldLabel(buffer);
	ImGui::NextColumn();

	ImGui::PushItemWidth(-FLT_MIN);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	Gleam::TString preview = "-";
	if (IsMixedValue() == false)
	{
		preview = mask == 0 ? "None" : "";
		Gleam::ReflectionUtils::ForEachSetCase(mask, *enumDesc, [&](const Gleam::Reflection::EnumCaseDescription& enumCase)
		{
			if (preview.empty() == false)
			{
				preview += " | ";
			}
			preview += ResolveCaseName(enumCase);
		});
	}

	if (ImGui::BeginCombo("##", preview.c_str()))
	{
		for (const auto& item : enumDesc->Cases())
		{
			const auto caseMask = Gleam::ReflectionUtils::TruncateToSize(item.Value(), enumDesc->GetSize());

			char itemBuffer[64];
			const auto itemLabel = ResolveCaseName(item);
			std::memcpy(itemBuffer, itemLabel.data(), itemLabel.size());
			itemBuffer[itemLabel.size()] = '\0';

			bool isSelected = caseMask != 0 and (mask & caseMask) == caseMask;
			if (ImGui::Checkbox(itemBuffer, &isSelected))
			{
				mask = isSelected ? mask | caseMask : mask & ~caseMask;
				Gleam::ReflectionUtils::WriteFlagMask(value, size, mask);
				MarkEditCommitted();
			}
			TrackEdit();
		}
		ImGui::EndCombo();
	}

	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PopStyleVar();
	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawClassFields(void* obj, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	DrawClassFields(Gleam::TArrayView<void*>(&obj, 1), classDesc, columnWidth);
}

void PropertyDrawer::DrawClassFields(Gleam::TArrayView<void*> instances, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth)
{
	for (const auto& baseClass : classDesc.ResolveBaseClasses())
	{
		DrawClassFields(instances, baseClass, columnWidth);
	}

	Gleam::TArray<uint8_t> scratch;
	for (const auto& field : classDesc.ResolveFields())
	{
		DrawField(field, instances, columnWidth, scratch);
	}
}

void PropertyDrawer::DrawField(const Gleam::Reflection::FieldDescription& field, Gleam::TArrayView<void*> instances, float columnWidth, Gleam::TArray<uint8_t>& scratch)
{
	Gleam::TStringView fieldName;
	if (field.HasAttribute<Gleam::Reflection::Attribute::PrettyName>())
	{
		auto prettyName = field.GetAttribute<Gleam::Reflection::Attribute::PrettyName>();
		fieldName = prettyName->name;
	}
	else
	{
		fieldName = field.ResolveName();
	}

	// Nested structs recurse with every instance so the mixed state stays per leaf field
	if (field.GetType() == Gleam::Reflection::MetaType::Class)
	{
		const auto fieldDesc = Gleam::Reflection::GetClass(field.TypeHash());
		if (HasCustomDrawer(*fieldDesc) == false)
		{
			Gleam::TArray<void*> nested(instances.size());
			for (size_t i = 0; i < instances.size(); ++i)
			{
				nested[i] = Gleam::OffsetPointer(instances[i], field.GetOffset());
			}
			DrawClass(fieldName, nested, *fieldDesc, columnWidth);
			return;
		}
	}

	const bool multiEdit = instances.size() > 1;
	const bool editable = multiEdit == false || IsMultiEditable(field.GetType(), field.TypeHash());
	auto fieldPtr = Gleam::OffsetPointer(instances[0], field.GetOffset());

	if (multiEdit)
	{
		scratch.resize(field.GetSize());
		std::memcpy(scratch.data(), fieldPtr, field.GetSize());
		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, IsFieldMixed(field, instances));
		ImGui::BeginDisabled(editable == false);
	}

	switch (field.GetType())
	{
		case Gleam::Reflection::MetaType::Class:
		{
			const auto fieldDesc = Gleam::Reflection::GetClass(field.TypeHash());
			TryCustomDrawer(fieldName, fieldPtr, *fieldDesc, columnWidth);
			break;
		}
		case Gleam::Reflection::MetaType::Array:
		{
			const auto arrayDesc = Gleam::Reflection::GetArray(field.TypeHash());
			DrawArray(fieldName, fieldPtr, *arrayDesc, columnWidth);
			break;
		}
		case Gleam::Reflection::MetaType::Enum:
		{
			const auto enumDesc = Gleam::Reflection::GetEnum(field.TypeHash());
			DrawEnumOptions(fieldName, *enumDesc, fieldPtr, columnWidth);
			break;
		}
		case Gleam::Reflection::MetaType::Primitive:
		{
			constexpr uint64_t defaultValue = 0;
			const auto primitiveDesc = Gleam::Reflection::GetPrimitive(field.TypeHash());
			DrawScalarControl(fieldName, primitiveDesc.Type(), primitiveDesc.GetSize(), fieldPtr, &defaultValue, columnWidth);
			break;
		}
		default:
			break;
	}

	if (multiEdit)
	{
		ImGui::EndDisabled();
		ImGui::PopItemFlag();

		if (editable && std::memcmp(scratch.data(), fieldPtr, field.GetSize()) != 0)
		{
			for (size_t i = 1; i < instances.size(); ++i)
			{
				std::memcpy(Gleam::OffsetPointer(instances[i], field.GetOffset()), fieldPtr, field.GetSize());
			}
		}
	}
}

void PropertyDrawer::DrawClass(const Gleam::TStringView label, void* component, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth, const UIFunction& headerFunction)
{
	DrawClass(label, Gleam::TArrayView<void*>(&component, 1), classDesc, columnWidth, headerFunction);
}

void PropertyDrawer::DrawClass(const Gleam::TStringView label, Gleam::TArrayView<void*> instances, const Gleam::Reflection::ClassDescription& classDesc, float columnWidth, const UIFunction& headerFunction)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);
	if (DrawSectionHeader(buffer, headerFunction))
	{
		ImGui::Dummy(ImVec2(0.0f, 2.0f));
		DrawClassFields(instances, classDesc, columnWidth > 0.0f ? columnWidth : kLabelColumnWidth);
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}
	ImGui::PopID();
}

void PropertyDrawer::DrawArrayElements(void* obj, const Gleam::Reflection::ArrayDescription& arrayDesc, float columnWidth)
{
	size_t elementSize = ArrayElementSize(arrayDesc);
	if (elementSize == 0)
	{
		return;
	}

	static const auto assetReferenceHash = Gleam::Reflection::GetClass<Gleam::AssetReference>().TypeHash();
	if (arrayDesc.ElementType() == Gleam::Reflection::MetaType::Class and arrayDesc.ElementHash() == assetReferenceHash)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		uint32_t assetIndex = 0;
		for (size_t offset = 0; offset < arrayDesc.GetSize(); offset += elementSize, ++assetIndex)
		{
			ImGui::PushID(static_cast<int>(assetIndex));
			DrawAssetRow(assetIndex, *static_cast<Gleam::AssetReference*>(Gleam::OffsetPointer(obj, offset)));
			ImGui::PopID();
		}
		ImGui::PopStyleVar();
		return;
	}

	uint32_t index = 0;
	for (size_t offset = 0; offset < arrayDesc.GetSize(); offset += elementSize, ++index)
	{
		char elementLabel[24];
		snprintf(elementLabel, sizeof(elementLabel), "Element %u", index);

		auto element = Gleam::OffsetPointer(obj, offset);

		ImGui::PushID(static_cast<int>(index));
		switch (arrayDesc.ElementType())
		{
			case Gleam::Reflection::MetaType::Primitive:
			{
				constexpr uint64_t defaultValue = 0;
				const auto primitiveDesc = Gleam::Reflection::GetPrimitive(arrayDesc.ElementHash());
				DrawScalarControl(elementLabel, primitiveDesc.Type(), primitiveDesc.GetSize(), element, &defaultValue, columnWidth);
				break;
			}
			case Gleam::Reflection::MetaType::Enum:
			{
				const auto enumDesc = Gleam::Reflection::GetEnum(arrayDesc.ElementHash());
				DrawEnumOptions(elementLabel, *enumDesc, element, columnWidth);
				break;
			}
			case Gleam::Reflection::MetaType::Class:
			{
				const auto classDesc = Gleam::Reflection::GetClass(arrayDesc.ElementHash());
				if (TryCustomDrawer(elementLabel, element, *classDesc, columnWidth) == false)
				{
					DrawClass(elementLabel, element, *classDesc, columnWidth);
				}
				break;
			}
			case Gleam::Reflection::MetaType::Array:
			{
				const auto innerDesc = Gleam::Reflection::GetArray(arrayDesc.ElementHash());
				DrawArray(elementLabel, element, *innerDesc, columnWidth);
				break;
			}
			default:
				break;
		}
		ImGui::PopID();
	}
}

void PropertyDrawer::DrawArray(const Gleam::TStringView label, void* obj, const Gleam::Reflection::ArrayDescription& arrayDesc, float columnWidth)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	char caption[64];
	for (size_t i = 0; i <= label.size(); ++i)
	{
		caption[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(buffer[i])));
	}

	size_t elementSize = ArrayElementSize(arrayDesc);
	uint32_t elementCount = elementSize > 0 ? static_cast<uint32_t>(arrayDesc.GetSize() / elementSize) : 0u;

	Widgets::CaptionRow(*mFonts, caption, elementCount, 24.0f, 0.0f);
	DrawArrayElements(obj, arrayDesc, columnWidth > 0.0f ? columnWidth : kLabelColumnWidth);

	ImGui::PopID();
}

static const AssetItem* FindAssetItem(const Gleam::AssetReference& assetRef)
{
	if (assetRef.guid == Gleam::Guid::InvalidGuid())
	{
		return nullptr;
	}

	auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<EAssetManager>();
	return assetManager->FindAsset(assetRef.guid);
}

static void DrawAssetTile(ImDrawList* drawList, const ImVec2& min, float size, const ImVec4& color)
{
	const ImVec2 max(min.x + size, min.y + size);
	drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Control), 3.0f);
	drawList->AddRectFilled(ImVec2(min.x, max.y - 2.0f), max, Widgets::ColorU32(color), 3.0f, ImDrawFlags_RoundCornersBottom);
}

void PropertyDrawer::DrawAsset(const Gleam::TStringView label, Gleam::AssetReference& assetRef, float columnWidth)
{
	constexpr float kHeight = 34.0f;
	constexpr float kTileSize = 24.0f;

	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(buffer);

	const bool mixed = IsMixedValue();
	const AssetItem* item = FindAssetItem(assetRef);
	const auto icon = GetAssetIcon(item ? item->type : Gleam::Guid(Gleam::Guid::InvalidGuid()));

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (kHeight - ImGui::GetFontSize()) * 0.5f);
	ImGui::TextColored(Gleam::Theme::TextSecondary, "%s", buffer);
	ImGui::NextColumn();

	const ImVec2 min = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 max(min.x + width, min.y + kHeight);
	ImGui::InvisibleButton("##Asset", ImVec2(width, kHeight));
	if (assetRef.guid != Gleam::Guid::InvalidGuid() and ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", assetRef.guid.ToString().c_str());
	}

	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Background), 4.0f);
	drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::Border), 4.0f);
	DrawAssetTile(drawList, ImVec2(min.x + 5.0f, min.y + (kHeight - kTileSize) * 0.5f), kTileSize, icon.color);

	const char* name = mixed ? "-" : (item ? item->name.c_str() : "None");
	drawList->PushClipRect(min, ImVec2(max.x - 6.0f, max.y), true);
	drawList->AddText(ImVec2(min.x + 5.0f + kTileSize + 8.0f, min.y + (kHeight - ImGui::GetFontSize()) * 0.5f), Widgets::ColorU32(item ? Gleam::Theme::Text : Gleam::Theme::TextDim), name);
	drawList->PopClipRect();

	ImGui::Columns(1);

	ImGui::PopID();
}

void PropertyDrawer::DrawAssetRow(uint32_t index, Gleam::AssetReference& assetRef)
{
	constexpr float kRowHeight = 30.0f;
	constexpr float kIndexWidth = 28.0f;
	constexpr float kTileSize = 18.0f;

	const AssetItem* item = FindAssetItem(assetRef);
	const auto icon = GetAssetIcon(item ? item->type : Gleam::Guid(Gleam::Guid::InvalidGuid()));

	const ImVec2 min = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const ImVec2 max(min.x + width, min.y + kRowHeight);
	ImGui::InvisibleButton("##AssetRow", ImVec2(width, kRowHeight));
	if (assetRef.guid != Gleam::Guid::InvalidGuid() and ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", assetRef.guid.ToString().c_str());
	}

	auto drawList = ImGui::GetWindowDrawList();
	const float centerY = min.y + kRowHeight * 0.5f;

	char indexText[16];
	std::snprintf(indexText, sizeof(indexText), "%u", index);
	ImGui::PushFont(mFonts->mono);
	const float indexWidth = ImGui::CalcTextSize(indexText).x;
	drawList->AddText(ImVec2(min.x + kIndexWidth - indexWidth, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::TextDim), indexText);
	ImGui::PopFont();

	const float tileX = min.x + kIndexWidth + 8.0f;
	DrawAssetTile(drawList, ImVec2(tileX, centerY - kTileSize * 0.5f), kTileSize, icon.color);

	const char* name = IsMixedValue() ? "-" : (item ? item->name.c_str() : "None");
	drawList->PushClipRect(min, max, true);
	drawList->AddText(ImVec2(tileX + kTileSize + 8.0f, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(item ? Gleam::Theme::Text : Gleam::Theme::TextDim), name);
	drawList->PopClipRect();
	drawList->AddLine(ImVec2(min.x, max.y - 1.0f), ImVec2(max.x, max.y - 1.0f), Widgets::ColorU32(Gleam::Theme::RowDivider));
}

void PropertyDrawer::DrawCustom(const Gleam::TStringView label, size_t hash, UIFunction&& uiFunction)
{
	char buffer[64];
	std::memcpy(buffer, label.data(), label.size());
	buffer[label.size()] = '\0';

	ImGui::PushID(static_cast<int>(hash));
	if (DrawSectionHeader(buffer, nullptr))
	{
		ImGui::Dummy(ImVec2(0.0f, 2.0f));
		uiFunction();
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}
	ImGui::PopID();
}
