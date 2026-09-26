#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// ETAPA 1: DEFINIÇÕES LÉXICAS E ESTRUTURAS DE DADOS
// ============================================================================

// Definição dos nomes dos tokens (usados pelo Parser)
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,           // Identificadores (variáveis, funções)
    TOKEN_NUM_INT,      // Números inteiros (Ex: 42)
    TOKEN_NUM_FLOAT,    // Números reais (Ex: 3.14)
    TOKEN_OP_REL,       // Operadores relacionais
    TOKEN_KEYWORD,      // Palavras reservadas
    
    // NOTA: Para o Parser funcionar na Etapa 3, precisaremos de estender este enum 
    // futuramente para incluir TOKEN_OP_ARIT (+, -), TOKEN_ATRIBUICAO (<-) 
    // e TOKEN_PONTUACAO ( (, ), :, , ).
} TokenNome;

// Sub-códigos para especificar o atributo de operadores relacionais
typedef enum {
    OP_LT,  // < (Less Than)
    OP_LE,  // <= (Less or Equal)
    OP_EQ,  // == (Equal)
    OP_GT,  // > (Greater Than)
    OP_GE   // >= (Greater or Equal)
} OpRelAtributo;

// Estrutura do Token com a Union de Atributos
typedef struct {
    TokenNome type;     // Nome do token
    int line;           // Para tratamento de erros
    char lexema[100];
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

char *codigo_fonte;             // Ponteiro para armazenar o código fonte carregado em memória.
Token lookahead;          // Armazena o símbolo/token atual a ser avaliado pelo sintático.

int linha_atual = 1;      // Controla a linha atual para relatórios de erro.
int indice_leitura = 0;   // CORRIGIDO: Em C, os índices de arrays/strings começam em 0.

FILE *arquivo_saida;      // Ponteiro para o ficheiro onde os tokens serão guardados.

// ============================================================================
// ETAPA 2: ANALISADOR LÉXICO (SCANNER)
// ============================================================================

Token obterToken() {
    Token token;
    token.type = TOKEN_EOF; // Valor por omissão
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
        
        // TODO: Implementar lógica para Números (TOKEN_NUM_INT, TOKEN_NUM_FLOAT)
        
        // TODO: Implementar lógica para Identificadores e Palavras Reservadas (TOKEN_ID, TOKEN_KEYWORD)
        // ==========================================================
        // 3. MÁQUINA DE ESTADOS: RECONHECIMENTO DE TOKENS
        // ==========================================================
        
        // Reconhecimento de Identificadores e Palavras Reservadas
        // A transição inicial exige que o lexema comece obrigatoriamente por uma letra.
        if (isalpha(c)) {
            char lexema[100]; // codigo_fonte temporário para armazenar a palavra
            int i = 0;
            
            // O estado avança e consome os próximos caracteres enquanto forem letras, dígitos ou sublinhados
            while (isalpha(codigo_fonte[indice_leitura]) || isdigit(codigo_fonte[indice_leitura]) || codigo_fonte[indice_leitura] == '_') {
                if (i < 99) { // Proteção contra overflow do codigo_fonte
                    lexema[i++] = codigo_fonte[indice_leitura];
                }
                indice_leitura++;
            }
            lexema[i] = '\0'; // Finaliza a string do lexema
            
            // NOTA SOBRE O RETRACT: Como a leitura avalia diretamente o índice do array (codigo_fonte[indice_leitura]),
            // quando o laço while falha (encontrando o caractere "other"), o 'indice_leitura' fica 
            // perfeitamente posicionado nesse caractere limitador. Isto simula nativamente a operação retract()
            
            // Classificação: Prioridade das Palavras Reservadas sobre Identificadores comuns
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

        if(isdigit(codigo_fonte[indice_leitura]))
        {
        int i = 0;
        char lexema[100];
        int numero_real = 0;

        while(isdigit(codigo_fonte[indice_leitura]) && i < 100)
        {
            lexema[i++] = codigo_fonte[indice_leitura++];
        }
        if(isdigit(codigo_fonte[indice_leitura + 1]) && codigo_fonte[indice_leitura] == '.')
        {
            numero_real = 1;
            lexema[i++] = codigo_fonte[indice_leitura++];
            while(isdigit(codigo_fonte[indice_leitura]) && i < 100)
            {
                lexema[i++] = codigo_fonte[indice_leitura++];
            }
        }
        lexema[i] = '\0';
        if(numero_real)
        {   
            token.type = TOKEN_NUM_FLOAT;
            token.attribute.float_value = atof(lexema);
        }
        else
        {
            token.type = TOKEN_NUM_INT;
            token.attribute.int_value = atoi(lexema);
        }
        token.line = linha_atual;
        return token;

    }

        // TODO: Implementar lógica para Operadores Relacionais (TOKEN_OP_REL) e Atribuição (<-)
        // ==========================================================
        // LÓGICA PARA OPERADORES RELACIONAIS E ATRIBUIÇÃO
        // ==========================================================
        
        if (c == '>') {

            if (codigo_fonte[indice_leitura + 1] == '=') {
                // Maior ou igual: >=[cite: 6]
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GE;
                indice_leitura += 2;
                return token;
            } else {
                // Maior: >[cite: 6]
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GT;
                indice_leitura++;
                return token;
            }

        } else if (c == '<') {
            
            // Verifica o próximo caractere sem avançar o índice principal (simula o lookahead e evita o retract).
            if (codigo_fonte[indice_leitura + 1] == '-') {
                // Atribuição: <-
                token.type = TOKEN_KEYWORD; // Pode criar um TOKEN_ATRIBUICAO no enum futuramente
                indice_leitura += 2;
                return token;
            } else if (codigo_fonte[indice_leitura + 1] == '=') {
                // Menor ou igual: <=
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_LE;
                indice_leitura += 2;
                return token;
            } else if (codigo_fonte[indice_leitura + 1] == '>') {
                // Diferente: <>
                token.type = TOKEN_OP_REL;
                // NOTA: O enum do enunciado não possui OP_NE, mas o operador existe no Anexo I.
                indice_leitura += 2;
                return token;
            } else {
                // Menor: <[cite: 6]
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_LT;
                indice_leitura++;
                return token;
            }

        } else if (c == '=') {
            // Igual: =
            // O Anexo I estipula que a comparação de igualdade utiliza o sinal "=" (ex: senhaDigitada = 1234)[cite: 6].
            token.type = TOKEN_OP_REL;
            token.attribute.op_code = OP_EQ;
            indice_leitura++;
            return token;
        }            
        // ==========================================================
        // TRATAMENTO DE ERRO LÉXICO
        // ==========================================================
        // Se chegou aqui, encontrou um caractere inválido
        printf("ERRO LÉXICO\n");
        printf("Linha: %d\n", linha_atual);
        printf("Sequência incorreta: %c\n", c);
        exit(1); // Aborta a compilação
    }

    return token;
}

// ============================================================================
// ETAPA 3: ANALISADOR SINTÁTICO (PARSER)
// ============================================================================


void nextToken();
void consome(TokenNome tokenEsperado);
void consome_palavra(const char *palavraEsperada);

// Protótipos das funções da Gramática (Nomenclatura Descritiva - Opção 1)
void analisa_algoritmo_principal();
void processa_bloco_variaveis();
void avalia_lista_comandos();
void avalia_comando_condicional();
void avalia_comando_repeticao();
void analisa_subrotinas();
void avalia_expressao();

// --- Funções Auxiliares do Sintático ---

void nextToken() {
    lookahead = obterToken();
}

void consome(TokenNome tokenEsperado) {
    if (lookahead.type == tokenEsperado) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Token inesperado encontrado: %s\n", lookahead.lexema);
        exit(1);
    }
}

