#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Needed for using parallel algorithms
env.Append(CXXFLAGS=['-fexceptions'])

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["fluid/cpp_src"])
sources = Glob("fluid/cpp_src/*.cpp")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/fluid/libfluid.{}.{}.framework/libfluid.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "addons/fluid/libfluid{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
