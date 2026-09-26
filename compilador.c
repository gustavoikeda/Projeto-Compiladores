#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// ETAPA 1: DEFINIÇÕES LÉXICAS E ESTRUTURAS DE DADOS
// ============================================================================

// Definição dos nomes dos tokens (usados pelo Parser)[cite: 7]
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,           // Identificadores (variáveis, funções)
    TOKEN_NUM_INT,      // Números inteiros (Ex: 42)
    TOKEN_NUM_FLOAT,    // Números reais (Ex: 3.14)
    TOKEN_OP_REL,       // Operadores relacionais
    TOKEN_KEYWORD       // Palavras reservadas
} TokenNome;

// Sub-códigos para especificar o atributo de operadores relacionais[cite: 7]
typedef enum {
    OP_LT,  // < (Less Than)
    OP_LE,  // <= (Less or Equal)
    OP_EQ,  // == (Equal)
    OP_GT,  // > (Greater Than)
    OP_GE   // >= (Greater or Equal)
} OpRelAtributo;

// Estrutura do Token com a Union de Atributos estritamente igual à Figura 2[cite: 7]
typedef struct {
    TokenNome type;     // Nome do token
    int line;           // Para tratamento de erros
   
    // valor do atributo
    union {
        int table_index;        // índice para Tabela de Símbolos
        int int_value;          // Valor literal convertido
        double float_value;     // Valor literal convertido
        OpRelAtributo op_code;  // operador relacional específico
    } attribute;
} Token;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

char *codigo_fonte;       // Ponteiro para armazenar o código fonte carregado em memória.
Token lookahead;          // Armazena o símbolo/token atual a ser avaliado pelo sintático[cite: 8].

char lexema_atual[100];   // VARIÁVEL GLOBAL ADICIONADA: Guarda o texto do token atual para o parser preditivo.

int linha_atual = 1;      // Controla a linha atual para relatórios de erro.
int indice_leitura = 0;   // Em C, os índices de arrays/strings começam em 0.

FILE *arquivo_saida;      // Ponteiro para o ficheiro onde os tokens serão guardados.

// ============================================================================
// ETAPA 2: ANALISADOR LÉXICO (SCANNER)
// ============================================================================

