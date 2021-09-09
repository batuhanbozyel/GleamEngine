#pragma once

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Core/PlatformTargetDefines.h"
#include "Core/EngineDefines.h"

#include "Container/Pointer.h"
#include "Container/String.h"

#include "IO/Log.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif