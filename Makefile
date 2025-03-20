# Název spustitelného souboru
TARGET = program

# Kompilátor a kompilátorové příznaky
CC = gcc
CFLAGS = -g -Wno-discarded-qualifiers

# Zdrojové soubory
SRCS = main.c scanner.c error.c syntax.c expression.c tree.c seman.c symtable.c symstack.c codegen.c MemoryMap.c

# Složka pro objektové soubory
BUILD_DIR = build

# Zkompilované objektové soubory v build složce
OBJS = $(SRCS:%.c=%.o)

# Výchozí cíl, kompiluje spustitelný program
all: $(TARGET)

# Pravidlo pro kompilaci spustitelného souboru
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pravidlo pro kompilaci .o souborů z .c a umístění do build složky
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ 

# Vytvoří build složku, pokud neexistuje
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Vyčistí objektové soubory a spustitelný soubor
clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(BUILD_DIR)

# Spustí program
run: $(TARGET)
	./$(TARGET)

zip:
	zip -r xbucek17.zip *.c *.h Makefile rozdeleni rozsireni dokumentace.pdf
