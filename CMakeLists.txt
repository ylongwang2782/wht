cmake_minimum_required(VERSION 3.19)

string(TIMESTAMP BUILD_DATE "%Y%m%d")
set(OUTPUT_SUFFIX "_${BUILD_DATE}")

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

enable_language(ASM C CXX)

# 设置项目名称和版本号

if(BUILD_VARIANT STREQUAL "MASTER")
project(MasterFirmware VERSION 3.0)
elseif(BUILD_VARIANT STREQUAL "SLAVE")
project(SlaveFirmware VERSION 3.0)
elseif(BUILD_VARIANT STREQUAL "BOARDTEST")
project(BoardTestFirmware VERSION 3.1)
else()
  message(FATAL_ERROR "Unknown BUILD_VARIANT value: ${BUILD_VARIANT}")
endif()

add_compile_options(
  -Wno-unused-parameter # 忽略未使用的参数警告
  -Wno-pedantic # 忽略严格模式的警告
  -Wno-unused-variable)

add_link_options(-Wl,--unresolved-symbols=ignore-in-object-files)

add_definitions(-DGD32F470 -DGCC)

add_subdirectory(Source)
