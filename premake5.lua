-- premake5.lua
workspace "Multithreaded 7-Zip"
   architecture "x64"
   configurations {"Debug", "Release"}

project "exec"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++17"
   targetdir "build"
   objdir "build/obj/"
   files {"src/**.cpp"}
   includedirs {"external/include/", "src/"}
   libdirs {"external/lib/"}

   filter "configurations:Debug"
       defines {"DEBUG", "ENABLE_XENON_LOGGER"}
       symbols "On"

   filter "configurations:Release"
       defines { "NDEBUG" }
       optimize "On"
