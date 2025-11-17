#pragma once
#include <stdio.h>

typedef enum CellState {
    CELLSTATE_NORMAL,
    CELLSTATE_ONFIRE,
    CELLSTATE_BURNT,
} CellState;
const char* cellStateToStr(CellState state);


typedef enum CellType {
    CELLTYPE_TREE,
    CELLTYPE_BUSH,
    CELLTYPE_GRASS,
    CELLTYPE_UNBURNABLE,
} CellType;
const char* cellTypeToStr(CellType type);

typedef struct Cell {
    float moisture;
    float fuel;
    float heat;
    CellState state;
    CellType type;
} Cell;
void printCell(const Cell* cell, FILE* fd);

typedef struct CellArray {
    Cell* elements;
    size_t count;
} CellArray;

typedef struct CellularAutomaton {
    const CellArray* const rows;
    const size_t num_rows;
    float windX;
    float windY;
    /* other stuff maybe */
} CellularAutomaton;
void printAutomaton(const CellularAutomaton* automaton, FILE* fd);

typedef void (*cellProc)(const CellularAutomaton* automaton, size_t row, size_t col, void* userdata);
void forEachCell(const CellularAutomaton* automaton, cellProc fn, void* userdata);
