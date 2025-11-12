cmake_minimum_required(VERSION 4.0.2)
project(wildire-spotting C)

set(CMAKE_C_STANDARD 17)

add_executable(wildfire-spotting
    src/main.c
    src/direct_spread.c
    src/spotting_spread.c
    src/display.c
    src/input.c
    src/burnout_cell.c
)

install(TARGETS wildfire-spotting DESTINATION bin)
