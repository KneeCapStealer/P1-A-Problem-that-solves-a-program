#include "input.h"
#include "cell.h"
#include "string.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/// Parse an integer, this function looks a max of 11 bytes ahead
/// The maximum integer number is 10 digits, and a possible negation sign also takes 1 digit.
/// @return Returns the number of bytes read
static size_t parseNumber(const char* buf, int* out) {
    char* end_of_num_ptr = nullptr;

    // Now we use `strtol` to convert it to an int:
    *out = (int)strtol(buf, &end_of_num_ptr, 10);
    return (size_t)(end_of_num_ptr - buf); // number of bytes parsed to an integer
}

/// Used for reading numbers from the input files like so: num1,num2,...,num<num_addresses>
/// The numbers are outputtet to the variables pointet to by `addresses`.
static size_t parseNumberValues(const char* buf, int* const * addresses, size_t num_addresses) {
    size_t idx = 0;
    for (uint8_t value_index = 0; value_index < num_addresses; value_index++) {
        int* value = addresses[value_index];
        // Read a number into the value
        const size_t numdigits = parseNumber(buf + idx, value);

        // If number of digits read is 0, then there was an error
        if (numdigits == 0) {
            fprintf(stderr, "ERROR: couldn't parse value number: %hhu\n", value_index + 1);
            return 0;
        }

        // Success!! Now we check if the next character is a ','
        idx += numdigits;
        if (buf[idx] != ',') {
            fprintf(stderr, "ERROR: missing ',' after value %hhu\n", value_index + 1);
            return 0;
        }

        // the next character is a ',', so we bump
        idx++;
    }

    return idx;
}

