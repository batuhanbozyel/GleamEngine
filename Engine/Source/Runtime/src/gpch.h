#pragma once

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <memory>
#include <optional>
#include <mutex>
#include <atomic>
#include <utility>
#include <algorithm>
#include <functional>
#include <concepts>
#include <typeindex>
#include <type_traits>
#include <tuple>
#include <numeric>
#include <chrono>
#include <string>
#include <sstream>
#include <queue>
#include <list>
#include <array>
#include <vector>
#include <cstdarg>
#include <variant>
#include <bitset>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <entt/core/type_info.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/meta.hpp>
#include <entt/meta/factory.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include "Core/PlatformTargetDefines.h"
#include "Core/Macro.h"
#include "Core/EngineDefines.h"

#include "Container/Pointer.h"
#include "Container/String.h"
#include "Container/Array.h"
#include "Container/Stack.h"
#include "Container/Queue.h"
#include "Container/List.h"
#include "Container/Hash.h"
#include "Container/Tuple.h"
#include "Container/AnyArray.h"
#include "Container/PolyArray.h"

#include "IO/Log.h"
#include "IO/File.h"
#include "IO/Filesystem.h"
#include "IO/FileDialog.h"

#include "Reflection/TypeTraits.h"
#include "Reflection/Attribute.h"
#include "Reflection/Meta.h"
#include "Reflection/Reflection.h"

#include "Math/Common.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Quaternion.h"
#include "Math/Color.h"
#include "Math/Float2x2.h"
#include "Math/Float3x3.h"
#include "Math/Float4x4.h"
#include "Math/Size.h"
#include "Math/Rect.h"
#include "Math/BoundingBox.h"

#include "Core/Events/ApplicationEvent.h"
#include "Core/Events/RendererEvent.h"
#include "Core/Events/WindowEvent.h"
#include "Core/Events/MouseEvent.h"
#include "Core/Events/KeyEvent.h"
#include "Core/Globals.h"
#include "Core/GUID.h"
#include "Core/Time.h"

#include "Renderer/Shaders/ShaderInterop.h"
#include "Renderer/Shaders/ShaderTypes.h"
#include "Renderer/GraphicsObject.h"

#include "World/Components/MeshRenderer.h"
#include "World/Components/Transform.h"
#include "World/Components/Camera.h"

#include "Renderer/RenderGraph/RenderGraphResource.h"
#include "Renderer/Renderer.h"

#include "Input/InputSystem.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif
