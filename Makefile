# Nome do executável
EXECUTABLE = tp2virtual
EXECUTABLE_TWO_LEVELS = tp2virtual_two_levels
EXECUTABLE_INVERTED = tp2virtual_inverted
# Compilador C
CC = gcc

# Flags de compilação
CFLAGS = -Wall -g

# Arquivos fonte
SOURCES = main.c fifo.c random.c second_chance.c lru.c common.c
# Arquivos objeto (gerados a partir dos arquivos fonte)
OBJECTS = $(SOURCES:.c=.o)


SOURCES_TWO_LEVELS = main_two_levels.c lru_two_levels.c common.c
OBJECTS_TWO_LEVELS = $(SOURCES_TWO_LEVELS:.c=.o)

SOURCES_INVERTED = main_inverted.c lru_inverted.c common.c
OBJECTS_INVERTED = $(SOURCES_INVERTED:.c=.o)

# Regra padrão: compila o executável
all: $(EXECUTABLE) $(EXECUTABLE_TWO_LEVELS) $(EXECUTABLE_INVERTED)

# Regra para compilar o executável
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

$(EXECUTABLE_TWO_LEVELS): $(OBJECTS_TWO_LEVELS)
	$(CC) $(CFLAGS) $(OBJECTS_TWO_LEVELS) -o $@

$(EXECUTABLE_INVERTED): $(OBJECTS_INVERTED)
	$(CC) $(CFLAGS) $(OBJECTS_INVERTED) -o $@

# Regra para compilar os arquivos objeto 
%.o: %.c page_table_entry.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c two_levels_structs.h
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c inverted_structs.h
	$(CC) $(CFLAGS) -c $< -o $@
# Regra para limpar os arquivos objeto e o executável
clean:
	rm -f $(OBJECTS) $(OBJECTS_TWO_LEVELS) $(EXECUTABLE) $(EXECUTABLE_TWO_LEVELS) $(EXECUTABLE_INVERTED)