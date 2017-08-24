cmake_minimum_required (VERSION 3.0)
project (disksurvey)

add_definitions (-g -Werror)
add_subdirectory (libwire)
add_subdirectory (src)
