cmake_minimum_required(VERSION 3.25.0)
set(PROJECT_NAME TsfPad)
project(${PROJECT_NAME} LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(
    /D_UNICODE=1
    /DUNICODE=1
)

# fmt
find_package(fmt CONFIG REQUIRED)

include_directories("./")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(MY_EXECUTABLE_NAME "TsfPad")

add_executable(${PROJECT_NAME} WIN32
    "./src/DisplayAttribute.cpp"
    "./src/DisplayAttribute.h"
    "./src/private.h"
    "./src/resource.h"
    "./src/TextContainer.cpp"
    "./src/TextContainer.h"
    "./src/TextEditor.cpp"
    "./src/TextEditor.h"
    "./src/TextEditSink.cpp"
    "./src/TextEditSink.h"
    "./src/TextInputCtrl.cpp"
    "./src/TextInputCtrl.h"
    "./src/TextLayout.cpp"
    "./src/TextLayout.h"
    "./src/TextStore.cpp"
    "./src/TextStore.h"
    "./src/TsfPad.cpp"
    "./src/TsfPad.h"
    "./src/TsfPad.rc"
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    d2d1.lib
    dwrite.lib
    imm32.lib
    fmt::fmt-header-only
)