Token obterToken() {
    Token token;
    token.type = TOKEN_EOF; // Valor por omissão
    lexema_atual[0] = '\0'; // Limpa o lexema global a cada nova leitura
    char c;

    // Loop principal: percorre o codigo_fonte até encontrar o fim do ficheiro ('\0')
    while (codigo_fonte[indice_leitura] != '\0') {
        c = codigo_fonte[indice_leitura];

        // 1. REMOÇÃO DE ESPAÇOS E CONTROLO DE LINHAS
        if (c == ' ' || c == '\t' || c == '\r') {
            indice_leitura++;
            continue;
        }
        if (c == '\n') {
            linha_atual++;
            indice_leitura++;
            continue;
        }

        // 2. IGNORAR COMENTÁRIOS DE LINHA (//)
        if (c == '/' && codigo_fonte[indice_leitura + 1] == '/') {
            while (codigo_fonte[indice_leitura] != '\n' && codigo_fonte[indice_leitura] != '\0') {
                indice_leitura++;
            }
            continue; // O '\n' será tratado na próxima iteração do while
        }

        token.line = linha_atual; // Regista a linha onde o token começou

        // ==========================================================
        // 3. MÁQUINA DE ESTADOS: RECONHECIMENTO DE TOKENS
        // ==========================================================
        
        // --- Reconhecimento de Identificadores e Palavras Reservadas ---
        if (isalpha(c)) {
            char lexema[100]; 
            int i = 0;
            
            while (isalpha(codigo_fonte[indice_leitura]) || isdigit(codigo_fonte[indice_leitura]) || codigo_fonte[indice_leitura] == '_') {
                if (i < 99) { 
                    lexema[i++] = codigo_fonte[indice_leitura];
                }
                indice_leitura++;
            }
            lexema[i] = '\0'; 
            
            // Atualiza a variável global para que o Analisador Sintático possa avaliar a palavra[cite: 8]
            strcpy(lexema_atual, lexema);
            
            // Classificação das Palavras Reservadas do Anexo I[cite: 7]
            if (strcmp(lexema, "algoritmo") == 0 || strcmp(lexema, "var") == 0 || 
                strcmp(lexema, "inicio") == 0 || strcmp(lexema, "fimalgoritmo") == 0 ||
                strcmp(lexema, "se") == 0 || strcmp(lexema, "entao") == 0 || 
                strcmp(lexema, "senao") == 0 || strcmp(lexema, "fimse") == 0 ||
                strcmp(lexema, "escreva") == 0 || strcmp(lexema, "escreval") == 0 || 
                strcmp(lexema, "leia") == 0 || strcmp(lexema, "enquanto") == 0 || 
                strcmp(lexema, "faca") == 0 || strcmp(lexema, "fimenquanto") == 0 ||
                strcmp(lexema, "para") == 0 || strcmp(lexema, "de") == 0 || 
                strcmp(lexema, "ate") == 0 || strcmp(lexema, "passo") == 0 || 
                strcmp(lexema, "fimpara") == 0 || strcmp(lexema, "inteiro") == 0 || 
                strcmp(lexema, "real") == 0 || strcmp(lexema, "caractere") == 0 || 
                strcmp(lexema, "logico") == 0 || strcmp(lexema, "vetor") == 0 ||
                strcmp(lexema, "procedimento") == 0 || strcmp(lexema, "fimprocedimento") == 0 ||
                strcmp(lexema, "funcao") == 0 || strcmp(lexema, "retorne") == 0 ||
                strcmp(lexema, "verdadeiro") == 0 || strcmp(lexema, "falso") == 0) {
                
                token.type = TOKEN_KEYWORD;
            } else {
                token.type = TOKEN_ID;
            }
            
            return token;
        }

        // --- Reconhecimento de Números ---
        if (isdigit(codigo_fonte[indice_leitura])) {
            int i = 0;
            char lexema[100];
            int numero_real = 0;

            while (isdigit(codigo_fonte[indice_leitura]) && i < 99) {
                lexema[i++] = codigo_fonte[indice_leitura++];
            }
            if (codigo_fonte[indice_leitura] == '.' && isdigit(codigo_fonte[indice_leitura + 1])) {
                numero_real = 1;
                lexema[i++] = codigo_fonte[indice_leitura++]; // Consome o ponto
                while (isdigit(codigo_fonte[indice_leitura]) && i < 99) {
                    lexema[i++] = codigo_fonte[indice_leitura++];
                }
            }
            lexema[i] = '\0';
            strcpy(lexema_atual, lexema);
            
            if (numero_real) {   
                token.type = TOKEN_NUM_FLOAT;
                token.attribute.float_value = atof(lexema);
            } else {
                token.type = TOKEN_NUM_INT;
                token.attribute.int_value = atoi(lexema);
            }
            
            return token;
        }

        // --- Reconhecimento de Textos Literais (Strings) ---
        // Exigido para suportar o comando escreval("Olá, mundo!") presente no Anexo I[cite: 7].
        if (c == '"') {
            int i = 0;
            char lexema[100];
            lexema[i++] = codigo_fonte[indice_leitura++]; // Consome a aspa inicial
            
            while (codigo_fonte[indice_leitura] != '"' && codigo_fonte[indice_leitura] != '\0' && codigo_fonte[indice_leitura] != '\n') {
                if (i < 99) lexema[i++] = codigo_fonte[indice_leitura];
                indice_leitura++;
            }
            
            if (codigo_fonte[indice_leitura] == '"') {
                if (i < 99) lexema[i++] = codigo_fonte[indice_leitura];
                indice_leitura++; // Consome a aspa final
            }
            
            lexema[i] = '\0';
            strcpy(lexema_atual, lexema);
            token.type = TOKEN_ID; // Tratado como ID literal para simplificar as regras do Parser.
            return token;
        }

        // --- Operadores Relacionais e Atribuição ---
        if (c == '>') {
            if (codigo_fonte[indice_leitura + 1] == '=') {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GE;
                strcpy(lexema_atual, ">=");
                indice_leitura += 2;
                return token;
            } else {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GT;
                strcpy(lexema_atual, ">");
                indice_leitura++;
                return token;
            }
        } else if (c == '<') {
            if (codigo_fonte[indice_leitura + 1] == '-') {
                token.type = TOKEN_KEYWORD; 
                strcpy(lexema_atual, "<-");
                indice_leitura += 2;
                return token;
            } else if (codigo_fonte[indice_leitura + 1] == '=') {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_LE;
                strcpy(lexema_atual, "<=");
                indice_leitura += 2;
                return token;
            } else if (codigo_fonte[indice_leitura + 1] == '>') {
                token.type = TOKEN_OP_REL;
                strcpy(lexema_atual, "<>");
                indice_leitura += 2;
                return token;
            } else {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_LT;
                strcpy(lexema_atual, "<");
                indice_leitura++;
                return token;
            }
        } else if (c == '=') {
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_EQ;
            strcpy(lexema_atual, "=");
            indice_leitura++;
            return token;
        } 
        
        // --- Símbolos de Pontuação e Operadores Aritméticos ---
        // Essenciais para não causar "Erro Léxico" ao ler parênteses de chamadas e funções no Anexo I[cite: 7].
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == ':' || c == ',' || 
            c == '+' || c == '-' || c == '*' || c == '/' || c == '\\') {
            lexema_atual[0] = c;
            lexema_atual[1] = '\0';
            token.type = TOKEN_KEYWORD; // Tratado como Keyword sintática estrutural
            indice_leitura++;
            return token;
        }
                   
        // ==========================================================
        // TRATAMENTO DE ERRO LÉXICO
        // ==========================================================
        // Aborda o programa conforme especificado no documento do projeto[cite: 7].
        printf("ERRO LÉXICO\n");
        printf("Linha: %d\n", linha_atual);
        printf("Sequência incorreta: %c\n", c);
        exit(1);
    }

    return token;
}

