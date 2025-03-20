#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_free(Symbol* symbol) {

}

SymTable *symtable_create() {
  SymTable *table = malloc(sizeof(SymTable));
  if (!table) {
    return NULL;
  }

  table->items = malloc(SYMTABLE_MAX_SIZE * sizeof(SymTableItem*));
  if (!table->items) {
    free(table);
    return NULL;
  }
  table->size = SYMTABLE_MAX_SIZE;

  // Zero init
  for (int i = 0; i < table->size; i++) {
    table->items[i] = NULL;
  }

  return table;
}

void symtable_destroy(SymTable **table) {
  if (!table || !*table) {
    return;
  }

  SymTable *sym_table = *table;
  for (int i = 0; i < sym_table->size; i++) {
    if (sym_table->items[i]) {
      symbol_free(sym_table->items[i]->symbol);
      free(sym_table->items[i]->key);
      free(sym_table->items[i]);
      sym_table->items[i] = NULL;
    }
  }

  free(sym_table->items);
  free(sym_table);
  *table = NULL;
}

unsigned long symtable_hash(const char *key) {
  unsigned long h = 0;
  const unsigned char *p;
  for (p = (const unsigned char *)key; *p != '\0'; p++)
    h = 65599 * h + *p;
  return h;
}

int symtable_put(SymTable *table, char *key, Symbol *symbol) {
  if (!key) {
    return -1;  // Handle error in `symtable_put`
  }

  unsigned long hash = symtable_hash(key) % table->size;
  unsigned long first_hash = hash;

  while (table->items[hash]) {
    hash = (hash + 1) % table->size;

    // The whole table is full
    if (hash == first_hash) {
      return -1;
    }
  }

  table->items[hash] = malloc(sizeof(SymTableItem));
  if (!table->items[hash]) {
    return -1;
  }

  table->items[hash]->key = malloc(strlen(key) + 1);
  if (!table->items[hash]->key) {
    free(table->items[hash]);
    return -1;
  }

  strcpy(table->items[hash]->key, key);
  table->items[hash]->symbol = symbol;

  return 0;
}

Symbol *symtable_get(SymTable *table, const char *key) {
  unsigned long hash = symtable_hash(key) % table->size;
  unsigned long first_hash = hash;

  do {
    if(table->items[hash] && strcmp(table->items[hash]->key, key) == 0) {
      return table->items[hash]->symbol;
    }

    hash = (hash + 1) % table->size;
  } while(hash != first_hash);

  return NULL;
}

void symtable_remove(SymTable* table, const char *key) {
  unsigned long hash = symtable_hash(key) % table->size;
  unsigned long first_hash = hash;

  do {
    if(table->items[hash] && strcmp(table->items[hash]->key, key) == 0) {
      symbol_free(table->items[hash]->symbol);
      free(table->items[hash]->key);
      free(table->items[hash]);
      table->items[hash] = NULL;
    }

    hash = (hash + 1) % table->size;
  } while(hash != first_hash);
}

void symtable_print(SymTable* table) {
  fprintf(stderr, "SymTable Debug Output START:\n");
  fprintf(stderr, "Table size: %d\n", table->size);
  for(int i = 0; i < table->size; i++) {
    if(table->items[i]) {
      fprintf(stderr, "Item idx=%d: %s\n", i, table->items[i]->key);
    } else {
      fprintf(stderr, "Item idx=%d is empty\n", i);
    }
  }
  fprintf(stderr, "SymTable Debug Output END:\n");
}
