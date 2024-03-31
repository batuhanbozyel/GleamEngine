#pragma once
#include "Macro.h"

// This should never be used, but it is here to avoid returning reference to local variable
struct Dummy {};
GLEAM_TYPE(Dummy, Guid("00000000-0000-0000-0000-000000000000"))
GLEAM_END

GLEAM_TYPE(Gleam::TString, Guid("D41E842F-8A78-41A0-9ADF-3CE585F51668"))
    GLEAM_FUNC(length)
    GLEAM_FUNC(c_str)
GLEAM_END

GLEAM_TEMPLATE((typename T), (std::vector<T>), Guid("FA7B0E0D-F6B5-4060-9E2B-E43B0109F1FC"))
    GLEAM_FUNC(size)
    GLEAM_FUNC(data)
GLEAM_END

GLEAM_TEMPLATE((typename T, size_t size), (std::array<T, size>), Guid("4D138331-C39C-4845-BE74-C0CEA70DDCFD"))
    GLEAM_FUNC(data)
GLEAM_END
