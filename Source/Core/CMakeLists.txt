target_sources(${EXECUTABLE_NAME} PRIVATE
    ./gd32f4xx_it.c
    ./main.cpp

    ./chronolink.cpp
    ./slave/slave_mode.cpp
    ./slave/harness.cpp
    ./master/master_mode.cpp
    ./master/master_json_interface.cpp
    # ./cJSON.c
)

target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ./
    ./slave/
    ./master/
    ../../json/single_include/
)
