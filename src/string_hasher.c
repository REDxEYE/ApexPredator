#include <stdio.h>
#include <string.h>

#include "utils/lookup3.h"

#define MAX_LINE_LEN 1024

int main(void) {
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
            len--;
        }
        uint32 hash = hashlittle(line, strlen(line), 0);
        //printf("String: \"%s\" has hash: 0x%08X (%u)\n", line, hash, hash);
        printf("{.hash = 0x%08X, .name = \"%s\"},\n", hash, line);
    }
    return 0;
}