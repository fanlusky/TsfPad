set(TSF_D2D_TEXTBOX_SOURCES
    "./src/DisplayAttribute.cpp"
    "./src/TextContainer.cpp"
    "./src/TextEditor.cpp"
    "./src/TextEditSink.cpp"
    "./src/TextInputCtrl.cpp"
    "./src/TextLayout.cpp"
    "./src/TextStore.cpp"
    "./src/TsfD2DTextBox.cpp"
    "./src/TsfTextServices.cpp"
)

set(TSF_D2D_TEXTBOX_HEADERS
    "./include/TsfD2DTextBox.h"
    "./src/DisplayAttribute.h"
    "./src/private.h"
    "./src/TextContainer.h"
    "./src/TextEditor.h"
    "./src/TextEditSink.h"
    "./src/TextInputCtrl.h"
    "./src/TextLayout.h"
    "./src/TextStore.h"
    "./src/TsfTextServices.h"
)

add_library(TsfD2DTextBox STATIC
    ${TSF_D2D_TEXTBOX_SOURCES}
    ${TSF_D2D_TEXTBOX_HEADERS}
)

target_include_directories(TsfD2DTextBox
    PUBLIC "./include"
    PRIVATE "./src" "./"
)

target_link_libraries(TsfD2DTextBox PUBLIC
    d2d1.lib
    dwrite.lib
    imm32.lib
    fmt::fmt-header-only
)

add_executable(TsfD2DTextBoxDemo WIN32
    "./demo/TsfD2DTextBoxDemo.cpp"
)

target_include_directories(TsfD2DTextBoxDemo PRIVATE "./include")
target_link_libraries(TsfD2DTextBoxDemo PRIVATE TsfD2DTextBox)
