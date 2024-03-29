cmake_minimum_required (VERSION 3.11)

project(socket-sessions)

# Get the source files
file(GLOB sources "src/*.c")

if(ESP_PLATFORM)
    if(__idf_component_context)
        idf_component_register(
            SRCS ${sources}
            INCLUDE_DIRS "inc"
        )
    else()
        set(IS_ESP_COMPONENT True)
    endif()
else()
    if(MSVC)
        # ignore warnings about scanf
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        add_compile_options(/W4 /WX)
    else()
        add_compile_options(-Wall -Wextra -Werror)
    endif()

    # Load macrothreading
    if(NOT TARGET macrothreading)
        include(FetchContent)
        FetchContent_Declare(macrothreading_lib
            GIT_REPOSITORY https://github.com/gregjesl/macrothreading
        )
        FetchContent_Populate(macrothreading_lib)
        add_subdirectory(${macrothreading_lib_SOURCE_DIR} build)
    endif()

    # Create the library
    add_library(${PROJECT_NAME} ${sources})

    # Include directory
    target_include_directories(${PROJECT_NAME} PUBLIC inc)

    # Link the library
    if(NOT TARGET macrothreading)
        message(FATAL_ERROR "No macrothreading library found")
    endif()
    target_link_libraries (${PROJECT_NAME} PUBLIC macrothreading)

    # Check for top-level project
    if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()