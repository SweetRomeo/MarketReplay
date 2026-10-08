cmake_minimum_required(VERSION 3.20)

project(MarketReplay VERSION 0.1.0 LANGUAGES CXX)

add_library(marketreplay_core STATIC
        src/version.cpp
)

target_include_directories(marketreplay_core PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_features(marketreplay_core PUBLIC cxx_std_20)

set_target_properties(marketreplay_core PROPERTIES
        CXX_EXTENSIONS OFF
)

add_executable(marketreplay
        src/main.cpp
        include/marketreplay/version.hpp
)

target_link_libraries(marketreplay PRIVATE marketreplay_core)

set_target_properties(marketreplay PROPERTIES
        CXX_EXTENSIONS OFF
)

include(CTest)

if(BUILD_TESTING)
    add_test(
            NAME marketreplay_cli_smoke
            COMMAND marketreplay
    )

    set_tests_properties(marketreplay_cli_smoke PROPERTIES
            PASS_REGULAR_EXPRESSION "MarketReplay 0\\.1\\.0 ready"
    )
endif()