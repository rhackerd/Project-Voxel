file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(app ${app_sources})

target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../core/include"
)

target_link_libraries(app
    PRIVATE
        ${PROJECT_NAME}
        fmt
)

# Optional: Set output to bin/
set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)


add_custom_target(run
    COMMAND app
    DEPENDS app
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    COMMENT "Running the application..."
)