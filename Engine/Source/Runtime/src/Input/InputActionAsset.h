#pragma once
#include "InputActionDescriptor.h"

#include "Assets/Asset.h"

namespace Gleam {

template<typename Tag>
struct InputHandle
{
	static constexpr uint32_t InvalidValue = ~0u;

	uint32_t value = InvalidValue;

	NO_DISCARD FORCE_INLINE constexpr bool IsValid() const
	{
		return value != InvalidValue;
	}

	NO_DISCARD FORCE_INLINE constexpr bool operator==(const InputHandle& rhs) const
	{
		return value == rhs.value;
	}

	NO_DISCARD FORCE_INLINE constexpr bool operator!=(const InputHandle& rhs) const
	{
		return not (*this == rhs);
	}
};

struct InputActionTag;
struct InputActionMapTag;

using InputActionHandle = InputHandle<InputActionTag>;
using InputActionMapHandle = InputHandle<InputActionMapTag>;

struct InputBindingEntry
{
	InputActionHandle action;
	uint32_t binding = 0;
	uint32_t modifierCount = 0;
};

struct InputActionMapLayout
{
	uint32_t firstAction = 0;
	uint32_t actionCount = 0;
	TArray<InputBindingEntry> bindingOrder;
};

struct InputActionLayout
{
	InputActionMapHandle map;
	uint32_t index = 0;
	uint32_t firstBinding = 0;
	uint32_t bindingCount = 0;
};

class InputActionAsset final : public Asset
{
public:

	InputActionAsset(const AssetReference& reference, const AssetHeader& header, const InputActionAssetDescriptor& descriptor);

	InputActionAsset(const InputActionAssetDescriptor& descriptor);

	const InputActionAssetDescriptor& GetDescriptor() const;

	uint32_t GetActionMapCount() const;

	uint32_t GetActionCount() const;

	InputActionMapHandle FindActionMap(TStringView name) const;

	InputActionHandle FindAction(TStringView path) const;

	InputActionHandle FindAction(const Guid& map, const Guid& action) const;

	const InputActionMapDescriptor& GetActionMap(InputActionMapHandle map) const;

	const InputActionDescriptor& GetAction(InputActionHandle action) const;

	const InputActionMapLayout& GetActionMapLayout(InputActionMapHandle map) const;

	const InputActionLayout& GetActionLayout(InputActionHandle action) const;

	const TArray<InputActionMapHandle>& GetActionMapOrder() const;

	const TArray<InputBindingDescriptor>& GetBindings() const;

private:

	InputActionAssetDescriptor mDescriptor;

	TArray<InputActionMapLayout> mMapLayouts;

	TArray<InputActionLayout> mActionLayouts;

	TArray<InputActionMapHandle> mMapOrder;

	TArray<InputBindingDescriptor> mBindings;

};

} // namespace Gleam
