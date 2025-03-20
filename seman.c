#include "seman.h"
#include "codegen.h"

symbtable_stack *stack;                // stack used for scope
int function_return = 0;               // 0 return was not used 1 was used
int function_return_type = 0;          // function return type
int function_return_type_nullable = 0; // 1 function can return null
declared_variables *variables = NULL;  // Array of declared variables
int i_declared_variables = 0;          // Index for array of declared variables
int call_function_is_nullable = 0; // Function that is being called can be null

// Adding variable to array of declared variables
void addVariable(struct treeNode *node) {
  variables =
      realloc(variables, (i_declared_variables + 1) *
                             sizeof(declared_variables)); // Resize the array
  if (variables == NULL) {
    error(99, 0, "Memory fail");
  }
  variables[i_declared_variables].id = malloc(strlen(node->token->lexeme) + 1);
  strcpy(variables[i_declared_variables].id, node->token->lexeme);
  variables[i_declared_variables].is_used = 0;
  variables[i_declared_variables].is_null = 0;
  variables[i_declared_variables].scope = stack->top;
  variables[i_declared_variables].line = node->token->line;
  i_declared_variables++;
}

// Check if variable was used in the scope
void check_variables() {
  for (int i = 0; i < i_declared_variables; i++) {
    if (variables[i].scope == stack->top)
      if (variables[i].is_used == 0) {
        //printf("%s ", variables[i].id);
        error(9, variables[i].line, "Variable not used/modified");
      }
  }
}

// Print all declared variables
void print_variables() {
  for (int i = 0; i < i_declared_variables; i++) {
    //printf("\n %s %d\n", variables[i].id, variables[i].is_used);
  }
}

// Delete all declared variables
void free_variables() {
  for (int i = 0; i < i_declared_variables; i++) {
    free(variables[i].id);
  }
  free(variables);
}

// Variable was used
void variable_is_used(char *id) {
  for (int i = 0; i < i_declared_variables; i++) {
    if (!strcmp(id, variables[i].id)) {
      variables[i].is_used = 1;
    }
  }
}

// Variable was assigned to null
void variable_is_null(char *id) {
  for (int i = 0; i < i_declared_variables; i++) {
    if (!strcmp(id, variables[i].id)) {
      variables[i].is_null = 1;
    }
  }
}

// Variable was assigned to different value than null
void variable_is_not_null(char *id) {
  for (int i = 0; i < i_declared_variables; i++) {
    if (!strcmp(id, variables[i].id)) {
      variables[i].is_null = 0;
    }
  }
}

// Return 1 if variable was assigned to null
int is_variable_null(char *id) {
  for (int i = 0; i < i_declared_variables; i++) {
    if (!strcmp(id, variables[i].id)) {
      if (variables[i].is_null == 1) {
        return 1;
      }
      return 0;
    }
  }
  return 0;
}

// Check if main was defined and is without parameteres
void check_main() {
  Symbol *symbol_check = (Symbol *)malloc(sizeof(Symbol));
  symbol_check = get_symbol(stack, "main");
  if (symbol_check == NULL) {
    error(3, 0, "Main not defined");
  }
  if (symbol_check->number_of_params != 0) {
    error(4, 0, "Main has params");
  }
  // Check if main is void
  if (symbol_check->returnType != VOID) {
    error(4, 0, "Main has return type");
  }
}

// Adding predefined ifj functions
void add_ifj_functions() {
  add_ifj_function("ifj.readstr", U8, 1, (int[]){}, 0);
  add_ifj_function("ifj.readi32", I32, 1, (int[]){}, 0);
  add_ifj_function("ifj.readf64", F64, 1, (int[]){}, 0);
  add_ifj_function("ifj.write", VOID, 0, (int[]){ANY_PARAM_TYPE}, 1);
  add_ifj_function("ifj.i2f", F64, 0, (int[]){I32}, 1);
  add_ifj_function("ifj.f2i", I32, 0, (int[]){F64}, 1);
  add_ifj_function("ifj.string", U8, 0, (int[]){U8}, 1);
  add_ifj_function("ifj.length", I32, 0, (int[]){U8}, 1);
  add_ifj_function("ifj.concat", U8, 0, (int[]){U8, U8}, 2);
  add_ifj_function("ifj.substring", U8, 1, (int[]){U8, I32, I32}, 3);
  add_ifj_function("ifj.strcmp", I32, 0, (int[]){U8, U8}, 2);
  add_ifj_function("ifj.ord", I32, 0, (int[]){U8, I32}, 2);
  add_ifj_function("ifj.chr", U8, 0, (int[]){I32}, 1);
}

