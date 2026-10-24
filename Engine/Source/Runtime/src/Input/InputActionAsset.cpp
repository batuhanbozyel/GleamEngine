#include "gpch.h"
#include "InputActionAsset.h"

#include <EASTL/sort.h>

using namespace Gleam;

static uint32_t CountModifiers(EnumFlag<ModifierKeys> modifiers)
{
	uint32_t count = 0;
	for (auto mask = modifiers.Value(); mask != 0; mask &= mask - 1)
	{
		++count;
	}
	return count;
}

InputActionAsset::InputActionAsset(const AssetReference& reference, const AssetHeader& header, const InputActionAssetDescriptor& descriptor)
	: Asset(reference, header)
	, mDescriptor(descriptor)
{
	mMapLayouts.reserve(mDescriptor.maps.size());
	mMapOrder.reserve(mDescriptor.maps.size());
	for (uint32_t mapIndex = 0; mapIndex < mDescriptor.maps.size(); ++mapIndex)
	{
		const auto& map = mDescriptor.maps[mapIndex];
		InputActionMapLayout mapLayout{
			.firstAction = static_cast<uint32_t>(mActionLayouts.size()),
			.actionCount = static_cast<uint32_t>(map.actions.size())
		};

		for (uint32_t actionIndex = 0; actionIndex < map.actions.size(); ++actionIndex)
		{
			const auto& bindings = map.actions[actionIndex].bindings;
			const auto action = InputActionHandle{ static_cast<uint32_t>(mActionLayouts.size()) };
			mActionLayouts.push_back(InputActionLayout{
				.map = InputActionMapHandle{ mapIndex },
				.index = actionIndex,
				.firstBinding = static_cast<uint32_t>(mBindings.size()),
				.bindingCount = static_cast<uint32_t>(bindings.size())
			});

			for (const auto& binding : bindings)
			{
				mapLayout.bindingOrder.push_back(InputBindingEntry{
					.action = action,
					.binding = static_cast<uint32_t>(mBindings.size()),
					.modifierCount = CountModifiers(binding.modifiers)
				});
				mBindings.push_back(binding);
			}
		}

		eastl::stable_sort(mapLayout.bindingOrder.begin(), mapLayout.bindingOrder.end(), [](const InputBindingEntry& lhs, const InputBindingEntry& rhs)
		{
			return lhs.modifierCount > rhs.modifierCount;
		});

		mMapLayouts.push_back(eastl::move(mapLayout));
		mMapOrder.push_back(InputActionMapHandle{ mapIndex });
	}

	eastl::stable_sort(mMapOrder.begin(), mMapOrder.end(), [this](InputActionMapHandle lhs, InputActionMapHandle rhs)
	{
		return mDescriptor.maps[lhs.value].priority > mDescriptor.maps[rhs.value].priority;
	});
}

InputActionAsset::InputActionAsset(const InputActionAssetDescriptor& descriptor)
	: InputActionAsset(AssetReference(), AssetHeader(), descriptor)
{

}

const InputActionAssetDescriptor& InputActionAsset::GetDescriptor() const
{
	return mDescriptor;
}

uint32_t InputActionAsset::GetActionMapCount() const
{
	return static_cast<uint32_t>(mMapLayouts.size());
}

uint32_t InputActionAsset::GetActionCount() const
{
	return static_cast<uint32_t>(mActionLayouts.size());
}

InputActionMapHandle InputActionAsset::FindActionMap(TStringView name) const
{
	for (uint32_t i = 0; i < mDescriptor.maps.size(); ++i)
	{
		if (mDescriptor.maps[i].name == name)
		{
			return InputActionMapHandle{ i };
		}
	}
	return InputActionMapHandle();
}

InputActionHandle InputActionAsset::FindAction(TStringView path) const
{
	uint32_t first = 0;
	uint32_t count = GetActionCount();
	TStringView name = path;

	const auto separator = path.find('/');
	if (separator != TStringView::npos)
	{
		const auto map = FindActionMap(path.substr(0, separator));
		if (map.IsValid())
		{
			first = mMapLayouts[map.value].firstAction;
			count = mMapLayouts[map.value].actionCount;
		}
		else
		{
			count = 0;
		}
		name = path.substr(separator + 1);
	}

	for (uint32_t i = first; i < first + count; ++i)
	{
		const auto action = InputActionHandle{ i };
		if (GetAction(action).name == name)
		{
			return action;
		}
	}
	return InputActionHandle();
}

InputActionHandle InputActionAsset::FindAction(const Guid& map, const Guid& action) const
{
	for (uint32_t i = 0; i < mActionLayouts.size(); ++i)
	{
		const auto handle = InputActionHandle{ i };
		if (GetAction(handle).id == action and GetActionMap(mActionLayouts[i].map).id == map)
		{
			return handle;
		}
	}
	return InputActionHandle();
}

const InputActionMapDescriptor& InputActionAsset::GetActionMap(InputActionMapHandle map) const
{
	return mDescriptor.maps[map.value];
}

const InputActionDescriptor& InputActionAsset::GetAction(InputActionHandle action) const
{
	const auto& layout = mActionLayouts[action.value];
	return mDescriptor.maps[layout.map.value].actions[layout.index];
}

const InputActionMapLayout& InputActionAsset::GetActionMapLayout(InputActionMapHandle map) const
{
	return mMapLayouts[map.value];
}

const InputActionLayout& InputActionAsset::GetActionLayout(InputActionHandle action) const
{
	return mActionLayouts[action.value];
}

uint32_t InputActionAsset::FindBinding(InputActionHandle action, const Guid& binding) const
{
	const auto& bindings = GetAction(action).bindings;
	for (uint32_t i = 0; i < bindings.size(); ++i)
	{
		if (bindings[i].id == binding)
		{
			return i;
		}
	}
	return InvalidBinding;
}

const TArray<InputActionMapHandle>& InputActionAsset::GetActionMapOrder() const
{
	return mMapOrder;
}

const TArray<InputBindingDescriptor>& InputActionAsset::GetBindings() const
{
	return mBindings;
}
