#include <cstdio>
#include <cstdlib>

#include "campus_data.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <query>\n", argc > 0 ? argv[0] : "campus_query_cli");
        return 2;
    }

    char buffer[32768];
    const char* result = SearchCampusData(argv[1], buffer, static_cast<int>(sizeof(buffer)));
    if (result == nullptr || *result == '\0') {
        return 1;
    }

    std::puts(result);
    return 0;
}