// Nova função para consumir palavras reservadas específicas com precisão
void consome_palavra(const char *palavraEsperada) {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lookahead.lexema, palavraEsperada) == 0) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Esperado '%s', mas encontrado '%s'\n", palavraEsperada, lookahead.lexema);
        exit(1);
    }
}

// --- Funções Recursivas (Não-Terminais da Gramática) ---

void analisa_algoritmo_principal() {
    // TODO: Consumir palavra "algoritmo"[cite: 6]
    // TODO: Consumir nome do programa (TOKEN_ID ou TOKEN_STRING dependendo de como fizerem)
    // TODO: Chamar processa_bloco_variaveis() (se existir o bloco var)[cite: 7]
    // TODO: Chamar analisa_subrotinas() (se existirem procedimentos/funcoes declaradas)
    // TODO: Consumir palavra "inicio"[cite: 6]
    // TODO: Chamar avalia_lista_comandos()[cite: 7]
    // TODO: Consumir palavra "fimalgoritmo"[cite: 6]
}

void processa_bloco_variaveis() {
    // TODO: Verificar se lookahead é "var". Se for, consumir.[cite: 6]
    // TODO: Fazer um loop (while) enquanto o lookahead for um TOKEN_ID[cite: 7]
    // TODO: Dentro do loop, consumir TOKEN_ID, consumir ':' (requer adicionar ao léxico)
    // TODO: Consumir o tipo (inteiro, real, caractere, logico)[cite: 6]
}

