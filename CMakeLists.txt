project(builder)

hunter_add_package(Boost COMPONENTS program_options filesystem system regex)
hunter_add_package(Async++)
#hunter_add_package(BoostProcess)
find_package(Async++ CONFIG REQUIRED)
#find_package(BoostProcess CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED program_options filesystem system regex)

add_library(builder STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/builder.cpp
)
add_library(process INTERFACE
)
add_executable(main
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/main.cpp
)

target_include_directories(process
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/process/include
)
target_include_directories(builder
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(builder process Async++::Async++
            Boost::program_options Boost::filesystem Boost::system Boost::regex)

target_link_libraries(main builder)
