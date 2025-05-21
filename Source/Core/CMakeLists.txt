set(COMMON_SOURCES ./gd32f4xx_it.c ./main.cpp ./protocal.cpp)
add_subdirectory(uwb)

if(BUILD_VARIANT STREQUAL "MASTER_DEBUG" OR BUILD_VARIANT STREQUAL "MASTER_RELEASE")
  target_sources(
    ${EXECUTABLE_NAME}
    PRIVATE ${COMMON_SOURCES}
            ./master/master_mode.cpp
            ./master/pc_message.cpp
            ./master/Enet/netconf.cpp
            ./master/Enet/enet.cpp
            ./master/Enet/udp_echo.cpp
            ./master/Enet/ethernetif.cpp
            ./master/Enet/sys_arch.c)
  target_include_directories(${EXECUTABLE_NAME}
                             PUBLIC . ./master ./master/include ./master/Enet)

elseif(BUILD_VARIANT STREQUAL "SLAVE_DEBUG" OR BUILD_VARIANT STREQUAL "SLAVE_RELEASE")
  target_sources(
    ${EXECUTABLE_NAME}
    PRIVATE ${COMMON_SOURCES} ./slave/src/slave_mode.cpp
            ./slave/src/harness.cpp ./slave/src/msg_proc.cpp
            ./slave/src/peripherals.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./slave/inc)

elseif(BUILD_VARIANT STREQUAL "SLAVE_BOARDTEST")
  target_sources(
    ${EXECUTABLE_NAME} PRIVATE ${COMMON_SOURCES} ./board_test/slave/board_test.cpp
                               ./board_test/slave/peripherals.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./board_test/slave)
endif()
