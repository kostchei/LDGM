cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0091 NEW)

project(LDGMChronoSmoke LANGUAGES CXX)

find_package(Chrono COMPONENTS Vehicle CONFIG REQUIRED)

add_executable(ldgm_chrono_smoke main.cpp)
target_compile_features(ldgm_chrono_smoke PRIVATE cxx_std_17)
target_link_libraries(ldgm_chrono_smoke PRIVATE ${CHRONO_TARGETS})

if(MSVC)
    set_target_properties(
        ldgm_chrono_smoke
        PROPERTIES MSVC_RUNTIME_LIBRARY ${CHRONO_MSVC_RUNTIME_LIBRARY}
    )
endif()

enable_testing()
add_test(NAME chrono_core_vehicle_smoke COMMAND ldgm_chrono_smoke)
