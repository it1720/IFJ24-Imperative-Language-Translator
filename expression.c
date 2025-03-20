#include "expression.h"
#include "tree.h"

//      +-      */     !=<>    (       )       i       $   
int precTable[7][7]={
      {REDUCE, SHIFT, REDUCE, SHIFT, REDUCE, SHIFT, REDUCE}, // +-
      {REDUCE, REDUCE, REDUCE, SHIFT, REDUCE, SHIFT, REDUCE}, // */
      {SHIFT, SHIFT, REDUCE, SHIFT, REDUCE, SHIFT, REDUCE}, // !=<>
      {SHIFT, SHIFT, SHIFT, SHIFT, PREC_EQUAL, SHIFT, ERROR}, // (
      {REDUCE, REDUCE, REDUCE, ERROR, REDUCE, ERROR, REDUCE}, // )
      {REDUCE, REDUCE, REDUCE, ERROR, REDUCE, ERROR, REDUCE}, // i
      {SHIFT, SHIFT, SHIFT, SHIFT, ERROR, SHIFT, FINAL} // $
};



struct exprDLL* exprList = NULL;



void pushNode(struct treeNode* node){
      struct exprDLL* newNode = (struct exprDLL*)malloc(sizeof(struct exprDLL));
      newNode->node = node;
      newNode->next = NULL;
      newNode->prev = NULL;
      newNode->active = NULL;

      if(exprList == NULL){
            exprList = newNode;
            exprList->active = exprList;
      }else{
            newNode->prev = exprList->active;
            exprList->active->next = newNode;
            exprList->active = newNode;
            }
}

struct treeNode* popNode(){
      struct treeNode* node = exprList->active->node;
      struct exprDLL* temp = exprList->active;
      exprList->active = exprList->active->prev;
      free(temp);
      return node;
}

void pushToken(struct Token* token){
      struct treeNode* newNode = createNode(token);
      pushNode(newNode);
}     

int tableAction(enum TokenType type){
      switch (type)
      {
      case PLUS:
      case MINUS:
            return 0;
      case STAR:
      case SLASH:
            return 1;
      case BANG_EQUAL:
      case EQUAL_EQUAL:
      case GREATER:
      case GREATER_EQUAL:
      case LESS:
      case LESS_EQUAL:
            return 2;
      case LEFT_PAREN:
            return 3;
      case RIGHT_PAREN:
            return 4;
      case IDENTIFIER:
      case NUMBER:
      case FLOAT_NUMBER:
            return 5;
      default:
            return 6;
      }
}

int reduce(){
      if (exprList->active->node->isExpression)
      {
            if (exprList->active->prev->node->token->type==EOF_TOKEN)
            {
                  fprintf(stderr, "Expression: ");
                  return 0;
            }           
            struct treeNode* right = popNode();
            struct treeNode* operator = popNode();
            struct treeNode* left = popNode();
            operator->left = left;
            operator->right = right;
            operator->isExpression = 1;
            pushNode(operator);
      }
      else if (exprList->active->node->token->type==IDENTIFIER || exprList->active->node->token->type==NUMBER || exprList->active->node->token->type==FLOAT_NUMBER)
      {
            exprList->active->node->isExpression = 1;
      }
      else{
            error(2, exprList->active->node->token->line, "Reduction error.\n");
      }
            
      return 1;
}

enum TokenType getLastTokenType(){
      struct exprDLL* temp = exprList->active;
      while (temp->node->isExpression)
      {
            temp = temp->prev;
      }
      return temp->node->token->type;
}

struct treeNode* bottomUpParse(){
      struct Token* start = createToken(EOF_TOKEN, "$", 1, 0);
      pushToken(start);
      int bracket = 0;
      struct Token* token = getNextToken();
      fprintf(stderr, "Token: %s\n", token->lexeme);

      if (token->type != IDENTIFIER && token->type != NUMBER && token->type != FLOAT_NUMBER && token->type != LEFT_PAREN)
      {
            error(2, token->line, "Expression error");
      }
      else
      {
            pushToken(token);
      }
      token = getNextToken();
      while (1)
      {
            if (token->type != IDENTIFIER && token->type != NUMBER && token->type != FLOAT_NUMBER && token->type != LEFT_PAREN && token->type != RIGHT_PAREN
            && token->type != PLUS && token->type != MINUS && token->type != STAR && token->type != SLASH && token->type != BANG_EQUAL && token->type != EQUAL_EQUAL
            && token->type != GREATER && token->type != GREATER_EQUAL && token->type != LESS && token->type != LESS_EQUAL)
            {
                  break;
            }
            if (bracket < 0)
            {
                  break;
            }
            enum TokenType top = getLastTokenType();
            int topIndex = tableAction(top);
            int tokenIndex = tableAction(token->type);
            enum Action action = precTable[topIndex][tokenIndex];
            // printf("TopIndex: %d\n", topIndex);
            // printf("TokenIndex: %d\n", tokenIndex);
            switch (action)
            {
            case SHIFT:
                  // printf("SHIFT\n");
                  pushToken(token);
                  token = getNextToken();
                  if (token->type == LEFT_PAREN)
                  {
                        bracket++;
                  }
                  else if (token->type == RIGHT_PAREN)
                  {
                        bracket--;
                  }
                  break;
            case REDUCE:
                  // printf("REDUCE\n");
                  reduce();
                  break;
            case ERROR:
                  fprintf(stderr, "Top: %d\n", topIndex);
                  fprintf(stderr, "Token: %d\n", tokenIndex);
                  error(2, token->line, "Expression error");
            case FINAL:
            case PREC_EQUAL:
                  struct treeNode* tmp = popNode();
                  popNode();
                  pushNode(tmp);
                  token = getNextToken();
                  if (token->type == LEFT_PAREN)
                  {
                        bracket++;
                  }
                  else if (token->type == RIGHT_PAREN)
                  {
                        bracket--;
                  }
                  break;           
            }   
      }
      while (reduce())
      {
            /* code */
      }
      ungetToken(token);
      // printTree(exprList->active->node,0);
      return exprList->active->node;
}
