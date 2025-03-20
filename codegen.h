//
// Created by matejbucek on 9.10.24.
//

#ifndef IFJ24_CODEGEN_H
#define IFJ24_CODEGEN_H

typedef struct treeNode treeNode;

void codegen_init(int allow_debug_output);
void codegen_destroy();
void generate_code();

//
void codegen_printf(const char *format, ...);

// Tree dependent functions
void traverse_tree(treeNode *root);
void generate_temporary_expressions(treeNode *root, char **result);
void generate_assignment(const char *identifier, const char *expression_result);

// Code generation functions
void generate_preamble();
void generate_jump_to_main();
void generate_function(const char *label, treeNode *function_node);
void generate_function_call(const char *result, treeNode *function_name_node,
                            int is_special);
void generate_debug_comment(const char *message);
void generate_createframe();
void generate_pushframe();
void generate_popframe();
void generate_defvar(const char *var);
void generate_move(const char *var, const char *value);
void generate_add(const char *var, const char *op1, const char *op2);
void generate_sub(const char *var, const char *op1, const char *op2);
void generate_mul(const char *var, const char *op1, const char *op2);
void generate_div(const char *var, const char *op1, const char *op2);

void generate_builtin_functions();

// Helper functions
void escape_string();
int starts_with(const char *str, const char *prefix);
char *sanitize_string(const char *str);

// Argument translation
typedef struct ArgumentTranslation {
  char *internal_name;
  char *original_name;
} ArgumentTranslation;

typedef struct ArgumentTranslationContext {
  ArgumentTranslation *translations;
  int count;
  int capacity;
} ArgumentTranslationContext;

ArgumentTranslationContext *argument_translation_context_create();
void argument_translation_context_destroy(ArgumentTranslationContext **context);
void argument_translation_context_add(ArgumentTranslationContext *context,
                                      const char *internal_name,
                                      const char *original_name);
const char *
argument_translation_context_get(ArgumentTranslationContext *context,
                                 const char *original_name);
void dump_translation_context(ArgumentTranslationContext *context);

char *translate_if_possible(const char *original_name);

// Scope stack

typedef enum ScopeType { Function, If, While, None } ScopeType;

typedef struct ScopeStackNode {
  ScopeType type;
  struct ScopeStackNode *previous;
} ScopeStackNode;

typedef struct ScopeStack {
  ScopeStackNode *top;
} ScopeStack;

ScopeStack *scope_stack_create();
void scope_stack_destroy(ScopeStack **stack);
void scope_stack_push(ScopeStack *stack, ScopeType type);
ScopeType scope_stack_pop(ScopeStack *stack);

#endif // IFJ24_CODEGEN_H
