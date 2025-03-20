#include "tree.h"

struct treeNode* createNode(struct Token* token) {
    struct treeNode* node = (struct treeNode*)malloc(sizeof(struct treeNode));
    node->token = token;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->isExpression = 0;
    node->isRel = 0;
    enum TokenType ExprType = token->type;
    return node;
}

void printTree(struct treeNode *root, int space)
{
    if (root == NULL)
    {
        return;
    }

    // Increase distance between levels
    int indent = 5;
    space += indent;

    // Process right child first
    printTree(root->right, space);

    // Print current node after space
    fprintf(stderr, "\n");
    for (int i = indent; i < space; i++)
    {
      fprintf(stderr, " ");
    }
    fprintf(stderr, "%s\n", root->token->lexeme);

    // Process left child
    printTree(root->left, space);
}