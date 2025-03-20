//
// Created by matejbucek on 9.10.24.
//

#include "codegen.h"
#include "MemoryMap.h"
#include "error.h"
#include "seman.h"
#include "symstack.h"
#include "syntax.h"
#include "tree.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

static int s_allow_debug_output = 0;
static MemoryMap *s_memory_map;
static int s_label_counter = 0;
static ArgumentTranslationContext *s_argument_translation_context;

static ScopeStack *s_scope_stack;
static treeNode *scope_end = NULL;

static int if_counter = 0;
static int while_counter = 0;
static int while_current = 0;

static int output_only_defvar = 0;
static int output_no_defvar = 0;

static int in_main = 0;

static int rewrite_values = 0;
static enum TokenType rewrite_numbers_to = NUMBER;

void generate_arguments(struct treeNode *parameter_start);
void generate_binary_expression(char *return_variable,
                                struct treeNode *expression_node, int helper_id,
                                char *helper_prefix);
void codegen_init(int allow_debug_output) {
  s_allow_debug_output = allow_debug_output;
  s_memory_map = memory_map_create();
  s_scope_stack = scope_stack_create();
}

void generate_code() {
  codegen_init(1);
  treeNode *root = get_root();
  if (root == NULL) {
    error(99, 0, "Root AST is NULL");
  }

  symbtable_stack *stack = get_stack();
  if (stack == NULL) {
    error(99, 0, "Symbol table stack is NULL");
  }

  generate_preamble();
  generate_jump_to_main();

  generate_builtin_functions();

  traverse_tree(root);
}

