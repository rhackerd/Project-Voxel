file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
)

add_library(module SHARED ${MODULE_SOURCES})
add_library(Nova::module ALIAS module)

target_include_directories(module
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)


set_target_properties(module PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_include_directories(module PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../core/include"
)

target_compile_features(module PUBLIC cxx_std_20)

target_link_libraries(module
    PRIVATE
        Nova::template_cmake
)