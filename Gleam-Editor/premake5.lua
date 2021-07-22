project "Gleam-Editor"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Gleam-Core/src",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"Gleam-Core",
		"ImGui"
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