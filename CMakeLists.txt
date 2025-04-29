cmake_minimum_required(VERSION 3.19)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

# 定义可选构建变体
set(BUILD_VARIANTS
    "MASTER;SLAVE;DEMO"
    CACHE STRING "Available build variants: MASTER, SLAVE, DEMO")
set_property(CACHE BUILD_VARIANTS PROPERTY STRINGS MASTER SLAVE DEMO)

# 设置默认变体
if(NOT BUILD_VARIANT)
  set(BUILD_VARIANT
      "SLAVE"
      CACHE STRING "Selected build variant")
endif()

# Include toolchain file
include("CMake/arm-none-eabi-gcc.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

enable_language(ASM C CXX)

# 设置项目名称和版本号
project(wht VERSION 3.0.0)

add_compile_options(
  -Wno-unused-parameter # 忽略未使用的参数警告
  -Wno-pedantic # 忽略严格模式的警告
  -Wno-unused-variable)

add_link_options(-Wl,--unresolved-symbols=ignore-in-object-files)

add_definitions(-DGD32F470 -DGCC)

add_subdirectory(Source)
