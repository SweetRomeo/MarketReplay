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
        tests/version_test.cpp
)

target_link_libraries(marketreplay PRIVATE marketreplay_core)

set_target_properties(marketreplay PROPERTIES
        CXX_EXTENSIONS OFF
)

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
            GIT_SHALLOW TRUE
    )

    if(MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)

    add_executable(marketreplay_tests
            tests/version_test.cpp
    )

    target_link_libraries(marketreplay_tests PRIVATE
            marketreplay_core
            GTest::gtest_main
    )

    set_target_properties(marketreplay_tests PROPERTIES
            CXX_EXTENSIONS OFF
    )

    include(GoogleTest)
    gtest_discover_tests(marketreplay_tests)

    add_test(
            NAME marketreplay_cli_smoke
            COMMAND marketreplay
    )

    set_tests_properties(marketreplay_cli_smoke PROPERTIES
            PASS_REGULAR_EXPRESSION "MarketReplay 0\\.1\\.0 ready"
    )
endif()