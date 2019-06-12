--
-- premake5 file to build RecastDemo
-- http://premake.github.io/
--

local action = _ACTION or ""
local outdir = action

workspace  "libevent_prj"
	configurations { 
		"Debug",
		--"Release"
	}
	location (outdir)

project "libevent"
	language "C++"
	kind "ConsoleApp"


	includedirs { 
	}
	files	{ 
		"../libevent-2.1.8-stable/out/include/**.h",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
	}
    
project "lib_prj"
	language "C++"
	kind "ConsoleApp"


	includedirs { 
		"../libevent-2.1.8-stable/out/include/",
	}
	files	{ 
		"../lib_prj/**.cpp",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
		"../lib_prj/**.cc",
		"../lib_prj/**.h",
		"../lib_prj/**.hpp",
		"../lib_prj/**.txt",
		"../external/include/event2/**.h", 
	}

project "server_prj"
	language "C++"
	kind "ConsoleApp"


	includedirs { 
		"../lib_prj/",
	}
	files	{ 
		"../server_prj/**.cpp",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
		"../server_prj/**.cc",
		"../server_prj/**.h",
		"../server_prj/**.hpp",
		"../server_prj/**.txt",
		"../lib_prj/*.h", 
	}
    
project "client_prj"
	language "C++"
	kind "ConsoleApp"


	includedirs { 
		"../lib_prj/",
	}
	files	{ 
		"../client_prj/**.cpp",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
		"../client_prj/**.cc",
		"../client_prj/**.h",
		"../client_prj/**.hpp",
		"../client_prj/**.txt",
		"../lib_prj/*.h", 
	}
    
project "bus_mgr_lib_prj"
	language "C++"
	kind "ConsoleApp"


	includedirs { 
		"../lib_prj/",
	}
	files	{ 
		"../bus_mgr_lib_prj/**.cpp",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
		"../bus_mgr_lib_prj/**.cc",
		"../bus_mgr_lib_prj/**.h",
		"../bus_mgr_lib_prj/**.hpp",
		"../bus_mgr_lib_prj/**.txt",
		"../lib_prj/*.h", 
	}
    
project "test_bus_mgr_prj"
	language "C++"
	kind "ConsoleApp"


	includedirs { 
		"../lib_prj/",
	}
	files	{ 
		"../test_bus_mgr_prj/**.cpp",  --**递归所有子目录，指定目录可用 "cc/*.cpp" 或者  "cc/**.cpp"
		"../test_bus_mgr_prj/**.cc",
		"../test_bus_mgr_prj/**.h",
		"../test_bus_mgr_prj/**.hpp",
		"../test_bus_mgr_prj/**.txt",
		"../lib_prj/*.h", 
	}
    
    
    