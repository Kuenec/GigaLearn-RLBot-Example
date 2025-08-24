cmake_minimum_required (VERSION 3.8) # Consider updating to 3.10+ to remove deprecation warnings

project("GigaLearnBot")

# Define sources for the main GigaLearnBot executable
file(GLOB_RECURSE GIGALEARNBOT_FILES_SRC "src/*.cpp" "src/*.h" "src/*.hpp")
list(REMOVE_ITEM GIGALEARNBOT_FILES_SRC
     "${CMAKE_CURRENT_SOURCE_DIR}/src/rlbotmain.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/RLBotClient.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/RLBotClient.h"
)
add_executable(GigaLearnBot ${GIGALEARNBOT_FILES_SRC})

# Define sources for the new rlbot executable
# MODIFIED LINE: Added "src/RSVClient.cpp" to the list of sources
add_executable(rlbot "src/rlbotmain.cpp" "src/RLBotClient.cpp" "src/RLBotClient.h" "src/RSVClient.cpp")


# Set C++ version to 20 for GigaLearnBot
set_target_properties(GigaLearnBot PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(GigaLearnBot PROPERTIES CXX_STANDARD 20)
set_target_properties(GigaLearnBot PROPERTIES CXX_STANDARD_REQUIRED ON)

# Set C++ version to 20 for rlbot
set_target_properties(rlbot PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(rlbot PROPERTIES CXX_STANDARD 20)
set_target_properties(rlbot PROPERTIES CXX_STANDARD_REQUIRED ON)


# Make sure GigaLearnCPP is going to build in the same directory as us
# Otherwise, we won't be able to import it at runtime
set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

# Include GigaLearnCPP
add_subdirectory(GigaLearnCPP)
target_link_libraries(GigaLearnBot GigaLearnCPP)
target_link_libraries(rlbot GigaLearnCPP)

# Include RLBotCPP
add_subdirectory(RLBotCPP)
target_link_libraries(GigaLearnBot RLBotCPP)
target_link_libraries(rlbot RLBotCPP)