CellularAutomaton readInitialState(const char* path) {
    FILE* fd = fopen(path, "r");
    if (!fd) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        goto err_close_file;
    } 

    // Headers
    int width;
    int height;
    int windX;
    int windY;
    int* header_addresses[4] = {&width, &height, &windX, &windY};
    const uint8_t num_headers = sizeof(header_addresses) / sizeof(header_addresses[0]);

    // Load the first line, which holds the headers
    char header_line[128];
    uint8_t idx = 0;
    if(!fgets(header_line, sizeof(header_line), fd))
        goto err_failed_read;

    if (strlen(header_line) == sizeof(header_line)) {
        fputs("ERROR: header line unexpectedly long", stderr);
        goto err_close_file;
    }

    // Parse headers
    const size_t bytes_read = parseNumberValues(header_line, header_addresses, num_headers);
    if (bytes_read == 0) {
        fputs("Failed reading header values", stderr);
        goto err_close_file;
    }
    idx += bytes_read;
    
    if (height < 0) {
        fputs("ERROR: Header value \"height\" has a negative value\n", stderr);
        goto err_close_file;
    }
    if (width < 0) {
        fputs("ERROR: Header value \"width\" has a negative value\n", stderr);
        goto err_close_file;
    }

    // Correctly typed versions
    const size_t h = (size_t)height;
    const size_t w = (size_t)width;
    CellArray* cell_arrays = malloc(sizeof(CellArray) * h);
    if (!cell_arrays) {
        fprintf(stderr, "Out Of Memory\n");
        fclose(fd);
        exit(EXIT_FAILURE);
    }

    // Allocate memory for the cells
    Cell* cells = malloc(sizeof(Cell) * h * w);
    if (!cells) {
        fprintf(stderr, "Out Of Memory\n");
        fclose(fd);
        exit(EXIT_FAILURE);
    }

    // Distribute cell memory among the CellArrays
    for (size_t i = 0; i < h; i++) {
        cell_arrays[i] = (CellArray){
            .count = w,
            .elements = cells + (i * w),
        };
    }

    const CellularAutomaton automaton = {
        .num_rows = h,
        .rows = cell_arrays,
        .windY = (float)windY / 100.f,
        .windX = (float)windX / 100.f,
    };


    // Parse the cells
    char line[128];
    size_t cell_num = 0;
    for (; fgets(line, sizeof(line), fd); cell_num++) {
        if (cell_num >= w * h) {
            fprintf(stderr, "Cell number exceeded number allocated: %zu\n", cell_num); 
            goto err_close_file;
        }

        uint8_t idx = 0;
        const uint8_t line_len = (uint8_t)strlen(line);
        // Line too long
        if (line_len == sizeof(line)) {
            fputs("Line too long", stderr);
            goto err_close_file;
        }

        // Line too short
        if (line_len < strlen("N,T,0,0,0,\n")) {
            fputs("Line too short", stderr);
            goto err_close_file;
        }

        // parse state
        CellState state;
        switch (line[idx]) {
        // Normal
        case 'N':
            state = CELLSTATE_NORMAL;
            break;

        // on Fire
        case 'F':
            state = CELLSTATE_ONFIRE;
            break;

        // Out
        case 'O':
            state = CELLSTATE_BURNT;
            break;

        default:
            fprintf(stderr, "Invalid cell state at cell number: %zu\n", cell_num);
            goto err_close_file;
        }
        idx++;
        if (line[idx] != ',') {
            fprintf(stderr, "Missing comma at cell number: %zu\n", cell_num);
            goto err_close_file;
        }
        idx++;

        // parse type
        CellType type;
        switch (line[idx]) {
        case 'T':
            type = CELLTYPE_TREE;
            break;
        case 'B':
            type = CELLTYPE_BUSH;
            break;
        case 'G':
            type = CELLTYPE_GRASS;
            break;
        case 'U':
            type = CELLTYPE_UNBURNABLE;
            break;
        default: 
            fprintf(stderr, "Invalid cell type at cell number: %zu\n", cell_num);
            goto err_close_file;
        }
        idx++;
        if (line[idx] != ',') {
            fprintf(stderr, "Missing comma at cell number: %zu\n", cell_num);
            goto err_close_file;
        }
        idx++;

        // parse number values
        int fuel;
        int heat;
        int moisture;

        int* value_addresses[3] = {&fuel, &heat, &moisture};
        const size_t num_values = sizeof(value_addresses) / sizeof(value_addresses[0]);

        const size_t bytes_read = parseNumberValues(line + idx, value_addresses, num_values);
        if (bytes_read == 0) {
            fprintf(stderr, "Error reading number values at cell number: %zu\n", cell_num );
            goto err_close_file;
        }

        // Done parsing the cell!!!
        if (fuel < 0) {
            fprintf(stderr, "Fuel at cell %zu, was set to a negative value!\n", cell_num);
            goto err_close_file;
        }
        if (heat < 0) {
            fprintf(stderr, "Heat at cell %zu, was set to a negative value!\n", cell_num);
            goto err_close_file;
        }
        if (moisture < 0) {
            fprintf(stderr, "Moisture at cell %zu, was set to a negative value!\n", cell_num);
            goto err_close_file;
        }

        // convert numbers to floats
        const float f = (float)fuel / 100.f;
        const float h = (float)heat / 100.f;
        const float m = (float)moisture / 100.f;

        cells[cell_num] = (Cell){
            .state = state,
            .type = type,
            .fuel = f,
            .heat = h,
            .moisture = m,
        };
    }

    if (cell_num < w * h) {
        fprintf(stderr, "Not enough cells.\nGot %zu cells\nGridsize: %zu * %zu = %zu\n", cell_num, w, h, w * h);
        goto err_close_file;
    }

    if (!feof(fd))
        goto err_failed_read;

    return automaton;

// ez pz error handling in c
// super useful
err_failed_read:
    fprintf(stderr, "ERROR: failed to read file \"%s\"\n", path);

err_close_file:
    fclose(fd); // Now we will never forget to close the file
    return (CellularAutomaton){
        .num_rows = 0,
        .rows = nullptr,
        .windX = 0.f,
        .windY = 0.f,
    };
}
