add_subdirectory(frontend)
add_subdirectory(backend)
add_subdirectory(support)

target_link_libraries(frontend PUBLIC metal-cpp)
target_link_libraries(backend PUBLIC metal-cpp)
target_link_libraries(support PUBLIC metal-cpp)
