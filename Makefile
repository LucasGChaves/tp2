# Nome do executável
EXECUTABLE = tp2virtual

# Compilador C
CC = gcc

# Flags de compilação
CFLAGS = -Wall -g

# Arquivos fonte
SOURCES = main.c fifo.c random.c second_chance.c lru.c
# Arquivos objeto (gerados a partir dos arquivos fonte)
OBJECTS = $(SOURCES:.c=.o)

# Regra padrão: compila o executável
all: $(EXECUTABLE)

# Regra para compilar o executável
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Regra para compilar os arquivos objeto 
%.o: %.c page_table_entry.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos objeto e o executável
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)