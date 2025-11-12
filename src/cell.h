#pragma once
#include <stddef.h>
#include <inttypes.h>

typedef enum CellState {
    CELLSTATE_NORMAL,
    CELLSTATE_ONFIRE,
    CELLSTATE_BURNT,
} CellState;

typedef struct Cell {
    float moisture_level;
    float fuel_amount;
    float heat;
    CellState state;
} Cell;

typedef struct CellArray {
    Cell* elements;
    size_t count;
} CellArray;

typedef struct CellularAutomaton {
    CellArray cells;
    /* other stuff maybe */
} CellularAutomaton;
