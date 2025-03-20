
#ifndef SYMSTACK_H
#define SYMSTACK_H

#include "symtable.h"

typedef struct
{
    int top;
    unsigned capacity;
    SymTable *array;
} symbtable_stack;

symbtable_stack *stack_create();

Symbol *get_symbol(symbtable_stack *stack, char *name);

void add_symbol(symbtable_stack *stack, Symbol *symbol);

void push_symbtable_stack(symbtable_stack *stack);

void pop_symbtable_stack(symbtable_stack *stack);

#endif