cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_COVERAGE "Build coverage" OFF)

set(
  HUNTER_CACHE_SERVERS
  "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
  CACHE STRING "Default cache server"
)

include("cmake/HunterGate.cmake")
include_directories(process/include/boost)
include_directories(process/include)

HunterGate(
  URL "https://github.com/cpp-pm/hunter/archive/v0.23.218.tar.gz"
  SHA1 "9a3594a61227967fb92159339ba326701b287b19"
)

project(Process)

#hunter_add_package(BoostProcess)
#find_package(BoostProcess CONFIG REQUIRED)
hunter_add_package(Async++)
find_package(Async++ CONFIG REQUIRED)
hunter_add_package(Boost COMPONENTS program_options system)
#find_package (Threads)
#find_package (Boost COMPONENTS log_setup)
find_package(Boost CONFIG REQUIRED program_options system)





add_executable(builder
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/builder.cpp
)




#target_include_directories(picosha2
 # INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
  #          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

#target_link_libraries(main PUBLIC BoostProcess::boost_process)
target_link_libraries(builder Boost::program_options Async++::Async++ Boost::system)
target_link_libraries(builder)
