set(COMMON_SOURCES ./gd32f4xx_it.c ./main.cpp ./protocal.cpp syscalls.c)
add_subdirectory(uwb)

if(BUILD_VARIANT STREQUAL "MASTER_DEBUG" OR BUILD_VARIANT STREQUAL
                                            "MASTER_RELEASE")
  target_sources(
    ${EXECUTABLE_NAME}
    PRIVATE ${COMMON_SOURCES}
            ./master/master_mode.cpp
            ./master/pc_message.cpp
            ./Enet/netconf.cpp
            ./Enet/enet.cpp
            ./Enet/udp_echo.cpp
            ./Enet/ethernetif.cpp
            ./Enet/sys_arch.c)
  target_include_directories(${EXECUTABLE_NAME}
                             PUBLIC . ./master ./master/include ./Enet)

elseif(BUILD_VARIANT STREQUAL "SLAVE_DEBUG" OR BUILD_VARIANT STREQUAL
                                               "SLAVE_RELEASE")
  target_sources(
    ${EXECUTABLE_NAME}
    PRIVATE ${COMMON_SOURCES} ./slave/src/slave_mode.cpp
            ./slave/src/harness.cpp ./slave/src/msg_proc.cpp
            ./slave/src/peripherals.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./slave/inc)

elseif(BUILD_VARIANT STREQUAL "MASTER_BOARDTEST")
  target_sources(
    ${EXECUTABLE_NAME}
    PRIVATE ${COMMON_SOURCES}
            ./board_test/master/board_test.cpp
            ./Enet/netconf.cpp
            ./Enet/enet.cpp
            ./Enet/udp_echo.cpp
            ./Enet/ethernetif.cpp
            ./Enet/sys_arch.c)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./board_test/master ./Enet)
elseif(BUILD_VARIANT STREQUAL "SLAVE_BOARDTEST")
  target_sources(
    ${EXECUTABLE_NAME}
    PRIVATE ${COMMON_SOURCES} ./board_test/slave/board_test.cpp
            ./board_test/slave/peripherals.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./board_test/slave)
endif()
