#pragma once
#include "Macro.h"

GLEAM_TYPE(Gleam::Guid, Guid("3B44E2A0-D3B8-4C04-A4E9-E2484916B7F3"))
GLEAM_END

GLEAM_TYPE(Gleam::TString, Guid("D41E842F-8A78-41A0-9ADF-3CE585F51668"))
GLEAM_END

GLEAM_TYPE(Gleam::Filesystem::Path, Guid("332C17DE-9FF7-4848-A841-1BC2FA077221"))
GLEAM_END

GLEAM_TEMPLATE((typename T), (std::vector<T>), Guid("FA7B0E0D-F6B5-4060-9E2B-E43B0109F1FC"))
GLEAM_END

GLEAM_TEMPLATE((typename T, size_t s), (std::array<T, s>), Guid("4D138331-C39C-4845-BE74-C0CEA70DDCFD"))
GLEAM_END
