cmake_minimum_required(VERSION 3.19)

# 使用字符串连接操作生成可执行文件的名称
set(EXECUTABLE_NAME "${PROJECT_NAME}_v${PROJECT_VERSION}")

# 添加编译选项
add_compile_options(
  -Wno-unused-parameter # 忽略未使用的参数警告
  -Wno-pedantic # 忽略严格模式的警告
)

# 添加可执行文件目标
add_executable(${EXECUTABLE_NAME})

# 设置目标的私有包含目录
include_directories(Config # 配置文件目录
)

# 定义可选构建变体
set(BUILD_VARIANT
    "MASTER;SLAVE;BOARDTEST"
    CACHE STRING "Available build variants: MASTER, SLAVE, BOARDTEST")
set_property(CACHE BUILD_VARIANT PROPERTY STRINGS MASTER SLAVE BOARDTEST)

if(BUILD_VARIANT STREQUAL "MASTER")
  target_compile_definitions(${EXECUTABLE_NAME} PRIVATE MASTER
                                                        BUILD_VARIANT=MASTER)
elseif(BUILD_VARIANT STREQUAL "SLAVE")
  target_compile_definitions(${EXECUTABLE_NAME} PRIVATE SLAVE
                                                        BUILD_VARIANT=SLAVE)
elseif(BUILD_VARIANT STREQUAL "BOARDTEST")
  target_compile_definitions(${EXECUTABLE_NAME} PRIVATE BOARDTEST
                                                        BUILD_VARIANT=BOARDTEST)
else()
  message(FATAL_ERROR "Unknown BUILD_VARIANT value: ${BUILD_VARIANT}")
endif()

# 添加公共宏
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE GD32F470)

add_subdirectory(BSP)
add_subdirectory(Core)
add_subdirectory(Drivers)
add_subdirectory(Middlewares)
# add_subdirectory(Retarget)

# 链接目标与其他库
target_link_libraries(${EXECUTABLE_NAME} BSP Drivers freertos_kernel
                      FreeRTOScpp)

# ===============================================================================

# 设置可执行文件的生成位置
set(EXECUTABLE_OUTPUT_PATH
    ${PROJECT_SOURCE_DIR}/bin/${BUILD_VARIANT}/${CMAKE_BUILD_TYPE}/)

# 指定map文件生成路径
target_link_options(
  ${EXECUTABLE_NAME} PRIVATE
  "-Wl,-Map=$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/${EXECUTABLE_NAME}.elf.map")

# 添加编译后命令-生成bin和Hex格式固件、打印可执行文件尺寸信息
include(${PROJECT_SOURCE_DIR}/CMake/create_bin.cmake)

# 设置cmake install指令，烧录程序
include(${PROJECT_SOURCE_DIR}/CMake/flash_by_install.cmake)