void traverse_tree(treeNode *root) {
  if (root == NULL) {
    return;
  }

  if (root->right != NULL) {
    treeNode *current = root->right;
    switch (current->token->type) {
      // Function definition
    case PUB:
      generate_debug_comment("Function definition start");
      generate_function(current->right->token->lexeme, current->left);
      if (!in_main && !strcmp(current->right->token->lexeme, "main")) {
        in_main = 1;
      }
      scope_stack_push(s_scope_stack, Function);
      break;
      // Variable definition
    case CONST:
    case VAR:
      // Check if the variable is not const ifj = @import...
      if (current->right && !strcmp(current->right->token->lexeme, "ifj") &&
          current->left && current->left->left &&
          !strcmp(current->left->left->token->lexeme, "@")) {
        fprintf(stderr, "Skipping the import\n");
        break;
      }

      char *result = malloc(256);
      generate_debug_comment("Variable definition, temporary expression");
      generate_temporary_expressions(current->left, &result);
      if (strlen(result) == 0) {
        break;
      }
      char *identifier = malloc(256);
      sprintf(identifier, "LF@%s", current->right->token->lexeme);
      generate_debug_comment("Variable definition, assignment");
      generate_assignment(identifier, result);
      free(identifier);
      free(result);
      break;
      // Function call
    case IDENTIFIER:
      if (!strcmp(current->left->token->lexeme, "(") ||
          (current->left && current->left->left &&
           !strcmp(current->left->left->token->lexeme, "("))) {
        generate_debug_comment("Function call");
        int is_special = starts_with(current->token->lexeme, "ifj.");
        if (is_special) {
          generate_function_call(NULL, current->left, is_special);
        } else {
          generate_function_call(NULL, current, is_special);
        }
      } else {
        // Assignment
        generate_debug_comment("Assignment");
        char *assignment_tmp_result = malloc(256);
        char *assignment_result = malloc(256);
        sprintf(assignment_result, "LF@%s", current->token->lexeme);
        generate_temporary_expressions(current->left, &assignment_tmp_result);
        if (strcmp(current->token->lexeme, "_") != 0) {
          generate_assignment(assignment_result, assignment_tmp_result);
        }
        free(assignment_result);
      }
      break;
    case IF:
      generate_debug_comment("If statement");
      int if_current = if_counter++;
      // Define the condition variable
      char *condition_variable = malloc(256);
      sprintf(condition_variable, "LF@%%condition_%d", if_current);

      generate_defvar(condition_variable);

      // Generate if not null
      if (current->left->right->token->type == IDENTIFIER ||
          current->right->token->type == PIPE) {
        char *user_var = malloc(256);
        sprintf(user_var, "LF@%s", current->left->right->token->lexeme);
        codegen_printf("EQ %s %s nil@nil\n", condition_variable,
                       translate_if_possible(user_var));
        codegen_printf("NOT %s %s\n", condition_variable, condition_variable);
        char *non_null_var = malloc(256);
        sprintf(non_null_var, "LF@%s", current->right->left->token->lexeme);
        generate_defvar(non_null_var);
        generate_move(non_null_var, translate_if_possible(user_var));
        free(non_null_var);
        free(user_var);
      } else {
        // Generate the condition (binary operation)
        rewrite_values = 1;
        if (current->left->right->ExprType == I32) {
          rewrite_numbers_to = NUMBER;
        } else if (current->left->right->ExprType == F64) {
          rewrite_numbers_to = FLOAT_NUMBER;
        }
        generate_binary_expression(condition_variable, current->left->right,
                                   if_current, "if");
        rewrite_values = 0;
      }

      // Jump if the condition is false
      codegen_printf("JUMPIFEQ $if_false_%d %s bool@false\n", if_current,
                     condition_variable);

      // Generate until the end of the if statement
      scope_stack_push(s_scope_stack, If);
      traverse_tree(root->left);
      root = scope_end;
      scope_end = NULL;

      codegen_printf("JUMP $if_end_%d\n", if_current);
      codegen_printf("LABEL $if_false_%d\n", if_current);
      if (root->left && root->left->right &&
          root->left->right->token->type == ELSE) {
        scope_stack_push(s_scope_stack, If);
        traverse_tree(root->left->left);
        root = scope_end;
        scope_end = NULL;
      }
      fprintf(stderr, "The if_current is = %d\n", if_current);
      codegen_printf("LABEL $if_end_%d\n", if_current);
      break;
    case RIGHT_BRACE:
      generate_debug_comment("End of scope");
      ScopeType scope_type = scope_stack_pop(s_scope_stack);
      if (scope_type == While || scope_type == If) {
        scope_end = root;
        return;
      } else if (scope_type == Function) {
        if (in_main) {
          codegen_printf("EXIT int@0\n");
          in_main = 0;
        } else {
          codegen_printf("DEFVAR LF@%%retval\n");
          codegen_printf("RETURN\n");
        }
        break;
      }
      break;
    case WHILE:
      generate_debug_comment("While statement");
      while_current = while_counter++;

      char *while_condition = malloc(256);
      sprintf(while_condition, "LF@%%while_cond_%d", while_current);
      generate_defvar(while_condition);

      char *non_null_var = malloc(256);

      if (current->left->right->token->type == IDENTIFIER ||
          current->right->token->type == PIPE) {
        sprintf(non_null_var, "LF@%s", current->right->left->token->lexeme);
        generate_defvar(non_null_var);
      }

      codegen_printf("DEFVAR LF@%%condition_while_%d_helper\n", while_current);

      // Pregenerate all defvars
      // Copy all currents and counters
      int copy_label_counter = s_label_counter;
      int copy_while_current = while_current;
      int copy_while_counter = while_counter;
      int copy_if_counter = if_counter;
      generate_debug_comment("While statement, pregenerate all defvars");
      output_only_defvar = 1;
      scope_stack_push(s_scope_stack, While);
      traverse_tree(root->left);
      output_only_defvar = 0;
      generate_debug_comment("While statement, pregenerate all defvars end");
      s_label_counter = copy_label_counter;
      while_current = copy_while_current;
      while_counter = copy_while_counter;
      if_counter = copy_if_counter;

      output_no_defvar = 1;
      codegen_printf("LABEL $while_start_%d\n", while_current);
      // generate_binary_expression(while_condition, current->left->right);

      // Generate if not null
      if (current->left->right->token->type == IDENTIFIER &&
          current->right->token->type == PIPE) {
        char *user_var = malloc(256);
        sprintf(user_var, "LF@%s", current->left->right->token->lexeme);
        codegen_printf("EQ %s %s nil@nil\n", while_condition,
                       translate_if_possible(user_var));
        codegen_printf("NOT %s %s\n", while_condition, while_condition);
        generate_move(non_null_var, translate_if_possible(user_var));
        free(non_null_var);
        free(user_var);
      } else {
        rewrite_values = 1;
        if (current->left->right->ExprType == I32) {
          rewrite_numbers_to = NUMBER;
        } else if (current->left->right->ExprType == F64) {
          rewrite_numbers_to = FLOAT_NUMBER;
        }
        // Generate the condition (binary operation)
        generate_binary_expression(while_condition, current->left->right,
                                   while_current, "while");
        rewrite_values = 0;
      }

      codegen_printf("JUMPIFEQ $while_end_%d %s bool@false\n", while_current,
                     while_condition);

      scope_stack_push(s_scope_stack, While);
      traverse_tree(root->left);
      root = scope_end;
      scope_end = NULL;
      codegen_printf("JUMP $while_start_%d\n", while_current);
      codegen_printf("LABEL $while_end_%d\n", while_current);
      while_current--;
      output_no_defvar = 0;
      break;
    case RETURN:
      generate_debug_comment("Return statement");
      if (current->right && strcmp(current->right->token->lexeme, ";") != 0) {
        char *return_value = malloc(256);
        generate_temporary_expressions(current->right, &return_value);
        codegen_printf("DEFVAR LF@%%retval\n");
        codegen_printf("MOVE LF@%%retval %s\n", return_value);
        free(return_value);
      }
      codegen_printf("RETURN\n");
      break;
    default:
      break;
    }
  }

  if (root != NULL) {
    traverse_tree(root->left);
  }
}
void generate_binary_expression(char *return_variable,
                                struct treeNode *expression_node, int helper_id,
                                char *helper_prefix) {
  if (expression_node == NULL) {
    return;
  }

  char *left = malloc(256);
  generate_temporary_expressions(expression_node->left, &left);
  left = translate_if_possible(left);

  char *right = malloc(256);
  generate_temporary_expressions(expression_node->right, &right);
  right = translate_if_possible(right);

  char *helper = malloc(256);
  sprintf(helper, "LF@%%condition_%s_%d_helper", helper_prefix, helper_id);
  generate_defvar(helper);

  if (expression_node->token->type == EQUAL_EQUAL) {
    codegen_printf("EQ %s %s %s\n", return_variable, left, right);
  } else if (expression_node->token->type == BANG_EQUAL) {
    codegen_printf("EQ %s %s %s\n", return_variable, left, right);
    codegen_printf("NOT %s %s\n", return_variable, return_variable);
  } else if (expression_node->token->type == GREATER) {
    codegen_printf("GT %s %s %s\n", return_variable, left, right);
  } else if (expression_node->token->type == GREATER_EQUAL) {
    codegen_printf("GT %s %s %s\n", return_variable, left, right);
    codegen_printf("EQ %s %s %s\n", helper, left, right);
    codegen_printf("OR %s %s %s\n", return_variable, return_variable, helper);
  } else if (expression_node->token->type == LESS) {
    codegen_printf("LT %s %s %s\n", return_variable, left, right);
  } else if (expression_node->token->type == LESS_EQUAL) {
    codegen_printf("LT %s %s %s\n", return_variable, left, right);
    codegen_printf("EQ %s %s %s\n", helper, left, right);
    codegen_printf("OR %s %s %s\n", return_variable, return_variable, helper);
  }

  free(helper);
}

