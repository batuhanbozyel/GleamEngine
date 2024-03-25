#pragma once

#include "Core/EngineDefines.h"

#include "Container/Pointer.h"
#include "Container/String.h"
#include "Container/Array.h"
#include "Container/Stack.h"
#include "Container/Queue.h"
#include "Container/List.h"
#include "Container/Hash.h"
#include "Container/AnyArray.h"
#include "Container/PolyArray.h"

#include "IO/Log.h"
#include "IO/File.h"
#include "IO/FileDialog.h"

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
#include "Core/WindowSystem.h"
#include "Core/Application.h"
#include "Core/GUID.h"
#include "Core/Time.h"

#include "Assets/Asset.h"

#include "Renderer/Shaders/ShaderInterop.h"
#include "Renderer/Shaders/ShaderTypes.h"

#include "World/Components/MeshRenderer.h"
#include "World/Components/Transform2D.h"
#include "World/Components/Transform.h"
#include "World/Components/Camera.h"

#include "Renderer/Mesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/Material/MaterialSystem.h"

#include "Input/InputSystem.h"

#include "World/World.h"
#include "Assets/AssetLibrary.h"

#include "Renderer/Renderers/UIRenderer.h"
#include "Renderer/Renderers/DebugRenderer.h"
#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/ImGuiRenderer.h"
#include "Renderer/Renderers/PostProcessStack.h"
