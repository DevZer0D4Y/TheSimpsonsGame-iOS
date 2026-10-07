# Run with -DSDK_SOURCE=... -DTEST_WORK=... -P this file.
if(NOT SDK_SOURCE OR NOT TEST_WORK)
    message(FATAL_ERROR "Pass SDK_SOURCE and a temporary TEST_WORK directory")
endif()
include("${SDK_SOURCE}/cmake/rex_version.cmake")
file(MAKE_DIRECTORY "${TEST_WORK}/vendored-sdk")
# Simulate an enclosing application's v0.2.3 tag. A vendored SDK must ignore it.
file(WRITE "${TEST_WORK}/parent-git" "#!/bin/sh\nprintf 'v0.2.3\\n'\n")
file(CHMOD "${TEST_WORK}/parent-git"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
set(GIT_EXECUTABLE "${TEST_WORK}/parent-git")
rex_resolve_version(version SOURCE_DIR "${TEST_WORK}/vendored-sdk"
    FLOOR_MAJOR 0 FLOOR_MINOR 10)
if(NOT version MATCHES "^0\\.10\\.0")
    message(FATAL_ERROR "Vendored SDK inherited application version: ${version}")
endif()
message(STATUS "PASS: source-archive SDK ignores enclosing application tags (${version})")