// Adding single ifj function
void add_ifj_function(char *id, int returntype, int nullable, int *params,
                      int number_of_params) {
  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  symbol->id = id;
  symbol->type = -1;
  symbol->returnType = returntype;
  symbol->nullable = nullable;
  symbol->number_of_params = number_of_params;
  int size = 2;
  symbol->paramtypes = (enum TokenType *)malloc(size * sizeof(enum TokenType));
  for (int i = 0; i < number_of_params; i++) {
    size *= 2;
    symbol->paramtypes = (enum TokenType *)realloc(
        symbol->paramtypes, size * sizeof(enum TokenType));
    symbol->paramtypes[i] = params[i];
  }
  add_symbol(stack, symbol);
}

// Start of semantic control
void check_seman(struct treeNode *root) {
  if (root == NULL) {
    return;
  }
  struct treeNode *pom = NULL;
  struct treeNode *node = NULL;
  if (root->right != NULL) {
    pom = root->right;
    switch (pom->token->type) {
    case CONST:
      // Prolog
      if (!strcmp(pom->right->token->lexeme, "ifj")) {
        stack = stack_create();
        push_symbtable_stack(stack);
        node = root;

        add_ifj_functions();
        add_functions(node);

        check_main();
        break;
      }
      // Defining constant
      node = pom->right;
      seman_const_var(node, pom);
      break;
    case PUB:
      // Defining function
      node = pom;
      seman_pub(node);
      break;
      // Defining variable
    case VAR:
      node = pom->right;
      seman_const_var(node, pom);
      break;
      // Pseudovariable
    case UNDERSCORE:
      node = pom;
      seman_underscore(node);
      break;
      // Calling function or assigning
    case IDENTIFIER:
      node = pom;
      seman_identifier(node);
      break;
      // If condition
    case IF:
      node = pom;
      seman_while_if(node);
      break;
      // While loop
    case WHILE:
      node = pom;
      seman_while_if(node);
      break;
      // Return from declared funciton
    case RETURN:
      node = pom;
      seman_return(node);
      break;
      // Else branch
    case ELSE:
      push_symbtable_stack(stack);
      //printf("%s: NEWscope\n", pom->token->lexeme);
      break;
      // End of scope
    case RIGHT_BRACE:
      check_variables();
      pop_symbtable_stack(stack);
      if (stack->top == 0) {
        // check and reset everything
        if (function_return_type != VOID) {
          if (function_return == 0) {
            error(6, pom->token->line, "No return in function");
          }
        }
        i_declared_variables = 0;
        function_return = 0;
        function_return_type_nullable = 0;
        function_return_type = 0;
      }
      //printf("ENDscope\n");
      break;
    default:
      break;
    }
  }
  check_seman(root->left);
}

// Adding all declared functions before checking seman
void add_functions(struct treeNode *root) {
  if (root == NULL) {
    return;
  }
  struct treeNode *node = NULL;
  if (root->right != NULL) {
    node = root->right;
    // Adding declared function
    if (node->token->type == PUB) {
      seman_pub_add_functions(node);
    }
  }
  add_functions(root->left);
}