void avalia_lista_comandos() {
    // TODO: Fazer um loop avaliando o lookahead[cite: 7]
    // TODO: Se for "se", chama avalia_comando_condicional()
    // TODO: Se for "enquanto" ou "para", chama avalia_comando_repeticao()
    // TODO: Se for "escreva", "escreval", ou "leia", consome e avalia os parâmetros[cite: 6]
    // TODO: Se for TOKEN_ID, é atribuição ou chamada de procedimento (ex: variavel <- expressao)
}

void avalia_comando_condicional() {
    // TODO: Consumir "se"[cite: 6]
    // TODO: Consumir '('
    // TODO: Chamar avalia_expressao() (expressão lógica)[cite: 7]
    // TODO: Consumir ')'
    // TODO: Consumir "entao"[cite: 6]
    // TODO: Chamar avalia_lista_comandos() (para o bloco verdadeiro)
    // TODO: Fatoração à esquerda: verificar se o lookahead é "senao".[cite: 7]
    //       Se for, consumir "senao" e chamar avalia_lista_comandos() para o bloco falso.
    // TODO: Consumir "fimse"[cite: 6]
}

void avalia_comando_repeticao() {
    // TODO: Verificar se é um loop "enquanto" ou "para"[cite: 6, 7]
    // TODO: Lógica para "enquanto (exp) faca ... fimenquanto"
    // TODO: Lógica para "para ID de EXP ate EXP passo EXP faca ... fimpara"
}

void analisa_subrotinas() {
    // TODO: Lógica para consumir "procedimento" ou "funcao" e seus blocos internos[cite: 6, 7]
}

void avalia_expressao() {
    // TODO: Lógica para avaliar contas matemáticas ou comparações booleanas[cite: 7]
    // DICA: Para fatorar a esquerda e respeitar a ordem matemática, 
    // costuma-se dividir em avalia_termo() e avalia_fator()[cite: 7].
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

    fread(codigo_fonte, sizeof(char), tamanho_ficheiro, arquivo_fonte);
    codigo_fonte[tamanho_ficheiro] = '\0'; // Garante o fim da string
    fclose(arquivo_fonte);

    // 4. Preparar ficheiro de saída
    arquivo_saida = fopen("saida_tokens.txt", "w");

    // ========================================================================
    // TESTE DA ETAPA 3 (ANALISADOR SINTÁTICO ATIVADO)
    // ========================================================================
    
    // Inicializa a variável lookahead com o primeiro token do arquivo[cite: 7]
    nextToken(); 
    
    // Inicia a análise descendente pela regra principal da gramática[cite: 7]
    analisa_algoritmo_principal();
    
    printf("Compilação concluída com sucesso. Código sintaticamente correto!\n");

    /* BLOCO DA ETAPA 2 (COMENTADO) - Descomente se quiser apenas imprimir os tokens
    Token t;
    do {
        t = obterToken();
        printf("Lido token tipo %d com lexema '%s'\n", t.type, t.lexema);
    } while (t.type != TOKEN_EOF);
    */

    // Limpeza de memória e fecho de ficheiros
    free(codigo_fonte);
    if (arquivo_saida != NULL) fclose(arquivo_saida);

    return 0;
}