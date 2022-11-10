project(libmx LANGUAGES C)
cmake_minimum_required(VERSION 3.23)

find_package(Eigen3 CONFIG REQUIRED)
find_package(MKL CONFIG REQUIRED)

add_library(mx SHARED
    source/ao/analysis/aoAtmosphere.cpp
    source/ao/analysis/aoPSDs.cpp
    source/ao/analysis/aoSystem.cpp
    source/ao/analysis/clGainOpt.cpp
    source/ao/analysis/fourierTemporalPSD.cpp
    source/app/appConfigurator.cpp
    # source/app/application.cpp
    # source/app/clOptions.cpp
    # source/app/ini.cpp
    # source/app/optionparser.cpp
)

target_include_directories(mx PUBLIC include $<TARGET_PROPERTY:MKL::MKL,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(mx PRIVATE Eigen3::Eigen $<LINK_ONLY:MKL::MKL>)

install(TARGETS mx DESTINATION lib)