// Checking return
void seman_return(struct treeNode *node) {
  //printf("%s:", node->token->lexeme);
  if (node->right != NULL) {
    // Empty return
    if (node->right->token->type == SEMICOLON) {
      if (function_return_type != VOID)
        error(4, node->token->line, "Returns wrong type");
      return;
    }
  }
  // returns function
  if (node->left != NULL) {
    node = node->left;
    if (strcmp(node->token->lexeme, "ifj")){
      //printf("funkce id:%s ", node->token->lexeme);
    }
    int type = seman_function_call(node);
    if (function_return_type != type || function_return_type == VOID) {
      error(4, node->token->line, "Returns wrong type");
    }
    function_return = 1;
  }
  // returns single element
  if (node->right->left == NULL) {
    int type = -1;
    if (node->right->token->type == NUMBER) {
      if (function_return_type == I32 || function_return_type == F64)
        type = function_return_type;
    }
    if (node->right->token->type == FLOAT_NUMBER) {
      type = F64;
    }
    if (node->right->token->type == STRING) {
      type = U8;
    }
    if (node->right->token->type == IDENTIFIER) {
      type = call_param_type(node->right);
    }
    if (type != function_return_type) {
      error(4, node->token->line, "Returns wrong type");
    }
    //printf("%s", node->right->token->lexeme);
  }
  // Calling bottom up for expression
  else {
    checkExpression(node->right);
    if(node->right->isRel == 1){
        error(7, node->token->line, "Wrong type");
      }
    int type = call_param_type(node->right);
    if (function_return_type != F64 && function_return_type != I32) {
      error(4, node->token->line, "Returns wrong type");
    }
    if (function_return_type == I32 && type == F64) {
      error(4, node->token->line, "Returns wrong type");
    }
    //printf("EXPRESSION!!");
  }
  // Return was used
  function_return = 1;
  //printf("\n");
}

// Checking If or While
void seman_while_if(struct treeNode *node) {
  //printf("%s: ", node->token->lexeme);
  //printf("podminka: ");
  int type = -1;
  // Not function in If condition
  if (node->left->left == NULL) {
    // Just identifier in If condition
    if (node->left->right->token->type == IDENTIFIER) {
      if (node->left->right->left == NULL)
        type = call_param_type(node->left->right);
    }
    // Expression in If condition
    else {
      checkExpression(node->left->right);
      type = call_param_type(node->left->right);
      if(node->left->right->isRel != 1){
        error(7, node->token->line, "Nepravdivostni podminka");
      }
      //printf("EXPRESSION!!");
    }
  }
  // Function in If condition
  else if (!strcmp(node->left->left->token->lexeme, "ifj") ||
           node->left->left != NULL) {
    type = seman_function_call(node->left->left);
    if (type == VOID) {
      error(4, node->token->line, "Void funkce v podmince");
    }
  }
  node = node->right;
  // Id without null: |id_without_null|
  if (node->token->type == PIPE) {
    // Defining id without null as constant
    //printf(" id bez null: %s", node->left->token->lexeme);
    Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
    symbol->id = node->left->token->lexeme;
    symbol->constant = 1;
    symbol->nullable = 0;
    symbol->type = type;
    Symbol *symbol_check = (Symbol *)malloc(sizeof(Symbol));
    symbol_check = get_symbol(stack, node->token->lexeme);
    if (symbol_check != NULL) {
      error(5, node->token->line, "Already defined");
    }
    add_symbol(stack, symbol);
  }
  // New scope
  push_symbtable_stack(stack);
  //printf("NEWscope \n");
}

// Defining constant or variable
void seman_const_var(struct treeNode *node, struct treeNode *pom) {
  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  Symbol *symbol_check = (Symbol *)malloc(sizeof(Symbol));
  symbol_check = get_symbol(stack, node->token->lexeme);
  // Check if variable/constant was already defined
  if (symbol_check != NULL) {
    error(5, node->token->line, "Already defined");
  }
  symbol->id = node->token->lexeme;
  // Check if its variable or constant
  if (pom->token->type == VAR) {
    symbol->constant = 0;
  } else {
    symbol->constant = 1;
  }
  //printf("%s: id: %s ", pom->token->lexeme, node->token->lexeme);
  // Type was defined
  if (node->left != NULL) {
    symbol->type = node->left->token->type;
    //printf("typ:%s", node->left->token->lexeme);
    // Check if variable can be null
    if (node->right != NULL) {
      if (!strcmp(node->right->token->lexeme, "?")) {
        symbol->nullable = 1;
      } else {
        symbol->nullable = 0;
      }
    } else {
      symbol->nullable = 0;
    }
  } else {
    symbol->type = 0;
  }
  addVariable(node);
  //printf(" = ");
  // Variable was assigned to function
  if (pom->left->left != NULL) {
    node = pom->left->left;
    call_function_is_nullable = 0;
    int type = seman_function_call(node);
    if (call_function_is_nullable == 1) {
      symbol->nullable = 1;
    }
    call_function_is_nullable = 0;
    if (type == VOID) {
      error(7, node->token->line, "Nelze priradit void");
    }
    if (symbol->type != 0) {
      if (symbol->type != type) {
        error(7, node->token->line, "Spatny typ promenne");
      }
    } else {
      symbol->type = type;
    }
  }
  // Variable was assigned to Expression
  else {
    // Was assigned to single element
    if (pom->left->right->left == NULL) {
      if (pom->left->right->token->type == K_NULL) {
        if (symbol->nullable == 0) {
          error(7, node->token->line, "Not nullable");
        }
        variable_is_null(symbol->id);
      }
      if (pom->left->right->token->type == NUMBER) {
        if (symbol->type == 0)
          symbol->type = I32;
      }
      if (pom->left->right->token->type == FLOAT_NUMBER) {
        symbol->type = F64;
      }
      if (pom->left->right->token->type == STRING) {
        symbol->type = U8;
      }
      if (pom->left->right->token->type == IDENTIFIER) {

        symbol->type = call_param_type(pom->left->right);
      }
      //printf("%s", pom->left->right->token->lexeme);
    }
    // Calling bottom up for epxression
    else {
      node = pom->left->right;
      checkExpression(node);
      if(node->isRel == 1){
        error(7, node->token->line, "Wrong type");
      }
      int type = call_param_type(node);
      //printf("tt %d",type);
      if (symbol->type != 0) {
        if (symbol->type == I32 && symbol->type != type) {
          error(7, node->token->line, "Wrong type");
        }
      } else {
        symbol->type = type;
      }
    }
  }
  add_symbol(stack, symbol);
  //printf("\n");
}

