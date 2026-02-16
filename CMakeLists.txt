cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Запрещает fallback на старый стандарт
set(CMAKE_CXX_EXTENSIONS OFF)        # Отключает расширения компилятора

project("Client-UI" LANGUAGES CXX C)

#--------------------------SET--------------------------#

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(EXTERN_DIR "${PROJECT_SOURCE_DIR}/extern")
set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

if(ANDROID)
  set(CMAKE_ABI ${CMAKE_ANDROID_ARCH_ABI})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(LIBS_BUILD_TYPE Release)
  set(BUILD_TYPE_MACRO "RELEASE_BUILD_TYPE")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  set(LIBS_BUILD_TYPE Release)
  set(BUILD_TYPE_MACRO "RELWITHDEBINFO_BUILD_TYPE")
else()
  set(LIBS_BUILD_TYPE Debug)
  set(BUILD_TYPE_MACRO "DEBUG_BUILD_TYPE")
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(RmlUi_DIR      "${EXTERNAL_DIR}/RmlUi-6.2/${CMAKE_ABI}/${LIBS_BUILD_TYPE}/lib/cmake/RmlUi")
set(Freetype_DIR   "${EXTERNAL_DIR}/FreeType/${CMAKE_ABI}/${LIBS_BUILD_TYPE}/lib/cmake/freetype")
set(SDLWrapper_DIR "${EXTERNAL_DIR}/SDLWrapper/${CMAKE_ABI}/${LIBS_BUILD_TYPE}/lib/cmake/SDLWrapper")

if(NOT DEFINED EXTERNAL_DIR)
    message(ERROR EXTERNAL_DIR is not defined)
endif()

set(SOURCE_CPP

  ${EXTERN_DIR}/RmlUi/RmlUi_Platform_SDL.cpp
  ${EXTERN_DIR}/RmlUi/RmlUi_Renderer_SDL.cpp
  ${EXTERN_DIR}/pugixml/pugixml.cpp
  ${SRC_DIR}/main.cpp 
  ${SRC_DIR}/network/WebSocketClient.cpp
  ${SRC_DIR}/network/WebSocketClientStandalone.cpp
  ${SRC_DIR}/network/Utils/WebSocketSession.cpp
  ${SRC_DIR}/protocol/JsonPacker.cpp
  ${SRC_DIR}/protocol/JsonParser.cpp


  
  
)
set(SOURCE_HPP
  # ${SRC_DIR}/Objects.hpp
  # ${SRC_DIR}/CollisionMeneger.hpp
)
set(INCLUDE_PATHS
  ${SRC_DIR}
  ${SRC_DIR}/App
  ${EXTERN_DIR}
)

#--------------------------FIND_PACKAGE--------------------------#

#nlohmann_json
if(WIN32)
    find_package(nlohmann_json 3.12.0 REQUIRED)
    add_library(nlohmann_json INTERFACE IMPORTED)
else()
    find_package(nlohmann_json REQUIRED)
endif()
#end nlohmann_json

find_package(Boost 1.70 REQUIRED COMPONENTS system)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)

find_package(Freetype CONFIG REQUIRED)
find_package(RmlUi CONFIG REQUIRED)
find_package(SDLWrapper CONFIG REQUIRED)

if (ANDROID)
  find_package(ZLIB REQUIRED)
  add_library(${PROJECT_NAME} SHARED ${SOURCE_CPP} ${SOURCE_HPP})
else()
  add_executable(${PROJECT_NAME} ${SOURCE_CPP} ${SOURCE_HPP})
endif()

#--------------------------TARGET--------------------------#

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_PATHS})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  RMLUI_SDL_VERSION_MAJOR=3
  RMLUI_STATIC_LIB
  SDL_MAIN_USE_CALLBACKS
  ${BUILD_TYPE_MACRO}
)

if(ANDROID)

  target_link_libraries(${PROJECT_NAME} PRIVATE
    android
    log
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDLWrapper::SDLWrapper
    freetype
    RmlUi::RmlUi
    nlohmann_json::nlohmann_json
  )

else()

  target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    SDLWrapper::SDLWrapper
    Freetype::Freetype
    RmlUi::RmlUi
    nlohmann_json::nlohmann_json
  )

endif()

# Подключаем директории include
target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

# Определения для header-only режима
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    BOOST_ASIO_NO_DEPRECATED
    BOOST_BEAST_USE_STD_STRING_VIEW
)

# Линкуем только system (beast и asio header-only)
if(TARGET Boost::system)
    target_link_libraries(${PROJECT_NAME} PRIVATE Boost::system)
else()
    # Для старых версий CMake
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_SYSTEM_LIBRARY})
endif()


#--------------------------OPTIMIATION--------------------------#

if(ANDROID AND (CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel"))
  target_compile_options(${PROJECT_NAME} PRIVATE
          -O3
          -ffunction-sections -fdata-sections
          -fomit-frame-pointer
  )
  target_link_options(${PROJECT_NAME} PRIVATE
          -Wl,--gc-sections
          -Wl,--as-needed
  )
endif()

if(NOT USE_CONSOLE)
  if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS")
  elseif(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE "-mwindows")
  endif()
endif()