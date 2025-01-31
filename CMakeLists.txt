cmake_minimum_required(VERSION 3.30)
project(MKP C)

set(CMAKE_C_STANDARD 23)
include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(mkp_solver
        main.c
        data_structure.c
        utils.c
        local_search.c
        vnd.c
        vns.c
        ml_solver.c
)

target_link_libraries(mkp_solver m)
