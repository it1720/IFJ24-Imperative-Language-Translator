#ifndef TREE_H
#define TREE_H
#include <stdlib.h>
#include <stdio.h>
#include "scanner.h"
#include "error.h"

struct treeNode
{
    int isExpression;
    struct treeNode* left;
    struct treeNode* right;
    struct treeNode* parent;
    struct Token* token;
    enum TokenType ExprType;
    int isRel;
};

struct treeNode* createNode(struct Token* token);

void printTree(struct treeNode* tree, int space);
#endif //TREE_H