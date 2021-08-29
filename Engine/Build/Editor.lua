project "GleamEditor"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	location "%{wks.location}/Engine/Source/Editor"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	postbuildcommands 
	{
		"{COPY} %{wks.location}/bin/" .. outputdir .. "/Gleam/Gleam.dll %{cfg.targetdir}"
	}

	files
	{
		"%{wks.location}/Engine/Source/Editor/**.h",
		"%{wks.location}/Engine/Source/Editor/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Engine/Source/Runtime"
	}

	links
	{
		"Gleam"
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