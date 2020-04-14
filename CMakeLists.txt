cmake_minimum_required (VERSION 3.8)

project(socket-sessions)

set(SOCKET_SESSIONS_LIBRARY_NAME ${PROJECT_NAME})

# Load macrothreading
if(NOT MACROTHREADING_LIBRARY_NAME)
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
    # Include directory
    include_directories(inc)

    # Create the library
    add_library(${SOCKET_SESSIONS_LIBRARY_NAME} ${sources})

    # Link the library
    target_link_libraries (${SOCKET_SESSIONS_LIBRARY_NAME} ${MACROTHREADING_LIBRARY_NAME})

    # Check for top-level project
    if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
        enable_testing()
        add_subdirectory(test)
    endif()
endif()