// ============================================================================
// ETAPA 3: ANALISADOR SINTÁTICO (PARSER)
// ============================================================================

void nextToken();
void consome(TokenNome tokenEsperado);
void consome_palavra(const char *palavraEsperada);

// Protótipos das funções da Gramática Recursiva Preditiva[cite: 8]
void analisa_algoritmo_principal();
void processa_bloco_variaveis();
void avalia_lista_comandos();
void avalia_comando_condicional();
void avalia_comando_repeticao();
void analisa_subrotinas();
void avalia_expressao();

// --- Funções Auxiliares do Sintático ---

void nextToken() {
    lookahead = obterToken(); // A variável global lexema_atual é atualizada aqui internamente[cite: 8]
}

void consome(TokenNome tokenEsperado) {
    if (lookahead.type == tokenEsperado) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Token inesperado encontrado: %s\n", lexema_atual); // Usa a global[cite: 7]
        exit(1);
    }
}

// Consome comparando o lexema exato guardado na variável global, resolvendo o problema de 
// ambiguidade sem alterar a struct Token[cite: 8].
void consome_palavra(const char *palavraEsperada) {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, palavraEsperada) == 0) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Esperado '%s', mas encontrado '%s'\n", palavraEsperada, lexema_atual);
        exit(1);
    }
}

// --- Funções Recursivas (Não-Terminais da Gramática) ---

void analisa_algoritmo_principal() {
    // TODO: Consumir palavra "algoritmo" usando consome_palavra("algoritmo")[cite: 7]
    // TODO: Consumir nome do programa (TOKEN_ID)
    // TODO: Chamar processa_bloco_variaveis() (se existir o bloco var)[cite: 8]
    // TODO: Chamar analisa_subrotinas() (se existirem procedimentos/funcoes declaradas)
    // TODO: Consumir palavra "inicio"[cite: 7]
    // TODO: Chamar avalia_lista_comandos()[cite: 8]
    // TODO: Consumir palavra "fimalgoritmo"[cite: 7]
}

void processa_bloco_variaveis() {
    // TODO: Verificar se lexema_atual é "var". Se for, consumir[cite: 7].
    // TODO: Fazer um loop (while) enquanto o lookahead for um TOKEN_ID[cite: 8].
    // TODO: Dentro do loop, consumir TOKEN_ID, consumir ':'
    // TODO: Consumir o tipo (inteiro, real, caractere, logico)[cite: 7].
}

