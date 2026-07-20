cmake_minimum_required(VERSION 3.13)
project(extract-xiso)

string(TOUPPER __${CMAKE_SYSTEM_NAME}__ TARGET_OS)

set(SOURCE_FILES
	extract-xiso.c
)

add_executable(extract-xiso ${SOURCE_FILES})
target_compile_definitions(extract-xiso PRIVATE ${TARGET_OS})

if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
	# Statically link the C runtime so extract-xiso.exe runs on a bare Windows
	# install. The default dynamic linkage needs ucrtbase.dll/vcruntime140.dll
	# from the Microsoft Visual C++ Redistributable, which isn't present on
	# every player's machine -- it fails to launch with "The code execution
	# cannot proceed because ucrtbase.dll was not found."
	target_compile_options(extract-xiso PRIVATE -fms-runtime-lib=static)
	target_link_options(extract-xiso PRIVATE -fms-runtime-lib=static)
endif()

install(TARGETS extract-xiso RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
