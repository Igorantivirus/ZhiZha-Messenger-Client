function(set_build_types LIBS_BUILD_TYPE BUILD_TYPE_MACRO)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        set(${LIBS_BUILD_TYPE} Release PARENT_SCOPE)
        set(${BUILD_TYPE_MACRO} "RELEASE_BUILD_TYPE" PARENT_SCOPE)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(${LIBS_BUILD_TYPE} Release PARENT_SCOPE)
        set(${BUILD_TYPE_MACRO} "RELWITHDEBINFO_BUILD_TYPE" PARENT_SCOPE)
    else()
        set(${LIBS_BUILD_TYPE} Debug PARENT_SCOPE)
        set(${BUILD_TYPE_MACRO} "DEBUG_BUILD_TYPE" PARENT_SCOPE)
    endif()
endfunction()


function(create_executeble EXECUTABLE_NAME SOURCE_FILES)
    if(ANDROID)
        find_package(ZLIB REQUIRED)
        add_library(${EXECUTABLE_NAME} SHARED ${SOURCE_FILES})
    else()
        add_executable(${EXECUTABLE_NAME} ${SOURCE_FILES})
    endif()
endfunction()

function(release_optimization TARGET_NAME)
    if(ANDROID AND (CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel"))
        target_compile_options(${TARGET_NAME} PRIVATE
            -O3
            -ffunction-sections -fdata-sections
            -fomit-frame-pointer
        )
        target_link_options(${TARGET_NAME} PRIVATE
            -Wl,--gc-sections
            -Wl,--as-needed
        )
    endif()
endfunction()

function(change_console_use TARGET_NAME USE_CONSOLE)
    if(NOT USE_CONSOLE)
        if(MSVC)
            target_link_options(${TARGET_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
        elseif(MINGW)
            target_link_options(${TARGET_NAME} PRIVATE "-mwindows")
        endif()
    endif()
endfunction()

