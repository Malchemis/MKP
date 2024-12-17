cmake_minimum_required(VERSION 3.30)
project(MKP C)

set(CMAKE_C_STANDARD 23)

# If you have BLAS
find_package(BLAS REQUIRED)
# For GPU, find_package(CUDA) or handle accordingly (not shown here)

include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(mkp_solver
        main.c
        data_structure.c
        utils.c
        local_search.c
        vnd.c
        vns.c
)

target_link_libraries(mkp_solver PUBLIC BLAS::BLAS)
# For GPU, you would also link CUDA/cuBLAS libraries
# target_link_libraries(mkp_solver PUBLIC cublas)
