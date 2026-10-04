#pragma once
#include "Core/Attributes.h"

namespace GEditor {

GSTRUCT(EditorPreferences, "9AB864B8-7498-415D-B09F-3C4BD2DC23D5", Serializable, PrettyName("Editor Preferences"))
{
	GFIELD("18E3DC17-88F9-4E9F-A59B-2430E9D12E6E", Serializable, PrettyName("Interface Scale"))
	float uiScale = 1.0f;
};

} // namespace GEditor