void generate_temporary_expressions(treeNode *root, char **result) {
  if (root == NULL) {
    return;
  }

  // It's a function call
  int left_is_identifier = root->left && root->left->token->type == IDENTIFIER;
  int is_special_function =
      root->left && starts_with(root->left->token->lexeme, "ifj.");
  int is_function_call = root->left && root->left->left &&
                         root->left->left->token->type == LEFT_PAREN;

  if (left_is_identifier && (is_special_function || is_function_call)) {
    sprintf(*result, "LF@%%tmp%d", s_label_counter++);
    generate_defvar(*result);

    // When calling a special function, the name is for some reason in multiple
    // nodes, hence the left->left. This won't work though, when calling a
    // normal function...

    if (starts_with(root->left->token->lexeme, "ifj.")) {
      generate_function_call(*result, root->left->left, 1);
    } else {
      generate_function_call(*result, root->left, 0);
    }

    return;
  }

  fprintf(stderr, "Generating temporary expression\n");
  fprintf(stderr, "Root lexeme: %s\n", root->token->lexeme);
  fprintf(stderr, "Root type: %d\n", root->token->type);

  // This is a hack to get back the original type of the token,
  // because it's unreasonably changed in the Semantic analyser

  if (root->token->type == NUMBER) {
    if (root->token->lexeme[0] == '+') {
      root->token->type = PLUS;
    } else if (root->token->lexeme[0] == '-') {
      root->token->type = MINUS;
    } else if (root->token->lexeme[0] == '*') {
      root->token->type = STAR;
    } else if (root->token->lexeme[0] == '/') {
      root->token->type = SLASH;
    }
  }

  if (root->token->type == EQUAL) {
    generate_temporary_expressions(root->right, result);
    return;
  }

  if (root->token->type == IDENTIFIER) {
    sprintf(*result, "LF@%s", root->token->lexeme);
    return;
  }

  if (rewrite_values) {
    if (root->token->type == NUMBER || root->token->type == FLOAT_NUMBER) {
      root->token->type = rewrite_numbers_to;
    }
  }

  if (root->token->type == NUMBER) {
    sprintf(*result, "int@%s", root->token->lexeme);
    return;
  }

  if (root->token->type == FLOAT_NUMBER) {
    float f = strtof(root->token->lexeme, NULL);
    sprintf(*result, "float@%a", f);
    return;
  }

  if (root->token->type == K_NULL) {
    sprintf(*result, "nil@nil");
    return;
  }

  if (root->token->type == PLUS || root->token->type == MINUS ||
      root->token->type == STAR || root->token->type == SLASH) {
    char *left = malloc(256);
    char *right = malloc(256);
    generate_temporary_expressions(root->left, (char **)&left);

    if (root->token->type == SLASH && root->right->ExprType == NUMBER &&
        root->right->token->type == NUMBER) {
      root->right->token->type = FLOAT_NUMBER;
    }

    fprintf(stderr, "Right ExprType: %d, Right type: %d, Right lexeme: %s\n",
            root->right->ExprType, root->right->token->type,
            root->right->token->lexeme);

    generate_temporary_expressions(root->right, (char **)&right);

    left = translate_if_possible(left);
    right = translate_if_possible(right);

    generate_defvar(left);
    generate_defvar(right);
    sprintf(*result, "LF@%%tmp%d", s_label_counter++);
    fprintf(stderr, "Result: %s\n", *result);
    generate_move(*result, left);

    // If one of these operations, we should try to do conversions
    if (root->token->type == PLUS || root->token->type == MINUS ||
        root->token->type == STAR) {
      // The right side should be converted to the same type as is the left side
      fprintf(stderr, "Implicit cast: Left ExprType %d, Right ExprType %d\n",
              root->left->ExprType, root->right->ExprType);
      if ((root->left->ExprType == I32 || root->left->ExprType == NUMBER) &&
          (root->right->ExprType == F64 ||
           root->right->ExprType == FLOAT_NUMBER)) {
        fprintf(stderr, "Implicit cast: I32 -> F64\n");
        codegen_printf("FLOAT2INT %s %s\n", right, right);
      } else if ((root->left->ExprType == F64 ||
                  root->left->ExprType == FLOAT_NUMBER) &&
                 (root->right->ExprType == I32 ||
                  root->right->ExprType == NUMBER)) {
        fprintf(stderr, "Implicit cast: F64 -> I32\n");
        codegen_printf("INT2FLOAT %s %s\n", right, right);
      }
    }

    if (root->token->type == PLUS) {
      generate_add(*result, *result, right);
    } else if (root->token->type == MINUS) {
      generate_sub(*result, *result, right);
    } else if (root->token->type == STAR) {
      generate_mul(*result, *result, right);
    } else if (root->token->type == SLASH) {
      if (root->right->ExprType == I32 && root->right->token->type != NUMBER) {
        codegen_printf("INT2FLOAT %s %s\n", right, right);
      }

      if (root->left->ExprType == I32 || root->left->ExprType == NUMBER) {
        codegen_printf("INT2FLOAT %s %s\n", *result, *result);
      }

      root->ExprType = F64;

      generate_div(*result, *result, right);
    }
    return;
  }
}
void generate_assignment(const char *identifier,
                         const char *expression_result) {
  generate_move(identifier, expression_result);
}

