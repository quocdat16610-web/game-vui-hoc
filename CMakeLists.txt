cmake_minimum_required(VERSION 3.16)
project(CodeVoDai LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mã nguồn chứa tiếng Việt (UTF-8): bảo Visual Studio đọc file đúng bảng mã
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ---------------------------------------------------------------------------
# raylib: thư viện đồ họa/âm thanh/input. CMake sẽ tự tải về lần build đầu.
# ---------------------------------------------------------------------------
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------------
# Game chính
# ---------------------------------------------------------------------------
add_executable(CodeVoDai
    src/main.cpp
    src/Common.cpp
    src/UI.cpp
    src/Lessons.cpp
    src/Office.cpp
    src/LessonScreen.cpp
    src/Arena.cpp
    src/FighterModel.cpp
    src/Gfx.cpp
    src/Audio.cpp
    src/MiniCpp.cpp
    src/Challenges.cpp
    src/PlayerCode.cpp
    src/CodeEditor.cpp
    src/Character.cpp
    src/World.cpp
)
target_link_libraries(CodeVoDai PRIVATE raylib)

if(WIN32)
    # Chạy như ứng dụng Windows (không bật cửa sổ console đen)
    set_target_properties(CodeVoDai PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(CodeVoDai PRIVATE /ENTRY:mainCRTStartup)
    endif()
    if(MINGW)
        # Gộp thư viện C++ vào exe để chạy được trên máy không cài MinGW
        target_link_options(CodeVoDai PRIVATE -static -static-libgcc -static-libstdc++ $<$<CONFIG:Release>:-s>)
    endif()
endif()

# Copy thư mục assets (font...) cạnh file exe sau khi build
add_custom_command(TARGET CodeVoDai POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CodeVoDai>/assets)

# ---------------------------------------------------------------------------
# Các bài học mẫu (thư mục lessons/). Mỗi file là 1 chương trình chạy độc lập.
# ---------------------------------------------------------------------------
option(BUILD_LESSONS "Build cac file bai hoc trong lessons/" ON)
if(BUILD_LESSONS)
    file(GLOB LESSON_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/lessons/*.cpp)
    foreach(src ${LESSON_SOURCES})
        get_filename_component(name ${src} NAME_WE)
        add_executable(bai_${name} ${src})
        set_target_properties(bai_${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lessons)
        if(MINGW)
            target_link_options(bai_${name} PRIVATE -static -static-libgcc -static-libstdc++ $<$<CONFIG:Release>:-s>)
        endif()
        # Bài nào dùng raylib thì link raylib
        file(READ ${src} content)
        string(FIND "${content}" "#include \"raylib.h\"" uses_raylib)
        if(NOT uses_raylib EQUAL -1)
            target_link_libraries(bai_${name} PRIVATE raylib)
        endif()
    endforeach()
endif()
