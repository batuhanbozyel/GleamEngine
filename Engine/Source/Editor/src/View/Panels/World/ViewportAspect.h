#pragma once
#include "Core/Attributes.h"
#include "Math/Size.h"

namespace GEditor {

GENUM(ViewportAspect, "0873E79A-91F1-4245-84A3-BB622F928346", Serializable, PrettyName("Viewport Aspect"))
{
	GITEM(Free, "8FEC8C1E-F3FE-4FC6-B79F-60B7EF632ABE", PrettyName("Free Aspect")),
	GITEM(Ratio16x9, "1CE13856-4340-46A1-A728-61A6BC69B270", PrettyName("16:9")),
	GITEM(Ratio16x10, "7A8AA3D4-BEEB-4971-BC4F-1B0497DE4C60", PrettyName("16:10")),
	GITEM(Ratio21x9, "473B3EC2-A237-4BE4-8B4D-89FDF0007CC0", PrettyName("21:9")),
	GITEM(Ratio4x3, "248490F4-6ED5-40D1-B8A6-7C160D230160", PrettyName("4:3")),
	GITEM(Ratio1x1, "487E7310-9E31-4771-853B-2BB358E0D218", PrettyName("1:1")),
	GITEM(HD, "3E7B473D-CC3D-4782-A463-9A3466AEA0EE", PrettyName("1280x720")),
	GITEM(FullHD, "E8ABA465-E318-4549-859A-4475C7969EAD", PrettyName("1920x1080")),
	GITEM(QHD, "90A1035B-721D-4AF9-B070-8E15F7F6C789", PrettyName("2560x1440")),
	GITEM(UHD, "863D266C-2B8F-4D3C-A9CE-C585562472D9", PrettyName("3840x2160")),
	GITEM(Custom, "61FF9007-4316-4017-8FAE-A1C8B941584F", PrettyName("Custom Resolution"))
};

GSTRUCT(ViewportAspectConfig, "859E0AEC-749E-4813-A30C-2362BD35454E", Serializable, PrettyName("Viewport Aspect"))
{
	GFIELD("2C49E256-F41E-4EF9-83C9-2141B241A443", Serializable, PrettyName("Aspect"))
	ViewportAspect aspect = ViewportAspect::Free;

	GFIELD("00C11D1D-A258-49D0-A27F-565CED0BFFB5", Serializable, PrettyName("Custom Resolution"))
	Gleam::Size customResolution = Gleam::Size(1920.0f, 1080.0f);

	GFIELD("4D089D79-E552-4E43-90DB-7D353ADB41B9", Serializable, PrettyName("Apply In Edit Mode"))
	bool applyInEditMode = false;
};

} // namespace GEditor
