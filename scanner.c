#include "scanner.h"

struct TokenDLL* tokenList = NULL;

struct Token* createToken(enum TokenType type, char* lexeme, int length, int line){
    struct Token* token = (struct Token*)malloc(sizeof(struct Token));
    token->type = type;
    token->lexeme = lexeme;
    token->length = length;
    token->line = line;
    return token;
}

void addToken(struct Token* token){
    struct TokenDLL* newToken = (struct TokenDLL*)malloc(sizeof(struct TokenDLL));
    newToken->token = token;
    newToken->next = NULL;
    newToken->prev = NULL;
    if(tokenList == NULL){
        tokenList = newToken;
    } else {
        struct TokenDLL* current = tokenList;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = newToken;
        newToken->prev = current;
    }
}

void printTokens(){
    struct TokenDLL* current = tokenList;
    while(current != NULL){
        fprintf(stderr, "Token: %d %s at line %d\n", current->token->type, current->token->lexeme, current->token->line);
        current = current->next;
    }
}

struct Token* getToken(){
    if (tokenList == NULL)
    {
        error(1, 0, "getTokens: No tokens available");
    }
    
    return tokenList->token;
}

struct Token* getNextToken(){
    struct Token* token = tokenList->token;
    tokenList = tokenList->next;
    if (token == NULL)
    {
        error(1, token->line, "getNextToken: No tokens available");
    }
    return token;
}

void ungetToken(){
    tokenList = tokenList->prev;
}