void generate_preamble() { codegen_printf(".IFJcode24\n"); }

void generate_jump_to_main() {
  generate_debug_comment("Jump into main");
  generate_createframe();
  generate_pushframe();
  codegen_printf("CALL main\n");
  codegen_printf("EXIT int@0\n");
}

void generate_createframe() {
  codegen_printf("CREATEFRAME\n");
  memory_map_create_frame(s_memory_map);
}

void generate_pushframe() {
  codegen_printf("PUSHFRAME\n");
  memory_map_push_frame(s_memory_map);
}

void generate_popframe() {
  codegen_printf("POPFRAME\n");
  memory_map_pop_frame(s_memory_map);
}

void generate_move(const char *var, const char *value) {
  generate_defvar(var);
  generate_defvar(value);
  codegen_printf("MOVE %s %s\n", var, value);
}

void generate_defvar(const char *fqn) {
  if (memory_map_def_if_not_exists(s_memory_map, fqn)) {
    codegen_printf("DEFVAR %s\n", fqn);
  } else {
    fprintf(stderr, "Variable %s should not be defined\n", fqn);
  }
}

void generate_add(const char *var, const char *op1, const char *op2) {
  generate_defvar(var);
  codegen_printf("ADD %s %s %s\n", translate_if_possible(var),
                 translate_if_possible(op1), translate_if_possible(op2));
}

void generate_sub(const char *var, const char *op1, const char *op2) {
  generate_defvar(var);
  codegen_printf("SUB %s %s %s\n", translate_if_possible(var),
                 translate_if_possible(op1), translate_if_possible(op2));
}

void generate_mul(const char *var, const char *op1, const char *op2) {
  generate_defvar(var);
  codegen_printf("MUL %s %s %s\n", translate_if_possible(var),
                 translate_if_possible(op1), translate_if_possible(op2));
}

void generate_div(const char *var, const char *op1, const char *op2) {
  generate_defvar(var);
  codegen_printf("DIV %s %s %s\n", translate_if_possible(var),
                 translate_if_possible(op1), translate_if_possible(op2));
}

void generate_function_call(const char *result, treeNode *function_name_node,
                            int is_special) {
  if (function_name_node == NULL) {
    return;
  }
  generate_createframe();
  generate_arguments(function_name_node->left);
  generate_pushframe();
  char *function_name = malloc(256);
  if (is_special) {
    sprintf(function_name, "$%s", function_name_node->token->lexeme);
  } else {
    sprintf(function_name, "%s", function_name_node->token->lexeme);
  }
  codegen_printf("CALL %s\n", function_name);
  generate_popframe();
  if (result != NULL) {
    // This expects that the function return register is predefined,
    // thus we can't use the generate_move function.
    codegen_printf("MOVE %s TF@%%retval\n", result);
  }
}

