#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "scanner.h"
#include "error.h"
#include <stdlib.h>
#include <stdio.h>
#include "tree.h"

enum Action {
    SHIFT, REDUCE, PREC_EQUAL, ERROR, FINAL
};

struct exprDLL {
    struct treeNode* node;
    struct exprDLL* next;
    struct exprDLL* prev;
    struct exprDLL* active;
};

struct treeNode* bottomUpParse();

#endif //EXPRESSION_H
