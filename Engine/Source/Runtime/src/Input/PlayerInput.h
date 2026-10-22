#pragma once
#include "InputActionDescriptor.h"

#include "Assets/AssetReference.h"

namespace Gleam {

GENUM(PlayerJoinBehavior, "BD64C7CC-6F84-4133-B834-6474A32C2017", Serializable, PrettyName("Join Behavior"))
{
	GITEM(Manual, "2665CCDE-81D7-44C0-97B9-3145030B2B26"),
	GITEM(JoinOnButtonPress, "5C7555AC-025B-4139-A62A-0D64A143E45E", PrettyName("Join On Button Press"))
};

GSTRUCT(PlayerInputManager, "65E4057E-DA6E-4C1F-8FD6-FB72A475B11E", EntityComponent, Singleton(true), Serializable, PrettyName("Player Input Manager"))
{
	GFIELD("B89B8E67-3EC5-4ABE-81DB-964815F09074", Serializable, PrettyName("Join Behavior"))
	PlayerJoinBehavior joinBehavior = PlayerJoinBehavior::Manual;

	GFIELD("0F2D092A-0B71-4E67-845E-5523F69D0CA1", Serializable, PrettyName("Max Players"))
	uint32_t maxPlayers = 1;
};

GSTRUCT(PlayerInput, "1875574F-F821-42DE-9315-7ECAA057FDE1", EntityComponent, Serializable, PrettyName("Player Input"))
{
	GFIELD("A55ADE84-0F50-4E38-826B-C2D2509046D5", Serializable, PrettyName("Actions"), AssetType(TypeOf<InputActionAssetDescriptor>))
	AssetReference actions;

	GFIELD("12AF9959-1D0C-4C8E-BA2A-57B988B8A9BC", Serializable, PrettyName("Player Index"))
	uint32_t playerIndex = 0;
};

} // namespace Gleam
