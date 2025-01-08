target_sources(${EXECUTABLE_NAME} PRIVATE
    ./gd32f4xx_it.c
    ./main.cpp

    ./chronolink.cpp
    ./slave/slave_mode.cpp
    ./slave/harness.cpp
    ./master/master_mode.cpp
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ./
    ./slave/
)
