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

CellularAutomaton readInitialState(const char* path) {
    CellularAutomaton automaton = {
        .rows = nullptr,
        .num_rows = 0,
        .windX = 0,
        .windY = 0
    };

    FILE* fd = fopen(path, "r");
    if (!fd) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        goto err_close_file;
    } 


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
    for (uint8_t header_index = 0; header_index < num_headers; header_index++) {
        int* header = header_addresses[header_index];
        const size_t numdigits = parseNumber(header_line + idx, header);

        // Failed reading header
        if (numdigits == 0) {
            fprintf(stderr, "ERROR: couldn't parse header number: %hhu\n", header_index + 1);
            goto err_close_file;
        }

        printf("numdigits: %zu\n", numdigits);

        // Success!! Now we check if the next character is a ','
        idx += numdigits;
        if (header_line[idx] != ',') {
            fprintf(stderr, "ERROR: missing ',' after header %hhu\n", header_index + 1);
            goto err_close_file;
        }

        // the next character is a ',', so we bump
        idx++;
    }

    // Handle newline
    switch (header_line[idx]) {
    // Windows moment
    case '\r':
        idx += 2;
        break;

    // Unix newline
    case '\n':
        idx++;
        break;

    
    default:
        fputs("ERROR: missing new line at the end of header section", stderr);
        goto err_close_file;
    }

    if (height < 0) {
        fprintf(stderr, "ERROR: Header value \"height\" has a negative value\n");
        goto err_close_file;
    }
    if (width < 0) {
        fprintf(stderr, "ERROR: Header value \"width\" has a negative value\n");
        goto err_close_file;
    }

    // Correctly typed versions
    size_t h = (size_t)height;
    size_t w = (size_t)width;

    automaton.num_rows = h;
    automaton.rows = malloc(sizeof(CellArray*) * h);
    if (!automaton.rows) {
        fprintf(stderr, "Out Of Memory");
        fclose(fd);
        exit(EXIT_FAILURE);
    }
    automaton.windY = (float)windY / 100.f;
    automaton.windX = (float)windX / 100.f;

    // Allocate memory for the cells
    Cell* cells = malloc(sizeof(Cell) * h * w);
    if (!cells) {
        fprintf(stderr, "Out Of Memory");
        fclose(fd);
        exit(EXIT_FAILURE);
    }

    // Distribute it among the CellArrays
    for (size_t i = 0; i < h; i++) {
        automaton.rows[i] = (CellArray){
            .count = w,
            .elements = cells + (i * w),
        };
    }

    // Parse the cells
    char line[128];
    while (fgets(line, sizeof(line), fd)) {
        puts("reading!");
    }

    if (!feof(fd))
        goto err_failed_read;

    return automaton;

err_failed_read:
    fprintf(stderr, "ERROR: failed to read file \"%s\"\n", path);

err_close_file:
    fclose(fd);
    return (CellularAutomaton){
        .num_rows = 0,
        .rows = nullptr,
        .windX = 0.f,
        .windY = 0.f,
    };
}
