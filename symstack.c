#include "symstack.h"

symbtable_stack *stack_create()
{
    symbtable_stack *stack = (symbtable_stack *)malloc(sizeof(symbtable_stack));
    stack->capacity = 1000;
    stack->top = -1;
    stack->array = (SymTable *)malloc(stack->capacity * sizeof(SymTable));
    return stack;
}

Symbol *get_symbol(symbtable_stack *stack, char *name)
{
    for (int i = stack->top; i >= 0; i--)
    {
        Symbol *symbol = symtable_get(&stack->array[i], name);
        if (symbol != NULL)
        {
            return symbol;
        }
    }
    return NULL;
}

void add_symbol(symbtable_stack *stack, Symbol *symbol)
{
    symtable_put(&stack->array[stack->top], symbol->id,symbol);
}

void push_symbtable_stack(symbtable_stack *stack)
{
    if (stack->top == stack->capacity - 1)
    {
        fprintf(stderr, "chyba stacku");
        exit(99);
    }

    SymTable *symtable = symtable_create();
    stack->array[++stack->top] = *symtable;
}

void pop_symbtable_stack(symbtable_stack *stack)
{
    if (stack->top == -1)
    {
        fprintf(stderr, "chyba stacku");
        exit(99);
    }
    stack->top--;
}