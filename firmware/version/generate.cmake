# Writes the file for cim_version. Run at every build, so the version follows
# new commits and tags without a reconfigure. Input: OUT, REPO, and optionally
# CIM_VERSION to override the version.
execute_process(
    COMMAND git -c safe.directory=* -C ${REPO} describe --tags --always --dirty
    OUTPUT_VARIABLE version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE result
)
if (NOT result EQUAL 0 OR version STREQUAL "")
    set(version unknown)
endif()
if (CIM_VERSION)
    set(version ${CIM_VERSION})
endif()

set(content "#include \"cim/version.h\"\n\nconst char cim_version[] = \"${version}\";\n")
set(old "")
if (EXISTS ${OUT})
    file(READ ${OUT} old)
endif()
if (NOT old STREQUAL content)
    file(WRITE ${OUT} "${content}")
endif()
