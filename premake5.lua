function GetVulkanSDKLinkPath()
	if os.is("windows") then
		return "vulkan-1.dll"
	else
		return os.getenv("VULKAN_SDK") .. "/lib/libvulkan.1.dylib"
	end
end

function GetVulkanSDKIncludePath()
	local vulkan_sdk = os.getenv("VULKAN_SDK")
	if os.is("windows") then
		return vulkan_sdk .. "/Include"
	else
		return vulkan_sdk .. "/include"
	end
end

workspace "GleamEngine"
	architecture "x86_64"
	startproject "GleamEditor"

	configurations
	{
		"Debug",
		"RelWithDebugInfo",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["SDL2"] = "%{wks.location}/Engine/ThirdParty/SDL2/include"
IncludeDir["spdlog"] = "%{wks.location}/Engine/ThirdParty/spdlog/include"
IncludeDir["glm"] = "%{wks.location}/Engine/ThirdParty/glm"
IncludeDir["stb_image"] = "%{wks.location}/Engine/ThirdParty/stb"
IncludeDir["ImGui"] = "%{wks.location}/Engine/ThirdParty/imgui"
IncludeDir["ImGuizmo"] = "%{wks.location}/Engine/ThirdParty/ImGuizmo"
IncludeDir["entt"] = "%{wks.location}/Engine/ThirdParty/entt/include"
IncludeDir["tinyobjloader"] = "%{wks.location}/Engine/ThirdParty/tinyobjloader"
IncludeDir["OpenFBX"] = "%{wks.location}/Engine/ThirdParty/OpenFBX/src"
IncludeDir["tinygltf"] = "%{wks.location}/Engine/ThirdParty/tinygltf"

group "Dependencies"
	include "Tools/premake"
group ""

include "Engine/Build/Runtime.lua"
include "Engine/Build/Editor.lua"