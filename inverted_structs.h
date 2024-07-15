#ifndef INVERTED_STRUCTS_H
#define INVERTED_STRUCTS_H

// Estrutura da entrada da tabela de páginas invertida
typedef struct InvertedPageTableEntry
{
    int processId;                       // ID do processo
    unsigned long int pageNumber;        // Número da página virtual
    unsigned long int frameNumber;                     // Número do quadro físico (índice na tabela)
    int valid;                           // Bit de validade
    unsigned long int lastAccess;
    struct InvertedPageTableEntry *next; // Ponteiro para a próxima entrada na lista encadeada (para tratar colisões)
} InvertedPageTableEntry;

// Estrutura da tabela de páginas invertida
typedef struct InvertedPageTable
{
    int tableSize;                    // Tamanho da tabela (número de quadros físicos)
    InvertedPageTableEntry **entries; // Array de ponteiros para as entradas da tabela (para usar listas encadeadas)
} InvertedPageTable;

#endif