// Pseudovariable
void seman_underscore(struct treeNode *node) {
  node = node->left;
  // Assing to function
  if (node->left != NULL) {
    //printf("FUNKCE");
    if (node->left->token->type == IDENTIFIER) {
      node = node->left;
      if (!strcmp(node->token->lexeme, "ifj") ||
          node->left->token->type == LEFT_PAREN) {
        int type = seman_function_call(node);
        if (type == VOID) {
          error(7, node->token->line, "Nelze priradit void");
        }
      }
    }
    // Assing to expression
  } else {
    checkExpression(node->right);
    if(node->right->isRel == 1){
      error(7, node->token->line, "Wrong type");
    }
    //printf("EXPRESSION");
  }
}

// Checking identifier
void seman_identifier(struct treeNode *node) {
  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  symbol = get_symbol(stack, node->token->lexeme);
  // Not defined ifj function
  if (symbol == NULL && strcmp(node->token->lexeme, "ifj")) {
    error(3, node->token->line, "Not defined");
  }
  // Calling ifj function
  if (!strcmp(node->token->lexeme, "ifj")) {
    //printf("%s: ", node->token->lexeme);
    int type = seman_function_call(node);
    if (type != VOID) {
      error(4, node->token->line, "Calling non-void function");
    }
  } else {
    //printf("%s:", node->token->lexeme);
    // Checking if its function call or variable assign
    int type = is_it_function(node);
    if(type == VOID){
      return;
    }
    if (type != K_NULL) {
      variable_is_not_null(symbol->id);
    }
    if (symbol->type == F64 || symbol->type == I32) {
      if (type == NUMBER) {
        return;
      }
    }
    //printf(" %d ", symbol->type);
    if (type == K_NULL) {
      if (symbol->nullable != 1) {
        error(7, node->token->line, "Variable is not nullable");
      }
      variable_is_null(symbol->id);
      return;
    }
    if (symbol->type != type){
      error(7, node->token->line, "Wrong type");
    }
  }
  //printf("\n");
}

// Checking declared function
void seman_pub(struct treeNode *node) {
  // New scope
  push_symbtable_stack(stack);
  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  function_return_type = node->right->left->token->type;
  //printf("%s:", node->token->lexeme);
  //printf(" id:%s return_type:%d", node->right->token->lexeme,node->right->left->token->type);
  // Check if function can return Null
  if (node->right->right != NULL) {
    symbol->nullable = 1;
    function_return_type_nullable = 1;
    //printf("%s", node->right->right->token->lexeme);
  }
  // Checking parameterers
  while (node->left != NULL) {
    symbol = (Symbol *)malloc(sizeof(Symbol));
    node = node->left;
    symbol->id = node->right->token->lexeme;
    symbol->constant = 1;
    symbol->type = node->right->left->token->type;
    Symbol *symbol_check = (Symbol *)malloc(sizeof(Symbol));
    symbol_check = get_symbol(stack, symbol->id);
    if (symbol_check != NULL) {
      error(5, node->token->line, "Already defined");
    }
    // Defining parameteres as constant
    add_symbol(stack, symbol);
    addVariable(node->right);

    //printf(" %s %s typ:%d ", node->token->lexeme, symbol->id,node->right->left->token->type);
  }
  //printf("NEWscope \n");
}

