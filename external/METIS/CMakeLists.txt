# Specify the version being used as well as the language
cmake_minimum_required(VERSION 3.10)

#------------------------------------------------------------------------------------#
# CMake targets
#------------------------------------------------------------------------------------#
add_custom_target(clean-external-metis COMMAND ${CMAKE_MAKE_PROGRAM} clean WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

install(FILES "cmake/FindMETIS.cmake" DESTINATION "${CMAKE_INSTALL_PREFIX}/${BITPIT_INSTALL_CMAKE_DIR}")
