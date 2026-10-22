#pragma once
#include "InputUser.h"
#include "InputActionMap.h"

namespace Gleam {

class InputActionSet final
{
	friend class InputSystem;
public:

	void Build(const InputActionAssetDescriptor& descriptor);

	InputUser* GetUser() const;

	void SetUser(InputUser* user);

	uint32_t GetPlayerIndex() const;

	void SetPlayerIndex(uint32_t index);

	InputActionMap* FindActionMap(TStringView name) const;

	InputAction* FindAction(TStringView path) const;

	const TArray<RefCounted<InputActionMap>>& GetActionMaps() const;

private:

	TArray<RefCounted<InputActionMap>> mMaps;

	InputUser* mUser = nullptr;

	uint32_t mPlayerIndex = InputUser::InvalidPlayerIndex;

};

} // namespace Gleam
