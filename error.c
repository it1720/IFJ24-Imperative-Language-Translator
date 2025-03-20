#include "error.h"

void error(int code, int line, const char* message){
    fprintf(stderr, "Error: %d %s at line %d\n",code, message, line);
    exit(code);
}