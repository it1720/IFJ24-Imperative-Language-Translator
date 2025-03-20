#include "syntax.h"
#include "seman.h"
#include "tree.h"

struct treeNode *root = NULL;
struct treeNode *radek = NULL;
struct treeNode *pom = NULL;

struct treeNode *ast_new_line(struct treeNode *radek) {
  struct Token *new_line_token = (struct Token *)malloc(sizeof(struct Token));
  new_line_token->lexeme = "NEW_LINE";
  radek->left = createNode(new_line_token); // First left node
  return radek->left;
}

struct treeNode *ast_new_parametr(struct treeNode *node) {
  struct Token *new_line_token = (struct Token *)malloc(sizeof(struct Token));
  new_line_token->lexeme = "PARAMETR";
  node->left = createNode(new_line_token); // First left node
  return node->left;
}

void start_prog() {
  struct Token *start_token = (struct Token *)malloc(sizeof(struct Token));
  start_token->lexeme = "START";
  root = createNode(start_token);
  radek = root; // Start from root
  radek = ast_new_line(radek);
  parse_prog();
}
void parse_prog() {
  struct Token *token;
  struct treeNode *node = radek;
  for (int i = 0; i < 9; i++) {
    token = getNextToken();
    switch (i) {
    case 0:
      if (token->type != CONST) {
        error(2, token->line, "Syntax Error");
      }
      node->right = createNode(token);
      break;
    case 1:
      if (strcmp(token->lexeme, "ifj")) {
        error(2, token->line, "Syntax Error");
      }
      node = node->right;
      node->right = createNode(token);
      break;
    case 2:
      if (token->type != EQUAL) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      break;
    case 3:
      if (token->type != AT) {
        error(2, token->line, "Syntax Error");
      }
      node = node->left;
      node->left = createNode(token);
      break;
    case 4:
      if (token->type != IMPORT) {
        error(2, token->line, "Syntax Error");
      }
      node->right = createNode(token);
      break;
    case 5:
      if (token->type != LEFT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
      break;
    case 6:
      if (token->type != STRING) {
        error(2, token->line, "Syntax Error");
      }
      if(strstr(token->lexeme, "ifj24.zig") == NULL){
        error(2, token->line, "Syntax Error");
      }
      node = node->right;
      node->right = createNode(token);
      break;
    case 7:
      if (token->type != RIGHT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
      break;
    case 8:
      if (token->type != SEMICOLON) {
        error(2, token->line, "Syntax Error");
      }
      break;
    }
  }
  parse_func_declare();
}

void parse_func_declare() {
  struct Token *token;
  for (int i = 0; i < 6; i++) {
    token = getNextToken();
    switch (i) {
    case 0:
      if (token->type == EOF_TOKEN) {
        //printf("\nBinary Tree Visualization:\n");
        //printTree(root, 0);
        check_seman(root);
        return;
      }
      if (token->type != PUB) {
        error(2, token->line, "Syntax Error");
      }
      radek = ast_new_line(radek);
      struct treeNode *node = radek;
      node->right = createNode(token);
      break;
    case 1:
      if (token->type != FN) {
        error(2, token->line, "Syntax Error");
      }
      break;
    case 2:
      if (token->type != IDENTIFIER) {
        error(2, token->line, "Syntax Error");
      }
      node = node->right;
      node->right = createNode(token);
      break;
    case 3:
      if (token->type != LEFT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
      pom = node;
      parse_param();
      break;
    case 4:
      node = radek;
      if (is_it_not_return_type(token, node->right->right)) {
        error(2, token->line, "Syntax Error");
      }
      break;
    case 5:
      if (token->type != LEFT_BRACE) {
        error(2, token->line, "Syntax Error");
      }
      parse_func_body();
      break;
    }
  }
}

int is_it_not_type(struct Token *token, struct treeNode *node) {
  if (token->type == QUESTION_MARK) {
    node->right = createNode(token);
    token = getNextToken();
  }
  if (token->type == I32 || token->type == F64 || token->type == U8) {
    node->left = createNode(token);
    return 0;
  }
  return 1;
}

int is_it_not_return_type(struct Token *token, struct treeNode *node) {
  if (token->type == QUESTION_MARK) {
    node->right = createNode(token);
    token = getNextToken();
    if (token->type == I32 || token->type == F64 || token->type == U8) {
      node->left = createNode(token);
      return 0;
    }
  }
  if (token->type == I32 || token->type == F64 || token->type == U8 ||
      token->type == VOID) {
    node->right = NULL;
    node->left = createNode(token);
    return 0;
  }
  return 1;
}

void parse_param() {
  struct Token *token;
  for (int i = 0; i < 3; i++) {
    token = getNextToken();
    switch (i) {
    case 0:
      if (token->type == RIGHT_PAREN)
        return;
      if (token->type != IDENTIFIER) {
        error(2, token->line, "Syntax Error");
      }
      struct treeNode *node = pom;
      node = ast_new_parametr(node);
      node->right = createNode(token);
      break;
    case 1:
      if (token->type != COLON) {
        error(2, token->line, "Syntax Error");
      }
      break;
    case 2:
      if (is_it_not_type(token, node->right)) {
        error(2, token->line, "Syntax Error");
      }
      pom = node;
      // node->right->left = createNode(token);
      parse_next_param();
      break;
    }
  }
}

void parse_next_param() {
  struct Token *token = getNextToken();
  switch (token->type) {
  case RIGHT_PAREN:
    break;
  case COMMA:
    parse_param();
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

void parse_func_body() {
  struct Token *token = getNextToken();
  if (token->type == RIGHT_BRACE) {
    radek = ast_new_line(radek);
    radek->right = createNode(token);
    parse_func_declare();
    return;
  }
  parse_body(token);
  parse_func_body();
}

void parse_body(struct Token *token) {
  radek = ast_new_line(radek);
  struct treeNode *node = radek;
  node->right = createNode(token);
  node = node->right;
  switch (token->type) {
  case CONST:
    token = getNextToken();
    if (token->type != IDENTIFIER) {
      error(2, token->line, "Syntax Error");
    }
    node->right = createNode(token);
    pom = node;
    parse_assign();
    break;
  case VAR:
    token = getNextToken();
    if (token->type != IDENTIFIER) {
      error(2, token->line, "Syntax Error");
    }
    node->right = createNode(token);
    pom = node;
    parse_assign();
    break;
  case UNDERSCORE:
    node->left = createNode(token);
    pom = node->left;
    token = getNextToken();
    if (token->type != EQUAL) {
      error(2, token->line, "Syntax Error");
    }
    node->left = createNode(token);
    pom = node->left;
    parse_expression();
    token = getNextToken();
    if (token->type != SEMICOLON) {
      error(2, token->line, "Syntax Error");
    }
    break;
  case IDENTIFIER:
    if (!strcmp(token->lexeme, "ifj")) {
      token = getNextToken();
      if (token->type != DOT) {
        error(2, token->line, "Syntax Error");
      }
      node->right = createNode(token);
      token = getNextToken();
      if (token->type != IDENTIFIER) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      node = node->left;
      token = getNextToken();
      if (token->type != LEFT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      pom = node->left;
      parse_call_param();
      token = getNextToken();
      if (token->type != SEMICOLON) {
        error(2, token->line, "Syntax Error");
      }
    } else {
      pom = node;
      parse_func_call_check();
      token = getNextToken();
      if (token->type != SEMICOLON) {
        error(2, token->line, "Syntax Error");
      }
    }
    break;
  case IF:
    token = getNextToken();
    if (token->type != LEFT_PAREN) {
      error(2, token->line, "Syntax Error");
    }
    node->left = createNode(token);
    pom = node->left;
    token = getNextToken();
    if (token->type == IDENTIFIER) {
      struct Token *constant = token;
      token = getNextToken();
      if (token->type == RIGHT_PAREN) {
        pom->right = createNode(constant);
      } else {
        ungetToken();
        ungetToken();
        parse_expression();
        token = getNextToken();
        if (token->type != RIGHT_PAREN) {
          error(2, token->line, "Syntax Error");
        }
      }
    } else {
      ungetToken();
      parse_expression();
      token = getNextToken();
      if (token->type != RIGHT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
    }

    pom = node;
    parse_if_while_exp();
    parse_if_while_body();
    parse_else();
    break;
  case WHILE:
    token = getNextToken();
    if (token->type != LEFT_PAREN) {
      error(2, token->line, "Syntax Error");
    }
    node->left = createNode(token);
    pom = node->left;
    token = getNextToken();

    enum TokenType next_token_type = getNextToken()->type;
    ungetToken();

    if (token->type == IDENTIFIER && next_token_type == RIGHT_PAREN) {
      struct Token *constant = token;
      token = getNextToken();
      if (token->type == RIGHT_PAREN) {
        pom->right = createNode(constant);
      }
    } else {
      ungetToken();
      parse_expression();

      token = getNextToken();
      if (token->type != RIGHT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
    }
    pom = node;
    parse_if_while_exp();
    parse_if_while_body();
    break;
  case RETURN:
    pom = node;
    parse_return_expression();
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

void parse_return_expression() {
  struct Token *token = getNextToken();
  if (token->type == SEMICOLON) {
    pom->right = createNode(token);
    return;
  }
  ungetToken();
  parse_expression();
  token = getNextToken(token);
  if (token->type != SEMICOLON) {
    error(2, token->line, "Syntax Error");
  }
}

void parse_else() {
  struct Token *token = getNextToken();
  if (token->type != ELSE) {
    ungetToken();
    return;
  }
  radek = ast_new_line(radek);
  struct treeNode *node = radek;
  node->right = createNode(token);
  pom = node;
  token = getNextToken();
  if (token->type != LEFT_BRACE) {
    error(2, token->line, "Syntax Error");
  }
  parse_if_while_body();
}

void parse_if_while_body() {
  struct Token *token = getNextToken();
  if (token->type == RIGHT_BRACE) {
    radek = ast_new_line(radek);
    struct treeNode *node = radek;
    node->right = createNode(token);
    return;
  }
  parse_body(token);
  parse_if_while_body();
}

void parse_if_while_exp() {
  struct treeNode *node = pom;
  struct Token *token = getNextToken();
  node->right = createNode(token);
  switch (token->type) {
  case LEFT_BRACE:
    break;
  case PIPE:
    token = getNextToken();
    if (token->type != IDENTIFIER) {
      error(2, token->line, "Syntax Error");
    }
    node->right->left = createNode(token);
    token = getNextToken();
    if (token->type != PIPE) {
      error(2, token->line, "Syntax Error");
    }
    token = getNextToken();
    if (token->type != LEFT_BRACE) {
      error(2, token->line, "Syntax Error");
    }
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

void parse_func_call_check() {
  struct Token *token = getNextToken();
  struct treeNode *node = pom;
  switch (token->type) {
  case LEFT_PAREN:
    node->left = createNode(token);
    pom = node->left;
    parse_call_param();
    break;
  case EQUAL:
    node->left = createNode(token);
    pom = node->left;
    parse_expression();
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

void parse_assign() {
  struct Token *token = getNextToken();
  struct treeNode *node = pom;
  switch (token->type) {
  case COLON:
    token = getNextToken();
    if (is_it_not_type(token, node->right)) {
      error(2, token->line, "Syntax Error");
    }
    // node->right->left = createNode(token);
    token = getNextToken();
    if (token->type != EQUAL) {
      error(2, token->line, "Syntax Error");
    }
    node->left = createNode(token);
    pom = node->left;
    parse_expression();
    token = getNextToken();
    if (token->type != SEMICOLON) {
      error(2, token->line, "Syntax Error");
    }
    break;
  case EQUAL:
    node->left = createNode(token);
    pom = node->left;
    parse_expression();
    token = getNextToken();
    if (token->type != SEMICOLON) {
      error(2, token->line, "Syntax Error");
    }
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

void parse_expression() {
  struct Token *token = getNextToken();
  struct Token *constant;
  struct treeNode *node = pom;
  if (token->type == IDENTIFIER) {
    if (!strcmp(token->lexeme, "ifj")) {
      node->left = createNode(token);
      node = node->left;
      token = getNextToken();
      if (token->type != DOT) {
        error(2, token->line, "Syntax Error");
      }
      node->right = createNode(token);
      token = getNextToken();
      if (token->type != IDENTIFIER) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      node = node->left;
      token = getNextToken();
      if (token->type != LEFT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      pom = node->left;
      parse_call_param();
    } else {
      pom = node;
      parse_func_check(token);
    }
    return;
  }
  if (token->type == K_NULL) {
    constant = token;
    token = getNextToken();
    if (token->type == SEMICOLON) {
      node->right = createNode(constant);
      ungetToken();
      return;
    }
  }
  if (token->type == NUMBER || token->type == STRING ||
      token->type == FLOAT_NUMBER) {
    constant = token;
    token = getNextToken();
    if (token->type == SEMICOLON) {
      node->right = createNode(constant);
      ungetToken();
      return;
    }
    ungetToken();
    ungetToken();
    node->right = bottomUpParse();
    return;
  }
  error(2, token->line, "Syntax Error");
}

void parse_func_check(struct Token *id) {
  struct Token *token = getNextToken();
  if (token->type == LEFT_PAREN) {
    pom->left = createNode(id);
    pom = pom->left;
    pom->left = createNode(token);
    pom = pom->left;
    parse_call_param();
  } else if (token->type == SEMICOLON) {
    pom->right = createNode(id);
    pom = pom->right;
    ungetToken();
    return;
  } else {
    ungetToken();
    ungetToken();
    pom->right = bottomUpParse();
  }
}

void parse_call_next_param() {
  struct Token *token = getNextToken();
  switch (token->type) {
  case RIGHT_PAREN:
    break;
  case COMMA:
    parse_call_param();
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

void parse_call_func_param(struct Token *id_name) {
  struct Token *token = getNextToken();
  struct treeNode *node = pom;
  if (token->type == LEFT_PAREN) {
    node = ast_new_parametr(node);
    node->right = createNode(id_name);
    pom = node->right;
    pom->left = createNode(token);
    pom = pom->left;
    parse_call_param();
    pom = node;
    parse_call_next_param();
    return;
  }
  ungetToken();
  node = ast_new_parametr(node);
  node->right = createNode(id_name);
  pom = node;
  parse_call_next_param();
}

void parse_call_param() {
  struct Token *token = getNextToken();
  struct treeNode *node = pom;
  switch (token->type) {
  case RIGHT_PAREN:
    break;
  case NUMBER:
    node = ast_new_parametr(node);
    node->right = createNode(token);
    pom = node;
    parse_call_next_param();
    break;
  case STRING:
    node = ast_new_parametr(node);
    node->right = createNode(token);
    pom = node;
    parse_call_next_param();
    break;
  case IDENTIFIER:
    if (!strcmp(token->lexeme, "ifj")) {
      node = ast_new_parametr(node);
      node->right = createNode(token);
      node = node->right;
      token = getNextToken();
      if (token->type != DOT) {
        error(2, token->line, "Syntax Error");
      }
      node->right = createNode(token);
      token = getNextToken();
      if (token->type != IDENTIFIER) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      node = node->left;
      token = getNextToken();
      if (token->type != LEFT_PAREN) {
        error(2, token->line, "Syntax Error");
      }
      node->left = createNode(token);
      pom = node->left;
      parse_call_param();
      parse_call_next_param();
    } else {
      parse_call_func_param(token);
    }
    break;
  default:
    error(2, token->line, "Syntax Error");
  }
}

struct treeNode *get_root() { return root; }