// We get the '(' here
void generate_arguments(struct treeNode *parameter_start) {
  if (parameter_start == NULL) {
    return;
  }

  struct treeNode *parameter = parameter_start->left;

  int argument_counter = 0;
  while (parameter != NULL) {
    char *result = malloc(256);
    sprintf(result, "TF@%%arg%d", argument_counter++);

    // If the parameter is an identifier
    if (parameter->right->token->type == IDENTIFIER) {
      char *value = malloc(256);
      sprintf(value, "LF@%s", parameter->right->token->lexeme);
      dump_translation_context(s_argument_translation_context);
      generate_move(result, translate_if_possible(value));
      free(result);
      free(value);
    } else if (parameter->right->token->type == NUMBER) {
      char *value = malloc(256);
      sprintf(value, "int@%s", parameter->right->token->lexeme);
      generate_move(result, value);
      free(result);
      free(value);
    } else if (parameter->right->token->type == FLOAT_NUMBER) {
      char *value = malloc(256);
      sprintf(value, "float@%s", parameter->right->token->lexeme);
      generate_move(result, value);
      free(result);
      free(value);
    } else if (parameter->right->token->type == STRING) {
      char *value = malloc(256);
      sprintf(value, "string@%s",
              sanitize_string(parameter->right->token->lexeme));
      generate_move(result, value);
      free(result);
      free(value);
    } else if (parameter->right->token->type == K_NULL) {
      generate_move(result, "nil@nil");
      free(result);
    } else {
      fprintf(stderr, "Unknown parameter type %d when generating arguments.\n",
              parameter->right->token->type);
    }
    parameter = parameter->left;
  }
}

void generate_function(const char *label, treeNode *function_node) {
  memory_map_function_start_reset(s_memory_map);

  if (s_argument_translation_context != NULL) {
    argument_translation_context_destroy(&s_argument_translation_context);
  }

  s_argument_translation_context = argument_translation_context_create();

  fprintf(stderr, "Generating function %s\n", label);

  if (function_node != NULL) {
    int argument_counter = 0;
    treeNode *parameter = function_node;
    char *fqn = malloc(256);
    char *original_name = malloc(256);
    while (parameter != NULL) {
      sprintf(fqn, "LF@%%arg%d", argument_counter++);
      memory_map_def_if_not_exists(s_memory_map, fqn);

      sprintf(original_name, "LF@%s", parameter->right->token->lexeme);
      argument_translation_context_add(s_argument_translation_context, fqn,
                                       original_name);

      fprintf(stderr, "Adding translation: %s -> %s\n", fqn, original_name);
      parameter = parameter->left;
    }

    free(fqn);
  }

  codegen_printf("LABEL %s\n", label);
}

void generate_debug_comment(const char *message) {
  codegen_printf("# %s\n", message);
}

