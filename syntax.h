#ifndef SYNTAX_H
#define SYNTAX_H
#include <stdio.h>
#include <string.h>
#include "scanner.h"
#include "tree.h"
#include "expression.h"

void start_prog();

void parse_prog();

void parse_func_declare();

void parse_func_body();

void parse_body(struct Token*token);

void parse_if_while_body();

void parse_if_while_exp();

void parse_else();

void parse_param();

void parse_next_param();

void parse_func_call_check();

void parse_func_check(struct Token * id);

void parse_call_param();

void parse_call_next_param();

void parse_call_next_param();

void parse_call_func_param(struct Token*token);

void parse_assign();

void parse_expression();

void parse_return_expression();

int is_it_not_type(struct Token *token, struct treeNode*node);
int is_it_not_return_type(struct Token* token, struct treeNode*node);

struct treeNode* get_root();
#endif