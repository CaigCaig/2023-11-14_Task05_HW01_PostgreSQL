cmake_minimum_required(VERSION 3.20)

project("2023-11-14_Task05_HW01_PostgreSQL")

add_subdirectory("D:/libpqxx-7.8.1" libpqxx-build) 

add_executable (2023-11-14_Task05_HW01_PostgreSQL "main.cpp")

target_compile_features(2023-11-14_Task05_HW01_PostgreSQL PRIVATE cxx_std_17)
target_link_libraries(2023-11-14_Task05_HW01_PostgreSQL pqxx)