void generate_builtin_functions() {
  // IFJ WRITE - OK
  generate_debug_comment("Built-in function ifj.write");
  printf("LABEL $write\n");
  printf("WRITE LF@%%arg0\n");
  printf("RETURN\n");
  // IFJ READ I32 - OK
  generate_debug_comment("Built-in function ifj.readi32");
  printf("LABEL $readi32\n");
  printf("DEFVAR LF@%%retval\n");
  printf("READ LF@%%retval int\n");
  printf("RETURN\n");
  // IFJ READ F64 - OK
  generate_debug_comment("Built-in function ifj.readf64");
  printf("LABEL $readf64\n");
  printf("DEFVAR LF@%%retval\n");
  printf("READ LF@%%retval float\n");
  printf("RETURN\n");
  // IFJ READ STR - OK
  generate_debug_comment("Built-in function ifj.readstr");
  printf("LABEL $readstr\n");
  printf("DEFVAR LF@%%retval\n");
  printf("READ LF@%%retval string\n");
  printf("RETURN\n");
  // IFJ INT2FLOAT - OK
  generate_debug_comment("Built-in function ifj.i2f");
  printf("LABEL $i2f\n");
  printf("DEFVAR LF@%%retval\n");
  printf("INT2FLOAT LF@%%retval LF@%%arg0\n");
  printf("RETURN\n");
  // IFJ FLOAT2INT - OK
  generate_debug_comment("Built-in function ifj.f2i");
  printf("LABEL $f2i\n");
  printf("DEFVAR LF@%%retval\n");
  printf("FLOAT2INT LF@%%retval LF@%%arg0\n");
  printf("RETURN\n");
  // IFJ STRING - OK
  generate_debug_comment("Built-in function ifj.string");
  printf("LABEL $string\n");
  printf("DEFVAR LF@%%retval\n");
  printf("MOVE LF@%%retval LF@%%arg0\n");
  printf("RETURN\n");
  // IFJ LENGTH - OK
  generate_debug_comment("Built-in function ifj.length");
  printf("LABEL $length\n");
  printf("DEFVAR LF@%%retval\n");
  printf("STRLEN LF@%%retval LF@%%arg0\n");
  printf("RETURN\n");
  // IFJ CONCAT - OK
  generate_debug_comment("Built-in function ifj.concat");
  printf("LABEL $concat\n");
  printf("DEFVAR LF@%%retval\n");
  printf("CONCAT LF@%%retval LF@%%arg0 LF@%%arg1\n");
  printf("RETURN\n");
  // IFJ SUBSTRING - OK
  generate_debug_comment("Built-in function ifj.substring");
  printf("LABEL $substring\n");
  printf("DEFVAR LF@%%retval\n");
  printf("MOVE LF@%%retval string@\n");
  printf("DEFVAR LF@%%char\n");
  printf("DEFVAR LF@%%cond\n");
  // i < 0
  printf("LT LF@%%cond LF@%%arg1 int@0\n");
  printf("JUMPIFEQ $ret_null LF@%%cond bool@true\n");
  // j < 0
  printf("LT LF@%%cond LF@%%arg2 int@0\n");
  printf("JUMPIFEQ $ret_null LF@%%cond bool@true\n");
  // i > j
  printf("GT LF@%%cond LF@%%arg1 LF@%%arg2\n");
  printf("JUMPIFEQ $ret_null LF@%%cond bool@true\n");
  // i >= length(s)
  printf("DEFVAR LF@%%length\n");
  printf("DEFVAR LF@%%helper\n");
  printf("STRLEN LF@%%length LF@%%arg0\n");
  printf("GT LF@%%cond LF@%%arg1 LF@%%length\n");
  printf("EQ LF@%%helper LF@%%arg1 LF@%%length\n");
  printf("OR LF@%%cond LF@%%cond LF@%%helper\n");
  printf("JUMPIFEQ $ret_null LF@%%cond bool@true\n");
  // j > length(s)
  printf("GT LF@%%cond LF@%%arg2 LF@%%length\n");
  printf("JUMPIFEQ $ret_null LF@%%cond bool@true\n");
  // vse ok, loop
  printf("LABEL $substring_loop\n");
  printf("GETCHAR LF@%%char LF@%%arg0 LF@%%arg1\n");
  printf("CONCAT LF@%%retval LF@%%retval LF@%%char\n");
  printf("ADD LF@%%arg1 LF@%%arg1 int@1\n");
  printf("JUMPIFEQ $substring_end LF@%%arg1 LF@%%arg2\n");
  printf("JUMP $substring_loop\n");
  printf("LABEL $ret_null\n");
  printf("MOVE LF@%%retval nil@nil\n");

  //printf("LABEL $ret_null_exit\n");
  //printf("MOVE LF@%%retval nil@nil\n");
  //printf("EXIT int@58\n");

  printf("LABEL $substring_end\n");
  printf("RETURN\n");
  // IFJ STRCMP
  generate_debug_comment("Built-in function ifj.strcmp");
  printf("LABEL $strcmp\n");
  printf("DEFVAR LF@%%retval\n");     
  printf("MOVE LF@%%retval int@0\n"); 
  printf("DEFVAR LF@%%cond\n");       

  printf("DEFVAR LF@%%length1\n");
  printf("DEFVAR LF@%%length2\n");
  printf("DEFVAR LF@%%index\n");
  printf("MOVE LF@%%index int@0\n");

  printf("STRLEN LF@%%length1 LF@%%arg0\n");
  printf("STRLEN LF@%%length2 LF@%%arg1\n");

  printf("DEFVAR LF@%%char1\n");
  printf("DEFVAR LF@%%char2\n");
  printf("MOVE LF@%%char1 string@\n");
  printf("MOVE LF@%%char2 string@\n");

  printf("LABEL $strcmp_loop\n"); // Hlavni smycka porovnani
  printf("JUMPIFEQ $strcmp_length LF@%%index LF@%%length1\n"); // Konec smycky, pokud jsme dosli na konec str1
  printf("JUMPIFEQ $strcmp_length LF@%%index LF@%%length2\n"); // Konec smycky, pokud jsme dosli na konec str2

  printf("GETCHAR LF@%%char1 LF@%%arg0 LF@%%index\n"); // Nacti znak z str1
  printf("GETCHAR LF@%%char2 LF@%%arg1 LF@%%index\n"); // Nacti znak z str2

  printf("EQ LF@%%cond LF@%%char1 LF@%%char2\n");
  printf("JUMPIFEQ $strcmp_next LF@%%cond bool@true\n"); // Pokud znaky jsou stejne, pokracuj

  // Znak se lisi -> rozhodni o poradi
  printf("GT LF@%%cond LF@%%char1 LF@%%char2\n");
  printf("JUMPIFEQ $strcmp_gt LF@%%cond bool@true\n");
  printf("MOVE LF@%%retval int@-1\n"); // str1 < str2
  printf("JUMP $strcmp_end\n");

  printf("LABEL $strcmp_gt\n");
  printf("MOVE LF@%%retval int@1\n"); // str1 > str2
  printf("JUMP $strcmp_end\n");

  printf("LABEL $strcmp_next\n"); // Pokracuj v porovnani
  printf("ADD LF@%%index LF@%%index int@1\n");
  printf("JUMP $strcmp_loop\n");

  printf("LABEL $strcmp_length\n"); // Pokud jsme dosli na konec jednoho ze stringu
  // Porovnej delky
  printf("EQ LF@%%cond LF@%%length1 LF@%%length2\n");
  printf("JUMPIFEQ $strcmp_end LF@%%cond bool@true\n"); // Pokud jsou delky stejne, navrat 0

  printf("GT LF@%%cond LF@%%length1 LF@%%length2\n");
  printf("JUMPIFEQ $strcmp_gt LF@%%cond bool@true\n"); // Pokud str1 > str2 podle delky

  printf("MOVE LF@%%retval int@-1\n"); // str1 < str2 podle delky
  printf("JUMP $strcmp_end\n");

  printf("LABEL $strcmp_end\n"); // Konec funkce
  printf("RETURN\n");
  // IFJ ORD - OK
  generate_debug_comment("Built-in function ifj.ord");
  printf("LABEL $ord\n");
  printf("DEFVAR LF@%%retval\n");
  printf("DEFVAR LF@%%cond_length\n");
  printf("MOVE LF@%%retval int@0\n");              // default value
  printf("LT LF@%%cond_length LF@%%arg1 int@0\n"); // i < 0
  printf("JUMPIFEQ $ord_ret LF@%%cond_length bool@true\n");
  printf("DEFVAR LF@%%length\n");
  printf("STRLEN LF@%%length LF@%%arg0\n");
  printf("LT LF@%%cond_length LF@%%arg1 LF@%%length\n"); // i < length(s)
  printf("JUMPIFEQ $ord_ret LF@%%cond_length bool@false\n");
  printf("STRI2INT LF@%%retval LF@%%arg0 LF@%%arg1\n");
  printf("LABEL $ord_ret\n");
  printf("RETURN\n");
  // IFJ CHR - OK
  generate_debug_comment("Built-in function ifj.chr");
  printf("LABEL $chr\n");
  printf("DEFVAR LF@%%retval\n");
  printf("MOVE LF@%%retval string@\n");
  printf("DEFVAR LF@%%cond\n");
  printf("LT LF@%%cond LF@%%arg0 int@0\n");
  printf("JUMPIFEQ $chr_ret LF@%%cond bool@true\n");
  printf("GT LF@%%cond LF@%%arg0 int@255\n");
  printf("JUMPIFEQ $chr_ret LF@%%cond bool@true\n");
  printf("INT2CHAR LF@%%retval LF@%%arg0\n");
  printf("LABEL $chr_ret\n");
  printf("RETURN\n");
}

