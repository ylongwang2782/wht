set(COMMON_SOURCES ./gd32f4xx_it.c ./main.cpp ./protocal.cpp)

if(SLAVE)
  target_sources(
    ${EXECUTABLE_NAME} PRIVATE ${COMMON_SOURCES} ./slave/slave_mode.cpp
                               ./slave/harness.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC . ./slave)
else()
  target_sources(${EXECUTABLE_NAME} PRIVATE ${COMMON_SOURCES}
                                            ./master/master_mode.cpp)
  target_include_directories(${EXECUTABLE_NAME} PUBLIC ./master)
endif()