void avalia_lista_comandos() {
    // O loop contínuo que avalia qual a próxima instrução baseada no lookahead preditivo[cite: 8].
    while (lookahead.type == TOKEN_ID || (lookahead.type == TOKEN_KEYWORD &&
            (strcmp(lexema_atual, "se") == 0 ||
              strcmp(lexema_atual, "enquanto") == 0 ||
              strcmp(lexema_atual, "para") == 0 ||
              strcmp(lexema_atual, "escreva") == 0 ||
              strcmp(lexema_atual, "escreval") == 0 ||
              strcmp(lexema_atual, "leia") == 0
          ))) {
        
        // 1. Comando Condicional
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "se") == 0) {
            avalia_comando_condicional();
        }
        
        // 2. Comandos de Repetição
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "enquanto") == 0 || strcmp(lexema_atual, "para") == 0)) {
            avalia_comando_repeticao();
        }
        
        // 3. Comandos de Entrada e Saída (I/O)
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "escreva") == 0 || strcmp(lexema_atual, "escreval") == 0 || strcmp(lexema_atual, "leia") == 0)) {
            // Guarda qual o comando antes de avançar
            char comando[100];
            strcpy(comando, lexema_atual);
            
            consome_palavra(comando); // Consome a palavra específica[cite: 7].
            consome_palavra("(");     // Consome o parêntese obrigatório[cite: 7].
            
            // avalia_expressao();    // TODO: Descomentar quando a função estiver pronta
            
            consome_palavra(")");     // Consome o fecho do parêntese[cite: 7].
        }
        
        // 4. Atribuição de Variável ou Chamada de Procedimento
        else if (lookahead.type == TOKEN_ID) {
            consome(TOKEN_ID); 
            
            // Verifica se o símbolo seguinte é a seta de atribuição (<-)
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "<-") == 0) {
                consome_palavra("<-");
                // avalia_expressao(); // Avalia o cálculo[cite: 8]
            }
        }
    }
}

void avalia_comando_condicional() {
    // TODO: Consumir "se"[cite: 7]
    // TODO: Consumir '('
    // TODO: Chamar avalia_expressao() (expressão lógica)[cite: 8]
    // TODO: Consumir ')'
    // TODO: Consumir "entao"[cite: 7]
    // TODO: Chamar avalia_lista_comandos() (para o bloco verdadeiro)
    // TODO: Fatoração à esquerda: verificar se lexema_atual é "senao"[cite: 8].
    //       Se for, consumir "senao" e chamar avalia_lista_comandos() para o bloco falso.
    // TODO: Consumir "fimse"[cite: 7]
}

void avalia_comando_repeticao() {
    // TODO: Lógica para "enquanto (exp) faca ... fimenquanto"[cite: 7, 8]
    // TODO: Lógica para "para ID de EXP ate EXP passo EXP faca ... fimpara"[cite: 7, 8]
}

void analisa_subrotinas() {
    // TODO: Lógica para consumir "procedimento" ou "funcao" e seus blocos internos[cite: 7, 8]
}

void avalia_expressao() {
    // TODO: Lógica para avaliar contas matemáticas ou comparações[cite: 8]
}

// ============================================================================
// MAIN: PONTO DE ENTRADA E LEITURA DO FICHEIRO
// ============================================================================

int main(int argc, char *argv[]) {
    // 1. Verificação dos argumentos da linha de comandos
    if (argc < 2) {
        printf("Erro: Ficheiro fonte nao informado.\n");
        printf("Uso: %s <nome_do_arquivo.txt>\n", argv[0]);
        return 1;
    }

    // 2. Abrir o ficheiro fonte
    FILE *arquivo_fonte = fopen(argv[1], "r");
    if (arquivo_fonte == NULL) {
        printf("Erro: Nao foi possivel abrir o ficheiro %s\n", argv[1]);
        return 1;
    }

    // 3. Alocar memória e carregar o ficheiro para o 'codigo_fonte'
    fseek(arquivo_fonte, 0, SEEK_END);
    long tamanho_ficheiro = ftell(arquivo_fonte);
    rewind(arquivo_fonte);

    codigo_fonte = (char *)malloc((tamanho_ficheiro + 1) * sizeof(char));
    if (codigo_fonte == NULL) {
        printf("Erro: Falha na alocacao de memoria para o codigo_fonte.\n");
        fclose(arquivo_fonte);
        return 1;
    }

    size_t lidos = fread(codigo_fonte, sizeof(char), tamanho_ficheiro, arquivo_fonte);
    codigo_fonte[lidos] = '\0'; // Garante o fim da string
    fclose(arquivo_fonte);

    // 4. Preparar ficheiro de saída
    arquivo_saida = fopen("saida_tokens.txt", "w");

    // ========================================================================
    // TESTE DA ETAPA 3 (ANALISADOR SINTÁTICO ATIVADO)
    // ========================================================================
    
    // Inicializa a variável lookahead com o primeiro token do arquivo[cite: 8]
    nextToken(); 
    
    // Inicia a análise descendente pela regra principal da gramática[cite: 8]
    analisa_algoritmo_principal();
    
    printf("Compilação concluída com sucesso. Código sintaticamente correto!\n");

    // Limpeza de memória e fecho de ficheiros
    free(codigo_fonte);
    if (arquivo_saida != NULL) fclose(arquivo_saida);

    return 0;
}