int starts_with(const char *str, const char *prefix) {
  while (*prefix && *str) {
    if (*prefix++ != *str++) {
      return 0;
    }
  }

  if (*prefix != '\0') {
    return 0;
  }

  return 1;
}

char *sanitize_string(const char *str) {
  if (str == NULL) {
    return NULL;
  }

  // Calculate the new string length
  size_t original_length = strlen(str);
  size_t new_length = original_length;

  for (size_t i = 0; i < original_length; i++) {
    unsigned char c = str[i];

    // Check for ASCII range 0-32, # (35), \ (92), or literal "\n"
    if ((c <= 32) || (c == 35) || (c == 92)) {
      new_length += 3; // \xxx takes 4 chars, replacing 1 original char
    } else if (c == '\\' && str[i + 1] == 'n') {
      new_length += 2; // Extra space for converting "\n" to "\010"
      i++;             // Skip the 'n' part of "\n"
    }
  }

  // Allocate memory for the new string
  char *sanitized = (char *)malloc(new_length + 1);
  if (sanitized == NULL) {
    return NULL; // Allocation failed
  }

  // Build the new string
  size_t j = 0;
  for (size_t i = 0; i < original_length; i++) {
    unsigned char c = str[i];

    // Handle literal "\n"
    if (c == '\\' && str[i + 1] == 'n') {
      sanitized[j++] = '\\';
      sanitized[j++] = '0';
      sanitized[j++] = '1';
      sanitized[j++] = '0';
      i++; // Skip the 'n'
    }
    // Handle literal "\r"
    else if (c == '\\' && str[i + 1] == 'r') {
      sanitized[j++] = '\\';
      sanitized[j++] = '0';
      sanitized[j++] = '1';
      sanitized[j++] = '3';
      i++; // Skip the 'r'
    }
    // Handle literal "\t"
    else if (c == '\\' && str[i + 1] == 't') {
      sanitized[j++] = '\\';
      sanitized[j++] = '0';
      sanitized[j++] = '0';
      sanitized[j++] = '9';
      i++; // Skip the 't'
    } else if (c == '\\' && str[i + 1] == 'x') {
      sanitized[j++] = '\\';
      char hex[3] = {str[i + 2], str[i + 3], '\0'};
      int value = strtol(hex, NULL, 16);
      char output[4];
      sprintf(output, "%03d", value);
      for (int k = 0; k < 3; k++) {
        sanitized[j++] = output[k];
      }
      i = i + 3; // Convert hex
    }
    // Handle ASCII range 0-32, # (35), and \ (92)
    else if ((c <= 32) || (c == 35) || (c == 92)) {
      sanitized[j++] = '\\';
      sanitized[j++] = '0' + (c / 100);       // Hundreds place
      sanitized[j++] = '0' + ((c / 10) % 10); // Tens place
      sanitized[j++] = '0' + (c % 10);        // Ones place
    } else {
      sanitized[j++] = str[i];
    }
  }

  sanitized[j] = '\0'; // Null-terminate the string
  return sanitized;
}

