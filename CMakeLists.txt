cmake_minimum_required (VERSION 3.8)

project(socket-sessions)

# Load macrothreading
if(NOT TARGET macrothreading)
    # Check to see if the folder is already cloned
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/macrothreading)
        execute_process(COMMAND git pull origin master
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/macrothreading
        )
    else()
        execute_process(COMMAND git clone https://github.com/gregjesl/macrothreading lib/macrothreading
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()

    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/lib/macrothreading")
endif()

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