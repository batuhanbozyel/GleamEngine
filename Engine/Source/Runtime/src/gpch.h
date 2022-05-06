#pragma once

#include <fstream>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <utility>
#include <algorithm>
#include <functional>
#include <typeindex>
#include <type_traits>
#include <string>
#include <sstream>
#include <list>
#include <array>
#include <vector>
#include <cstdarg>
#include <unordered_map>
#include <unordered_set>

#include "Core/PlatformTargetDefines.h"
#include "Core/EngineDefines.h"

#include "Container/Pointer.h"
#include "Container/String.h"
#include "Container/Array.h"
#include "Container/Hash.h"

#include "Math/Common.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Quaternion.h"
#include "Math/Matrix2.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Math/Color.h"

#include "Core/Events/ApplicationEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Core/Events/MouseEvent.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Time.h"

#include "IO/Log.h"
#include "IO/IOUtils.h"

#include "Renderer/GraphicsObject.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif
