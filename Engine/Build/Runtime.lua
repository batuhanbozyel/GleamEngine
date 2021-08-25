project "Gleam"
	kind "SharedLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	location "%{wks.location}/Engine/Source/Runtime"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "gpch.h"
	pchsource "%{wks.location}/Engine/Source/Runtime/gpch.cpp"

	files
	{
		"%{wks.location}/Engine/Source/Runtime/**.h",
		"%{wks.location}/Engine/Source/Runtime/**.cpp"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLEAM_DYNAMIC",
		"GLEAM_EXPORT"
	}

	includedirs
	{
		"%{wks.location}/Engine/Source/Runtime",
		"%{IncludeDir.SDL2}",
		"%{IncludeDir.spdlog}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.tinyobjloader}",
		"%{IncludeDir.OpenFBX}",
		"%{IncludeDir.tinygltf}",
		GetVulkanSDKIncludePath()
	}

	libdirs
	{
		GetVulkanSDKLinkPath()
	}
	
	links
	{
		GetVulkanDynamicLibFile()
	}

	filter "configurations:Debug"
		defines "GDEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:RelWithDebugInfo"
		defines "GDEBUG"
		runtime "Release"
		optimize "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"