// Adding all declared functions by user
void seman_pub_add_functions(struct treeNode *node) {
  Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
  symbol->id = node->right->token->lexeme;
  symbol->type = -1;
  symbol->returnType = node->right->left->token->type;
  int size = 2;
  symbol->paramtypes = (enum TokenType *)malloc(size * sizeof(enum TokenType));
  if (node->right->right != NULL) {
    // parametr '?'
  }

  int i = 0;
  // Adding parameteres
  while (node->left != NULL) {
    size *= 2;
    symbol->paramtypes = (enum TokenType *)realloc(
        symbol->paramtypes, size * sizeof(enum TokenType));
    node = node->left;
    symbol->paramtypes[i] = node->right->left->token->type;
    i++;
  }
  symbol->number_of_params = i;

  add_symbol(stack, symbol);
}

// Function being called
int seman_function_call(struct treeNode *node) {
  char *func_id = node->token->lexeme;
  // Starts with token ifj, needs to be created full name: ifj.example
  if (!strcmp(node->token->lexeme, "ifj")) {
    strcat(func_id, node->right->token->lexeme);
    strcat(func_id, node->left->token->lexeme);
    node = node->left;
  }
  Symbol *symbol = get_symbol(stack, func_id);
  // Check if function was defined
  if (symbol == NULL) {
    error(3, node->token->line, "Not defined");
  }
  // Its variable not function
  if (symbol->type != -1) {
    error(3, node->token->line, "Not function");
  }
  // Function can return null
  if (symbol->nullable == 1) {
    call_function_is_nullable = 1;
  }
  int size = 2;
  //printf("call funkce id:%s", func_id);
  node = node->left;
  enum TokenType *paramtypes =
      (enum TokenType *)malloc(size * sizeof(enum TokenType));
  int i = 0;
  // Checking parameteres
  while (node->left != NULL) {
    size *= 2;
    node = node->left;
    paramtypes =
        (enum TokenType *)realloc(paramtypes, size * sizeof(enum TokenType));
    paramtypes[i] = call_param_type(node->right);
    //printf(" %s %s typ:%d", node->token->lexeme, node->right->token->lexeme,paramtypes[i]);
    i++;
  }
  // Wrong number of parameteres
  if (symbol->number_of_params != i) {
    error(4, node->token->line, "Spatny pocet parametru");
  }
  // Checking type of parameteres
  for (int j = 0; j <= i; j++) {
    if (paramtypes[j] != symbol->paramtypes[j] &&
        symbol->paramtypes[j] != ANY_PARAM_TYPE) {
      if (paramtypes[j] == NUMBER) {
        if (symbol->paramtypes[j] != I32 && symbol->paramtypes[j] != F64) {
          error(4, node->token->line, "Spatny typ parametru");
        }
      } else {
        if (paramtypes[j] != symbol->paramtypes[j]) {
          error(4, node->token->line, "Spatny typ parametru");
        }
      }
    }
  }
  return symbol->returnType;
}

// Returns type of parameter
int call_param_type(struct treeNode *node) {
  if (node->ExprType == 0)
  {
    node->ExprType = node->token->type;
  }
  switch (node->ExprType) {
  case NUMBER:
    return I32;
    break;
  case FLOAT_NUMBER:
    return F64;
    break;
  case K_NULL:
    return K_NULL;
    break;
  case STRING:
    return U8;
    break;
  case IDENTIFIER:
    Symbol *symbol = get_symbol(stack, node->token->lexeme);
    if (symbol == NULL) {
      error(3, node->token->line, "Undefined");
    }
    // Is function
    if (symbol->type == -1) {
      seman_function_call(node);
      return symbol->returnType;
    }
    // Is variable
    else {
      if (symbol->constant == 1) {
        variable_is_used(symbol->id);
      }
      return symbol->type;
    }
    break;
  default:
    return node->ExprType;
  }
}

