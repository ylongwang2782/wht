cmake_minimum_required(VERSION 3.19)

string(TIMESTAMP BUILD_DATE "%Y%m%d")
set(OUTPUT_SUFFIX "_${BUILD_DATE}")

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

enable_language(ASM C CXX)

# 获取 Git 标签作为版本号
execute_process(
  COMMAND git describe --tags --abbrev=0
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_TAG
  OUTPUT_STRIP_TRAILING_WHITESPACE)

# 提取 MAJOR, MINOR, PATCH
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" VERSION_MATCH ${GIT_TAG})
if(VERSION_MATCH)
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
                       PROJECT_VERSION_MAJOR ${VERSION_MATCH})
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
                       PROJECT_VERSION_MINOR ${VERSION_MATCH})
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3"
                       PROJECT_VERSION_PATCH ${VERSION_MATCH})
else()
  message(WARNING "Invalid git tag: ${GIT_TAG}, using default version 0.0.0")
  set(PROJECT_VERSION_MAJOR 0)
  set(PROJECT_VERSION_MINOR 0)
  set(PROJECT_VERSION_PATCH 0)
endif()

# 设置项目名称和版本号（保留你的 BUILD_VARIANT 结构）
if(BUILD_VARIANT STREQUAL "MASTER_DEBUG")
  project(
    MasterFirmware
    VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
elseif(BUILD_VARIANT STREQUAL "MASTER_RELEASE")
  project(
    MasterFirmwareRelease
    VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
elseif(BUILD_VARIANT STREQUAL "SLAVE_DEBUG")
  project(
    SlaveFirmware
    VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
elseif(BUILD_VARIANT STREQUAL "SLAVE_RELEASE")
  project(
    SlaveFirmwareRelease
    VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
elseif(BUILD_VARIANT STREQUAL "MASTER_BOARDTEST")
  project(
    MasterBoardTestFirmware
    VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
elseif(BUILD_VARIANT STREQUAL "SLAVE_BOARDTEST")
  project(
    SlaveBoardTestFirmware
    VERSION
      ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
  )
else()
  message(FATAL_ERROR "Unknown BUILD_VARIANT value: ${BUILD_VARIANT}")
endif()

add_compile_options(
  -Wno-unused-parameter # 忽略未使用的参数警告
  -Wno-pedantic # 忽略严格模式的警告
  -Wno-unused-variable)

add_link_options(-Wl,--unresolved-symbols=ignore-in-object-files)

add_definitions(-DGD32F470 -DGCC -DUSE_STDPERIPH_DRIVER)

add_subdirectory(Source)
