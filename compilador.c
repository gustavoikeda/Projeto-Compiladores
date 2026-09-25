#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// DEFINIÇÕES LÉXICAS E ESTRUTURAS DE DADOS
// ============================================================================

// Definição dos nomes dos tokens (usados pelo Parser)[cite: 1]
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,           // Identificadores (variáveis, funções)[cite: 1]
    TOKEN_NUM_INT,      // Números inteiros (Ex: 42)[cite: 1]
    TOKEN_NUM_FLOAT,    // Números reais (Ex: 3.14)[cite: 1]
    TOKEN_OP_REL,       // Operadores relacionais[cite: 1]
    TOKEN_KEYWORD       // Palavras reservadas[cite: 1]
} TokenNome;

// Sub-códigos para especificar o atributo de operadores relacionais[cite: 1]
typedef enum {
    OP_LT,  // < (Less Than)[cite: 1]
    OP_LE,  // <= (Less or Equal)[cite: 1]
    OP_EQ,  // == (Equal)[cite: 1]
    OP_GT,  // > (Greater Than)[cite: 1]
    OP_GE   // >= (Greater or Equal)[cite: 1]
} OpRelAtributo;

// Estrutura do Token com a Union de Atributos[cite: 1]
typedef struct {
    TokenNome type;     // Nome do token[cite: 1]
    int line;           // Para tratamento de erros[cite: 1]
    
    // valor do atributo[cite: 1]
    union {
        int table_index;        // índice para Tabela de Símbolos[cite: 1]
        int int_value;          // Valor literal convertido[cite: 1]
        double float_value;     // Valor literal convertido[cite: 1]
        OpRelAtributo op_code;  // operador relacional específico[cite: 1]
    } attribute;
} Token;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

char *buffer;       // Ponteiro para armazenar o código fonte carregado em memória[cite: 2, 3].
Token lookahead;    // Armazena o símbolo/token atual a ser avaliado pelo analisador sintático[cite: 2, 3].

// TODO: Declarar variáveis globais para controlar a linha atual do ficheiro (ex: int linhaAtual = 1;).
// TODO: Declarar um apontador ou índice de leitura para percorrer o 'buffer' caractere a caractere.

// ============================================================================
// PROTÓTIPOS DE FUNÇÕES
// ============================================================================

// --- Analisador Léxico (Scanner) ---
Token obterToken(); // Lê os caracteres do buffer, agrupa em lexemas e retorna o próximo Token[cite: 2].

// --- Analisador Sintático (Parser) ---
void nextToken();         // Faz chamadas à função obterToken() para alimentar o Parser[cite: 2].
void consome(TokenNome t); // Verifica se o lookahead é igual ao token esperado e avança[cite: 3].

// TODO: Etapa 1 - Após definir a Gramática Livre de Contexto no papel/documentação, 
// criar aqui os protótipos das funções mutuamente recursivas para cada Não-Terminal.
// Exemplo: 
// void programa();
// void declaracoes();
// void comando_se();

// ============================================================================
// IMPLEMENTAÇÃO DO ANALISADOR LÉXICO (SCANNER)
// ============================================================================

Token obterToken() {
    Token tokenRetorno;
    tokenRetorno.type = TOKEN_EOF; // Valor por omissão
    
    // TODO: Implementar a máquina de estados (Diagrama de Transições) para identificar:
    // 1. Remoção de espaços em branco (espaço, \n, \t)[cite: 4].
    // 2. Contagem de linhas (incrementar 'linhaAtual' quando encontrar \n)[cite: 4].
    // 3. Ignorar comentários (ex: // até ao fim da linha)[cite: 4].
    // 4. Reconhecer Números (Inteiros e Reais)[cite: 4].
    // 5. Reconhecer Operadores Relacionais (e definir o op_code correto)[cite: 4].
    // 6. Reconhecer Palavras Reservadas (keywords)[cite: 4].
    // 7. Reconhecer Identificadores[cite: 4].
    
    // TODO: Em caso de falha no reconhecimento, imprimir "ERRO LÉXICO" com a linha e a sequência errada, e usar exit(1)[cite: 2].
    
    // TODO: Antes de retornar o token, imprimir no ecrã e gravar no ficheiro de saída no formato: 
    // "Número da Linha do Átomo# NomeToken | Atributo"[cite: 2].
    
    return tokenRetorno;
}

// ============================================================================
// IMPLEMENTAÇÃO DO ANALISADOR SINTÁTICO (PARSER)
// ============================================================================

void nextToken() {
    // A interação entre léxico e sintático ocorre aqui[cite: 2].
    lookahead = obterToken();
}

void consome(TokenNome tokenEsperado) {
    // TODO: Verificar se lookahead.type == tokenEsperado[cite: 3].
    // Se SIM: chamar nextToken() para avançar[cite: 3].
    // Se NÃO: imprimir "ERRO SINTÁTICO", mostrar o token incorreto e a linha, e usar exit(1) para abortar[cite: 2, 3].
}

// TODO: Implementar as funções de cada Não-Terminal (ex: void programa() { ... })[cite: 3].

// ============================================================================
// FUNÇÃO PRINCIPAL
// ============================================================================

int main(int argc, char *argv[]) {
    // TODO: Verificar se o nome do ficheiro fonte foi passado por linha de comandos (argc > 1)[cite: 2].
    
    // TODO: Abrir o ficheiro fonte no modo de leitura ("r").
    
    // TODO: Alocar memória para o 'buffer' e carregar todo o conteúdo do ficheiro para esta variável[cite: 2].
    // Fechar o ficheiro fonte após o carregamento.
    
    // TODO: Preparar o ficheiro de saída para escrever os tokens gerados.
    
    // Inicialização do processo de compilação
    // nextToken(); // Carrega o primeiro token para o lookahead[cite: 3]
    
    // TODO: Chamar a função do símbolo inicial da gramática (ex: programa();) para iniciar a análise sintática[cite: 3].
    
    // TODO: Libertar a memória alocada para o buffer (free).
    
    return 0;
}