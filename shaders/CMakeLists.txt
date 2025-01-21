if(WIN32)
    add_definitions(-DNOMINMAX)
    set(GLSL_COMPILER glslc.exe) 
    set(GLSL_COMPILER_FLAGS -O)
else()
    set(GLSL_COMPILER glslangValidator)
    set(GLSL_COMPILER_FLAGS -gVS -V)
endif(WIN32)

# Compiles all shaders in /glsl folder to SPIR-V

file(GLOB_RECURSE GLSL_SHADERS CONFIGURE_DEPENDS "glsl/*")


foreach(GLSL ${GLSL_SHADERS})
    get_filename_component(FILE_NAME ${GLSL} NAME_WE)
    get_filename_component(FILE_EXT ${GLSL} LAST_EXT)
    string(REPLACE "." "" FILE_TYPE ${FILE_EXT})
    if(NOT ${FILE_TYPE} STREQUAL h)
        set(SPIRV "${FILE_NAME}_${FILE_TYPE}.spv")
        add_custom_command( OUTPUT ${SPIRV}
                            COMMAND ${GLSL_COMPILER} ${GLSL} ${GLSL_COMPILER_FLAGS} -o ${SPIRV}
                            DEPENDS ${GLSL})
        list(APPEND SPIRV_SHADERS ${SPIRV})
    endif()
endforeach(GLSL)


add_custom_target(tga_shaders DEPENDS ${SPIRV_SHADERS})
