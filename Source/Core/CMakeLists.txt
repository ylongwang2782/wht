set(COMMON_SOURCES ./gd32f4xx_it.c ./main.cpp ./protocal.cpp)
add_subdirectory(uwb)
if(SLAVE)
  target_sources(
    ${EXECUTABLE_NAME} PRIVATE ${COMMON_SOURCES} ./slave/slave_mode.cpp
                               ./slave/harness.cpp ./slave/msg_proc.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./slave)
else()
  target_sources(
    ${EXECUTABLE_NAME} PRIVATE ${COMMON_SOURCES} ./master/master_mode.cpp
                               ./master/pc_message.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./master)
endif()
