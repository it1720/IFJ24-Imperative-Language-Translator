#include <stdio.h>
#include "scanner.h"
#include "error.h"
#include "syntax.h"
#include "expression.h"
#include "codegen.h"

int main() {
    scanTokens(stdin);
    //struct treeNode* expr = bottomUpParse();
    //printTree(expr, 0);
    start_prog();
    //printTokens();
    generate_code();
    return 0;
}

