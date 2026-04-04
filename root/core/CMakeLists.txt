# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
)

# Create library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(Nova::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Include directory
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        fmt::fmt
        Nova::Logger
)
