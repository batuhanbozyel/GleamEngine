function GetVulkanSDKLinkPath()
	local vulkan_sdk = os.getenv("VULKAN_SDK")
	if os.is("windows") then
		return vulkan_sdk .. "/Lib/vulkan-1.lib"
	else
		vulkan_sdk = vulkan_sdk .. "/lib"
		print(vulkan_sdk)
		for filename in io.popen('dir "' .. vulkan_sdk ..'" /b'):lines() do
			if filename.match("libvulkan.1.") then return vulkan_sdk .. filename end
		end
		return nil
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

workspace "Gleam"
	architecture "x86_64"
	startproject "Gleam-Editor"

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
IncludeDir["SDL2"] = "%{wks.location}/Gleam-Core/vendor/SDL2/include"
IncludeDir["spdlog"] = "%{wks.location}/Gleam-Core/vendor/spdlog/include"
IncludeDir["glm"] = "%{wks.location}/Gleam-Core/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Gleam-Core/vendor/stb"
IncludeDir["ImGui"] = "%{wks.location}/Gleam-Core/vendor/imgui"
IncludeDir["ImGuizmo"] = "%{wks.location}/Gleam-Core/vendor/ImGuizmo"
IncludeDir["entt"] = "%{wks.location}/Gleam-Core/vendor/entt/include"
IncludeDir["tinyobjloader"] = "%{wks.location}/Gleam-Core/vendor/tinyobjloader"
IncludeDir["OpenFBX"] = "%{wks.location}/Gleam-Core/vendor/OpenFBX/src"
IncludeDir["tinygltf"] = "%{wks.location}/Gleam-Core/vendor/tinygltf"

group "Dependencies"
	include "vendor/premake"
group ""

include "Gleam-Core"
include "Gleam-Editor"