//
// Created by matejbucek on 28.9.24.
//

#ifndef IFJ24_HASHTABLE_H
#define IFJ24_HASHTABLE_H

#define SYMTABLE_MAX_SIZE 200
#include "scanner.h"

typedef struct Symbol {
  char *id;
  enum TokenType type;
  int constant; // 0 var 1 const
  int nullable; // 0 can't be null 1 can be null
  enum TokenType returnType;
  enum TokenType *paramtypes;
  int number_of_params;
} Symbol;

void symbol_free(Symbol* symbol);

typedef struct SymTableItem {
  Symbol* symbol;
  char* key;
} SymTableItem;

typedef struct SymTable {
  SymTableItem** items;
  int size;
} SymTable;

/*
 * @brief Creates a new symbol table
 * @return Pointer to the newly created symbol table
 */
SymTable* symtable_create();

/*
 * @brief Destroys the symbol table and nullifies the pointer
 * @param table Pointer to the symbol table pointer
 */
void symtable_destroy(SymTable** table);

/*
 * @brief Hashes the key
 * @param key Key to hash
 * @return Hashed key
 */
unsigned long symtable_hash(const char* key);

/*
 * @brief Puts a symbol into the symbol table
 * @param table Symbol table
 * @param key Key to put the symbol under
 * @param symbol Symbol to put into the table
 * @return 0 if successful, -1 if not
 */
int symtable_put(SymTable* table, char* key, Symbol* symbol);

//void create_symbol(char *id, Tokentype type, Tokentype returnType,Tokentype* params);

/*
 * @brief Gets a symbol from the symbol table
 * @param table Symbol table
 * @param key Key to get the symbol from
 * @return Symbol if successful, NULL if not
 */
Symbol* symtable_get(SymTable* table, const char *key);

/*
 * @brief Removes a symbol from the symbol table
 * @param table Symbol table
 * @param key Key to remove the symbol from
 */
void symtable_remove(SymTable* table, const char *key);

/*
 * @brief Prints the symbol table
 * @param table Symbol table
 */
void symtable_print(SymTable* table);

#endif // IFJ24_HASHTABLE_H
