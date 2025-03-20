#ifndef SEMAN_H
#define SEMAN_H
#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "tree.h"
#include "symstack.h"

typedef struct{
    char *id;
    int is_used;
    int is_null;
    int scope;
    int line;
}declared_variables;
void add_ifj_function();
void add_ifj_functions();
void seman_underscore();
void addVariable(struct treeNode *node);
void variable_is_used(char* id);
void check_seman(struct treeNode* root);
int seman_function_call(struct treeNode* node);
int is_it_function(struct treeNode* node);
void seman_pub(struct treeNode* node);
void seman_identifier(struct treeNode *node);
void seman_const_var(struct treeNode *node, struct treeNode *pom);
void seman_while_if(struct treeNode *node);
void seman_return(struct treeNode *node);
void seman_pub_add_functions(struct treeNode *node);
void add_functions(struct treeNode *root);
void checkExpression(struct treeNode *node);
int call_param_type(struct treeNode *node);

symbtable_stack *get_stack();
#endif