cmake_minimum_required(VERSION 3.10)
project(DashAIBrowser LANGUAGES CXX)

# Add subdirectories
add_subdirectory(browser_core)
# Add other subdirectories as needed, e.g., for ASOL if it uses C++

# Global compiler settings (optional)
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable testing with CTest
enable_testing()

# Example: Adding a library or executable (placeholder)
# add_executable(DashAIBrowser main.cpp)
# target_link_libraries(DashAIBrowser PRIVATE browser_core_lib)

# Include CPack for packaging (optional)
# include(CPack)
