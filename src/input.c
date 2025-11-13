#include "input.h"
#include "cell.h"
#include "string.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

char* findChar(char c);

/// Parse an integer, this function looks a max of 11 bytes ahead
/// The maximum integer number is 10 digits, and a possible negation sign also takes 1 digit.
/// @return Returns the number of bytes read
uint8_t parseNumber(const char* buf, size_t max_len, int* out);

typedef enum ParseState {
    PARSE_STATE_HEADER_VALUE,
    PARSE_STATE_SETUP_AUTOMATON,
    PARSE_STATE_CELLS,
} ParseState;

CellularAutomaton readInitialState(const char* path) {
    CellularAutomaton automaton = {
        .rows = nullptr,
        .num_rows = 0,
        .windX = 0,
        .windY = 0
    };

    FILE* fd = fopen(path, "r");
    if (fd == nullptr){
        printf("Failed to open file: %s\n", path);
        goto _err;
    } 

    ParseState state = PARSE_STATE_HEADER_VALUE;

    int width, height, windX, windY;
    int* header_addresses[4] = {&width, &height, &windX, &windY};
    uint8_t header_index = 0;

    char buf[128];
    uint8_t buf_size = 0;
    uint8_t buf_pos = 0;

    do {
        // If we havn't used all the bytes in the buffer yet
        // We copy them to the beginning
        uint8_t unused_bytes = buf_size - buf_pos;
        memcpy(buf, buf + buf_pos, unused_bytes);

        size_t num_bytes = fread(buf + unused_bytes, 1, sizeof(buf) - unused_bytes, fd);
        if (num_bytes == 0) continue;

        // The bytes we copied and the bytes we read
        buf_size = unused_bytes + num_bytes;
        buf_pos = 0;

parse_next:
        // No more bytes :( try to read more
        if (buf_pos >= buf_size) continue;

        switch (state) {
            // Parse a header value to the output pointed to by: header_address
            case PARSE_STATE_HEADER_VALUE:
                // The number of bytes left in the buffer
                size_t bytes_left = buf_size - buf_pos;
                uint8_t bytes_parsed = parseNumber(buf + buf_pos, bytes_left, header_addresses[header_index]);

                // We reached the end of the buffer, the number might be incomplete
                if (buf_pos + bytes_parsed >= buf_size) {
                    // We have reached the end of the file without completing the headers, this is an invalid file
                    if (feof(fd)) {
                        printf("ERROR: end of input file reached before parsing headers was complete\nStopped at header number: %d", header_index);
                        goto _err;
                    }

                    // We havn't reached the end of the file,
                    // we should therefore read more bytes into the buffer and try again
                    continue;
                }

                // We havn't reached the end of the buffer, the number must be valid!
                buf_pos += bytes_parsed;


                // Are there more headers?
                if (++header_index < sizeof(header_addresses)) {
                    // Since there are more headers, we need a ',' to seperate the values
                    if (buf[buf_pos] != ',') {
                        printf("ERROR: invalid char while parsing input headers: %c", buf[buf_pos]);
                        goto _err;
                    }

                    // The character was a ',' so we bump the buf_pos
                    buf_pos++;

                    goto parse_next;
                }

                // Check if we ran out of buffer space
                state = PARSE_STATE_SETUP_AUTOMATON;
                goto parse_next;

            // END OF PARSE_STATE_HEADER_VALUE

            case PARSE_STATE_SETUP_AUTOMATON:
                // No more headers, require newline at the end of header section:
                char header_end = buf[buf_pos];
                if (header_end != '\n' && header_end != '\r') {
                    printf("ERROR: missing newline at the end of header section");
                    goto _err;
                }

                // newline found, if it's a windows "\r\n" go 2 bytes forward. If it's a Unix '\n' go 1 byte forward
                if (header_end == '\r') {
                    // We need to bump 2 bytes, however there might not be enough buffer space for it
                    // Truly a Windows moment
                    buf_pos++;
                    // I'm lazy so I'll just reset this switch case.
                    // Then the `header_end` variable will be equal to '\n'
                    // Which whon't need any checking :+1:
                    goto parse_next;
                }
                // bump the '\n'
                buf_pos++;

                // Now we don't need to do any more reading from the buffer in this step.
                // We can safely setup the automaton
                automaton.num_rows = height;
                automaton.rows = malloc(sizeof(CellArray*) * height);
                if (!automaton.rows) {
                    printf("Out Of Memory");
                    fclose(fd);
                    exit(EXIT_FAILURE);
                }
                automaton.windY = (float)windY / 100.f;
                automaton.windX = (float)windX / 100.f;

                // Allocate memory for the cells
                Cell* cells = malloc(sizeof(Cell) * height * width);
                if (cells == nullptr) {
                    printf("Out Of Memory");
                    fclose(fd);
                    exit(EXIT_FAILURE);
                }

                // Distribute it among the CellArrays
                for (size_t i = 0; i < height; i++) {
                    automaton.rows[i] = (CellArray){
                        .count = width,
                        .elements = cells + (i * width),
                    };
                }

                state = PARSE_STATE_CELLS;
                goto parse_next;

            // END OF PARSE_STATE_SETUP_AUTOMATON

            case PARSE_STATE_CELLS:

                break;
        }

    } while(!feof(fd) || ferror(fd));


_err:
    if (fd) fclose(fd);
    return (CellularAutomaton) {
        .rows = nullptr,
        .num_rows = 0,
        .windX = 0,
        .windY = 0
    };
}
