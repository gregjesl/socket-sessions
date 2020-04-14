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