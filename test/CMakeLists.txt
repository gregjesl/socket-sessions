cmake_minimum_required (VERSION 3.8)

file(GLOB tests
    "*.c"
)

foreach(test ${tests})
    string(REGEX REPLACE ".*/" "" test_name "${test}")
    string(REGEX REPLACE ".c$" "" test_name "${test_name}")
    set(full_name "${PROJECT_NAME}_${test_name}")
    add_executable ("${full_name}_test" ${test})
    add_test(NAME "${full_name}" COMMAND "${full_name}_test")
    target_link_libraries ("${full_name}_test" ${PROJECT_NAME})
endforeach()