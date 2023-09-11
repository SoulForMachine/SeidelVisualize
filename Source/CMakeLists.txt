
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++17 -Wall -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-result -Wno-switch -Wno-sign-compare")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	add_definitions(-DUNICODE -D_UNICODE)
endif()

add_executable(SeidelVisualize
	"Main.cpp"
	"Common.h"
	"MainWindow.h" "MainWindow.cpp"
	"SplitterWidget.h" "SplitterWidget.cpp"
	"PolygonWidget.h" "PolygonWidget.cpp"
	"TrapTreeWidget.h" "TrapTreeWidget.cpp"
	"MenuPanel.h" "MenuPanel.cpp"
	"SeidelTriangulator.h" "SeidelTriangulator.cpp"
	"StepThroughPanel.h" "StepThroughPanel.cpp"
	"IntSliderWidget.h" "IntSliderWidget.cpp"
	"ComboWidget.h" "ComboWidget.cpp"
	"PopupButtonWidget.h" "PopupButtonWidget.cpp"
	"Benchmark.h" "Benchmark.cpp"
	"Serialization.h" "Serialization.cpp")

if(UNIX AND NOT APPLE)
	target_link_libraries(SeidelVisualize nanogui stdc++fs)
else()
	target_link_libraries(SeidelVisualize nanogui)
endif()
