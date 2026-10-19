if (NOT DEFINED SHADER_VALIDATOR OR NOT EXISTS "${SHADER_VALIDATOR}")
    message(FATAL_ERROR "spirv-val was not found. Install SPIRV-Tools, then reconfigure the build.")
endif ()

file(GLOB SHADER_ARTIFACTS "${SHADER_DIRECTORY}/*.spv")
if (NOT SHADER_ARTIFACTS)
    message(FATAL_ERROR "No SPIR-V artifacts found in ${SHADER_DIRECTORY}. Build with SHADER_TARGETS=spirv first.")
endif ()

set(FAILED_SHADERS "")
foreach (SHADER_ARTIFACT IN LISTS SHADER_ARTIFACTS)
    execute_process(
        COMMAND "${SHADER_VALIDATOR}" --target-env vulkan1.0 "${SHADER_ARTIFACT}"
        RESULT_VARIABLE SHADER_RESULT
        OUTPUT_VARIABLE SHADER_OUTPUT
        ERROR_VARIABLE SHADER_ERROR
    )
    if (NOT SHADER_RESULT STREQUAL "0")
        get_filename_component(SHADER_NAME "${SHADER_ARTIFACT}" NAME)
        list(APPEND FAILED_SHADERS "${SHADER_NAME}")
        message(STATUS "${SHADER_NAME} (validator returned ${SHADER_RESULT}): ${SHADER_OUTPUT}${SHADER_ERROR}")
    endif ()
endforeach ()

if (FAILED_SHADERS)
    message(FATAL_ERROR "Invalid Vulkan shader artifacts: ${FAILED_SHADERS}")
endif ()

list(LENGTH SHADER_ARTIFACTS SHADER_COUNT)
message(STATUS "Validated ${SHADER_COUNT} SPIR-V artifacts against Vulkan 1.0")
