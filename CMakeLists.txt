cmake_minimum_required(VERSION 3.16)
project(embedded_cpp_alpr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ALPR_BUILD_APP "Build OpenCV/Tesseract application" ON)
option(ALPR_BUILD_TESTS "Build unit tests" ON)

add_library(alpr_core src/PlateUtils.cpp src/EventLogger.cpp)
target_include_directories(alpr_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
  target_compile_options(alpr_core PRIVATE /W4)
else()
  target_compile_options(alpr_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ALPR_BUILD_APP)
  find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(TESSERACT REQUIRED IMPORTED_TARGET tesseract)
  add_executable(alpr src/main.cpp src/PlateDetector.cpp src/OCRProcessor.cpp)
  target_include_directories(alpr PRIVATE ${OpenCV_INCLUDE_DIRS})
  target_link_libraries(alpr PRIVATE alpr_core ${OpenCV_LIBS} PkgConfig::TESSERACT)
  if(NOT MSVC)
    target_compile_options(alpr PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(ALPR_BUILD_TESTS)
  enable_testing()
  add_executable(test_core tests/test_core.cpp)
  target_link_libraries(test_core PRIVATE alpr_core)
  add_test(NAME core_logic COMMAND test_core)
endif()

# Regression guard: C++ detector + OCR must read the bundled synthetic plate.
if(ALPR_BUILD_APP AND ALPR_BUILD_TESTS)
  add_test(NAME synthetic_image_ocr
    COMMAND alpr --input ${CMAKE_CURRENT_SOURCE_DIR}/samples/synthetic_plate.png
                 --output ${CMAKE_CURRENT_BINARY_DIR}/synthetic_events.csv)
endif()
