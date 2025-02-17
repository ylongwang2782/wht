add_subdirectory(FreeRTOS)
add_subdirectory(FreeRTOScpp)

target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ./json/single_include/nlohmann/
)