struct TokenDLL* scanTokens(FILE* fp){
    int line = 1;
    char c;
    while ((c = fgetc(fp)) != EOF)
    {
        //printf("%c\n", c);
        switch (c)
        {
        case '(':
            addToken(createToken(LEFT_PAREN, "(", 1, line));
            break;
        case ')':
            addToken(createToken(RIGHT_PAREN, ")", 1, line));
            break;
        // case '[':
        //     addToken(createToken(LEFT_SQ_BRACKET, "[", 1, line));
        //     break;
        // case ']':
        //     addToken(createToken(RIGHT_SQ_BRACKET, "]", 1, line));
        //     break;
        case '{':
            addToken(createToken(LEFT_BRACE, "{", 1, line));
            break;
        case '}':
            addToken(createToken(RIGHT_BRACE, "}", 1, line));
            break;
        case ',':
            addToken(createToken(COMMA, ",", 1, line));
            break;
        case ':':
            addToken(createToken(COLON, ":", 1, line));
            break;
        case ';':
            addToken(createToken(SEMICOLON, ";", 1, line));
            break;
        case '.':
            addToken(createToken(DOT, ".", 1, line));
            break;
        case '-':
            addToken(createToken(MINUS, "-", 1, line));
            break;
        case '_':
            addToken(createToken(UNDERSCORE, "_", 1, line));
            break;
        case '+':
            addToken(createToken(PLUS, "+", 1, line));
            break;
        case '*':
            addToken(createToken(STAR, "*", 1, line));
            break;
        case '|':
            addToken(createToken(PIPE, "|", 1, line));
            break;
        case '@':
            addToken(createToken(AT, "@", 1, line));
            break;
        case '?':
            addToken(createToken(QUESTION_MARK, "?", 1, line));
            break;
        case '!':
            if ((c = fgetc(fp)) == '='){
                addToken(createToken(BANG_EQUAL, "!=", 2, line));            
            }   
            else {
                error(1, line, "Unexpected character after '!'");
            }
            break;
        case '=':
            if ((c = fgetc(fp)) == '='){
                addToken(createToken(EQUAL_EQUAL, "==", 2, line));            
            }   
            else {
                addToken(createToken(EQUAL, "=", 1, line));
                ungetc(c, fp);
            }
            break;
        case '>':
            if ((c = fgetc(fp)) == '='){
                addToken(createToken(GREATER_EQUAL, ">=", 2, line));            
            }   
            else {
                addToken(createToken(GREATER, ">", 1, line));
                ungetc(c, fp);
            }
            break;    
        case '<':
            if ((c = fgetc(fp)) == '='){
                addToken(createToken(LESS_EQUAL, "<=", 2, line));            
            }   
            else {
                addToken(createToken(LESS, "<", 1, line));
                ungetc(c, fp);
            }
            break;
        case '/':
            if ((c=fgetc(fp)) == '/') {
                while ((c =fgetc(fp)) != '\n' && c != EOF);
                if (c == EOF)
                {
                    ungetc(c,fp);
                }
                line++;
            } else {
                addToken(createToken(SLASH, "/", 1, line));
                ungetc(c,fp);
            }
            break;
        case ' ':
        case '\t':
        case '\r':
            break;
        case '\n':
            line++;
            break;
        case '"':
            char* lexeme = (char*)malloc(sizeof(char));
            int length = 0;
            while ((c = fgetc(fp)) != '"' && c != EOF){
                lexeme[length++] = c;
                lexeme = (char*)realloc(lexeme, length+1);
            }
            if (c == EOF){
                error(1, line, "EOF");
            }
            lexeme[length] = '\0';
            addToken(createToken(STRING, lexeme, length, line));
            break;
        case '0' ... '9':
            lexeme = (char*)malloc(sizeof(char));
            length = 0;
            int isFloat = 0;
            int isExponential = 0;
            while ((c >= '0' && c <= '9')|| c == '.'|| c == 'e' || c == 'E'){
                if (c == '.'){
                    if (isExponential|| isFloat){
                        error(1, line, "Unexpected '.'");
                    }
                    isFloat = 1;
                }
                else if (c == 'e' || c == 'E')
                {
                    if (isExponential){
                        error(1, line, "Unexpected 'e'");
                    }
                    isExponential = 1;
                    lexeme[length++] = c;
                    lexeme = (char*)realloc(lexeme, length+1);
                    c = fgetc(fp);
                    if (c == '+' || c == '-'){
                        lexeme[length++] = c;
                        lexeme = (char*)realloc(lexeme, length+1);
                        c = fgetc(fp);
                    }
                    if (c < '0' || c > '9'){
                        error(1, line, "Unexpected character after 'e'");
                    }
                }
                lexeme[length++] = c;
                lexeme = (char*)realloc(lexeme, length+1);
                c = fgetc(fp);
            }
            lexeme[length] = '\0';
            if (isFloat|| isExponential){
                addToken(createToken(FLOAT_NUMBER, lexeme, length, line));
            } else
            {
                addToken(createToken(NUMBER, lexeme, length, line));
            }
            ungetc(c, fp);
            break;
        case 'a' ... 'z':
        case 'A' ... 'Z':
            length = 0;
            char *identifier = (char*)malloc(sizeof(char)+1);
            while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'|| (c >= '0' && c <= '9')){
                identifier[length++] = c;
                identifier = (char*)realloc(identifier, length+1);
                c = fgetc(fp);
            }
            identifier[length] = '\0';
            ungetc(c, fp);
            if (strcmp(identifier, "const") == 0){
                addToken(createToken(CONST, identifier, length, line));
            } else if (strcmp(identifier, "else") == 0){
                addToken(createToken(ELSE, identifier, length, line));
            } else if (strcmp(identifier, "fn") == 0){
                addToken(createToken(FN, identifier, length, line));
            } else if (strcmp(identifier, "if") == 0){
                addToken(createToken(IF, identifier, length, line));
            } else if (strcmp(identifier, "i32") == 0){
                addToken(createToken(I32, identifier, length, line));
            } else if (strcmp(identifier, "f64") == 0){
                addToken(createToken(F64, identifier, length, line));
            } else if (strcmp(identifier, "null") == 0){
                addToken(createToken(K_NULL, identifier, length, line));
            } else if (strcmp(identifier, "pub") == 0){
                addToken(createToken(PUB, identifier, length, line));
            } else if (strcmp(identifier, "return") == 0){
                addToken(createToken(RETURN, identifier, length, line));
            // } else if (strcmp(identifier, "u8") == 0){
            //     addToken(createToken(U8, identifier, length, line));
            } else if (strcmp(identifier, "var") == 0){
                addToken(createToken(VAR, identifier, length, line));
            } else if (strcmp(identifier, "void") == 0){
                addToken(createToken(VOID, identifier, length, line));
            } else if (strcmp(identifier, "while") == 0){
                addToken(createToken(WHILE, identifier, length, line));
            } else if (strcmp(identifier, "import") == 0){
                addToken(createToken(IMPORT, identifier, length, line));
            } else {
                addToken(createToken(IDENTIFIER, identifier, length, line));
            }
            break;
        case '[':
            length = 0;
            char *u8 = malloc(sizeof(char));
            u8[length++] = c;
            while ((c=fgetc(fp)) != '\t' && c != '\n' && c != '\r' && c != ' ' && c != EOF && c != '=' && c != '(' && c != ')' && c != '{'  && c != ',' && c != ';'){
                u8[length++] = c;
                u8 = (char*)realloc(u8, length+1);
            }
            u8[length] = '\0';
            ungetc(c,fp);
            if (strcmp(u8, "[]u8") == 0){
                addToken(createToken(U8, u8, length, line));
            } else {
                error(1, line, "Unexpected character");
            }
            break;

        default:
            // printf("%c\n", c);
            error(1, line, "Unexpected character");
            break;
        }
    }
    addToken(createToken(EOF_TOKEN, "EOF", 3, line));
    return tokenList; 
}