ArgumentTranslationContext *argument_translation_context_create() {
  ArgumentTranslationContext *context =
      (ArgumentTranslationContext *)malloc(sizeof(ArgumentTranslationContext));
  if (context == NULL) {
    return NULL;
  }

  context->translations = malloc(256 * sizeof(ArgumentTranslation));
  context->count = 0;
  context->capacity = 256;

  return context;
}
void argument_translation_context_destroy(
    ArgumentTranslationContext **context) {
  if (context == NULL || *context == NULL) {
    return;
  }

  for (int i = 0; i < (*context)->count; i++) {
    free((*context)->translations[i].internal_name);
    free((*context)->translations[i].original_name);
  }

  free((*context)->translations);
  free(*context);
  *context = NULL;
}
void argument_translation_context_add(ArgumentTranslationContext *context,
                                      const char *internal_name,
                                      const char *original_name) {
  if (context == NULL) {
    return;
  }

  if (context->count >= context->capacity) {
    error(99, 0, "Argument translation context is full");
  }

  ArgumentTranslation *translation = &context->translations[context->count];
  translation->internal_name = strdup(internal_name);
  translation->original_name = strdup(original_name);
  context->count++;
}
const char *
argument_translation_context_get(ArgumentTranslationContext *context,
                                 const char *original_name) {
  if (context == NULL) {
    return NULL;
  }

  for (int i = 0; i < context->count; i++) {
    if (!strcmp(context->translations[i].original_name, original_name)) {
      return context->translations[i].internal_name;
    }
  }

  return NULL;
}

char *translate_if_possible(const char *original_name) {
  if (s_argument_translation_context == NULL) {
    return NULL;
  }

  char *translated = argument_translation_context_get(
      s_argument_translation_context, original_name);

  return translated ? translated : original_name;
}

void dump_translation_context(ArgumentTranslationContext *context) {
  if (context == NULL) {
    return;
  }

  fprintf(stderr, "Translation context:\n");

  for (int i = 0; i < context->count; i++) {
    if (context->translations[i].internal_name == NULL ||
        context->translations[i].original_name == NULL) {
      continue;
    }
    fprintf(stderr, "Translation %d: %s -> %s\n", i,
            context->translations[i].original_name,
            context->translations[i].internal_name);
  }

  fprintf(stderr, "End of translation context\n");
}

ScopeStack *scope_stack_create() {
  ScopeStack *stack = malloc(sizeof(ScopeStack));
  if (stack == NULL) {
    error(99, 0, "Failed to create scope stack");
  }

  stack->top = NULL;

  return stack;
}

void destroy_scope_stack(ScopeStack **stack) {
  if (stack == NULL || *stack == NULL) {
    return;
  }

  while ((*stack)->top != NULL) {
    scope_stack_pop(*stack);
  }

  free(*stack);
  *stack = NULL;
}

void scope_stack_push(ScopeStack *stack, ScopeType type) {
  if (stack == NULL) {
    return;
  }

  ScopeStackNode *node = (ScopeStackNode *)malloc(sizeof(ScopeStackNode));
  if (node == NULL) {
    error(99, 0, "Failed to allocate memory for scope stack node");
  }

  node->type = type;
  node->previous = stack->top;
  stack->top = node;
}

ScopeType scope_stack_pop(ScopeStack *stack) {
  if (stack == NULL || stack->top == NULL) {
    return None;
  }

  ScopeStackNode *node = stack->top;
  stack->top = node->previous;

  ScopeType type = node->type;
  free(node);
  return type;
}

void codegen_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  va_list copy;
  va_copy(copy, args);
  unsigned int length = vsnprintf(NULL, 0, format, args);
  char *string = malloc(length + 1);
  if (string == NULL) {
    error(99, 0, "Failed to allocate memory for codegen printf");
    return;
  }
  vsprintf(string, format, copy);
  if (output_only_defvar) {
    if (starts_with(string, "DEFVAR")) {
      if (!starts_with(string, "DEFVAR TF")) {
        printf("%s", string);
      }
    }
  } else if (output_no_defvar) {
    if (!starts_with(string, "DEFVAR")) {
      printf("%s", string);
    } else if (starts_with(string, "DEFVAR TF")) {
      printf("%s", string);
    }
  } else {
    printf("%s", string);
  }
  free(string);
  va_end(copy);
  va_end(args);
}