// Bottom up parser for expressions
void checkExpression(struct treeNode *node) {
  if (node->token->type == IDENTIFIER) {
    Symbol *symbol = get_symbol(stack, node->token->lexeme);
    if (symbol == NULL) {
      error(3, node->token->line, "Not defined");
    }
    if (symbol->constant == 1) {
      variable_is_used(symbol->id);
    }
    //FIXME: The token type should stay the same
    fprintf(stderr, "%d being reassigned to %d\n", node->token->type, symbol->type);
    node->ExprType = symbol->type;
    return;
  }
  if (node->left != NULL) {
    if (node->token->type==BANG_EQUAL || node->token->type==EQUAL_EQUAL || node->token->type==GREATER || node->token->type==GREATER_EQUAL || node->token->type==LESS || node->token->type==LESS_EQUAL) {
      node->isRel = 1;
    }   
    checkExpression(node->left);
    if (node->token->type != FLOAT_NUMBER) {
      //FIXME: The token type should stay the same
      fprintf(stderr, "%d being reassigned to %d\n", node->token->type, node->left->token->type);
      // node->token->type = node->left->token->type;
      node->ExprType = node->left->ExprType;
    }
  }
  if (node->right != NULL) {
    if (node->right->token->type==BANG_EQUAL || node->right->token->type==EQUAL_EQUAL || node->right->token->type==GREATER || node->right->token->type==GREATER_EQUAL || node->right->token->type==LESS || node->right->token->type==LESS_EQUAL) {
      node->isRel = 1;
    }      
    checkExpression(node->right);
    if (node->token->type == FLOAT_NUMBER) {
      //FIXME: The token type should stay the same
      fprintf(stderr, "%d being reassigned to %d\n", node->token->type, node->right->token->type);
      node->ExprType = node->right->ExprType;
      // node->token->type = node->right->token->type;
    }
  }
  if (node->left == NULL && node->right == NULL){
    node->ExprType = node->token->type;
  }
  if (node->token->type==BANG_EQUAL || node->token->type==EQUAL_EQUAL || node->token->type==GREATER || node->token->type==GREATER_EQUAL || node->token->type==LESS || node->token->type==LESS_EQUAL) {
      node->isRel = 1;
    }
  if (node->left != NULL && node->right != NULL) {
    if (node->isRel && node->left->isRel || node->right->isRel) {
        error(10, node->token->line, "Relational operators cannot be chained");
      }
  }
  return;
}

// returns function return type or -1 if its not function
int is_it_function(struct treeNode *node) {
  Symbol *symbol = get_symbol(stack, node->token->lexeme);
  // Calling non-void function thats not being assigned
  if (node->left->token->type == LEFT_PAREN) {
    int type = seman_function_call(node);
    if (type != VOID) {
      error(4, node->token->line, "Calling non-void function");
    }
    return VOID;
  } else {
    //printf(" id: %s ", symbol->id);
  }
  // Variable assinging
  if (node->left->token->type == EQUAL) {
    variable_is_used(symbol->id);
    if (symbol->constant == 1) {
      error(5, node->token->line, "Nelze menit const");
    }
    node = node->left;
    if (node->left != NULL) {
      if (node->left->token->type == IDENTIFIER) {
        node = node->left;
        // Assigning to function
        if (!strcmp(node->token->lexeme, "ifj") ||
            node->left->token->type == LEFT_PAREN) {
          int type = seman_function_call(node);
          if (type == VOID) {
            error(7, node->token->line, "Nelze priradit void");
          }
          if (symbol->type != type) {
            error(7, node->token->line, "Wrong type");
          }
          return type;
        }
      }
    }
    // Assinging to expression
    if (node->right != NULL) {
      if (node->right->token->type == IDENTIFIER ||
          node->right->token->type == NUMBER ||
          node->right->token->type == FLOAT_NUMBER ||
          node->right->token->type == STRING ||  node->right->token->type == K_NULL)
        return call_param_type(node->right);
      else {
        node = node->right;
        checkExpression(node);
        if(node->isRel == 1){
          error(7, node->token->line, "Wrong type");
        }
        //printf("EXPRESSION");
        return node->ExprType;
      }
    }
  }

  return -1;
}

symbtable_stack *get_stack() { return stack; }
