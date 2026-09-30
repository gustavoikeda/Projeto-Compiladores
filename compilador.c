#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// ETAPA 1: DEFINIÇÕES LÉXICAS E ESTRUTURAS DE DADOS
// ============================================================================

/*
 * Enum para os tipos de Tokens.
 * Agrupamos palavras reservadas e símbolos simples (se, var, +) no TOKEN_KEYWORD
 * para simplificar a estrutura. O Analisador Sintático vai diferenciar eles pelo lexema.
 */
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ID,           
    TOKEN_NUM_INT,      
    TOKEN_NUM_FLOAT,    
    TOKEN_OP_REL,       
    TOKEN_KEYWORD       
} TokenNome;

/*
 * Enum para os atributos dos Operadores Relacionais.
 * Isso permite guardar na tabela de símbolos qual operação foi feita (ex: menor que)
 * sem precisar salvar a string (como "<=").
 */
typedef enum {
    OP_LT,  
    OP_LE,  
    OP_EQ,  
    OP_GT,  
    OP_GE,  
    OP_NE   
} OpRelAtributo;

/*
 * Estrutura do Token, baseada na exigência do documento do projeto (Figura 2).
 * Serve para guardar as informações do token identificado no Scanner para enviar ao Parser.
 */
typedef struct {
    TokenNome type;     
    int line;           
   
    union {
        int table_index;        
        int int_value;          
        double float_value;     
        OpRelAtributo op_code;  
    } attribute;
} Token;

// ============================================================================
// VARIÁVEIS GLOBAIS
// Utilizamos variáveis globais para evitar a passagem constante de parâmetros
// nas chamadas recursivas do Analisador Sintático.
// ============================================================================

char *codigo_fonte;       
Token lookahead;          // Guarda o próximo token lido (Lookahead LL(1))
char lexema_atual[100];   // Guarda a string do token que está sendo lido
int linha_atual = 1;      // Contador de linhas para relatórios de erro
int indice_leitura = 0;   // Posição atual da leitura na string do código-fonte
FILE *arquivo_saida;      

// ============================================================================
// ETAPA 2: ANALISADOR LÉXICO (SCANNER)
// ============================================================================

/*
 * Função principal do Scanner.
 * Ela varre o texto, caractere por caractere, para montar e classificar o próximo Token.
 * Espaços em branco e comentários são ignorados e não chegam no Analisador Sintático.
 */
Token obterToken() {
    Token token;
    token.type = TOKEN_EOF; 
    lexema_atual[0] = '\0'; 
    char c;

    // Laço de leitura do código-fonte
    while (codigo_fonte[indice_leitura] != '\0') {
        c = codigo_fonte[indice_leitura];

        // Ignora espaços e formatações
        if (c == ' ' || c == '\t' || c == '\r') {
            indice_leitura++;
            continue;
        }
        
        // Conta as linhas quando encontra o \n
        if (c == '\n') {
            linha_atual++;
            indice_leitura++;
            continue;
        }

        // Ignora comentários iniciados por '//' avançando até o final da linha
        if (c == '/' && codigo_fonte[indice_leitura + 1] == '/') {
            while (codigo_fonte[indice_leitura] != '\n' && codigo_fonte[indice_leitura] != '\0') {
                indice_leitura++;
            }
            continue; 
        }

        token.line = linha_atual; 

        // ----------------------------------------------------------
        // RECONHECIMENTO DE IDENTIFICADORES E PALAVRAS RESERVADAS
        // ----------------------------------------------------------
        
        // Se começar com letra, forma um lexema para ver se é ID ou Palavra Reservada
        if (isalpha(c)) {
            char lexema[100]; 
            int i = 0;
            
            // Aceita letras, números e underline na formação do nome
            while (isalpha(codigo_fonte[indice_leitura]) || isdigit(codigo_fonte[indice_leitura]) || codigo_fonte[indice_leitura] == '_') {
                if (i < 99) { 
                    lexema[i++] = codigo_fonte[indice_leitura];
                }
                indice_leitura++;
            }
            lexema[i] = '\0'; 
            
            strcpy(lexema_atual, lexema);
            
            // Verifica na lista de palavras reservadas da linguagem MiniVisualg
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
                strcmp(lexema, "funcao") == 0 || strcmp(lexema, "retorne") == 0 || strcmp(lexema, "fimfuncao") == 0 ||
                strcmp(lexema, "verdadeiro") == 0 || strcmp(lexema, "falso") == 0 ||
                strcmp(lexema, "MOD") == 0 || strcmp(lexema, "E") == 0 || strcmp(lexema, "OU") == 0) {
                
                token.type = TOKEN_KEYWORD;
            } else {
                token.type = TOKEN_ID; // Se não for palavra reservada, é variável (ID)
            }
            
            return token;
        }

        // ----------------------------------------------------------
        // RECONHECIMENTO DE NÚMEROS (INTEIROS E REAIS)
        // ----------------------------------------------------------
        
        // Se começar com dígito numérico, agrupa até formar o número
        if (isdigit(codigo_fonte[indice_leitura])) {
            int i = 0;
            char lexema[100];
            int numero_real = 0;

            while (isdigit(codigo_fonte[indice_leitura]) && i < 99) {
                lexema[i++] = codigo_fonte[indice_leitura++];
            }
            
            // Verifica se possui o caractere ponto ('.') para identificar que é um número Real (Float)
            if (codigo_fonte[indice_leitura] == '.' && isdigit(codigo_fonte[indice_leitura + 1])) {
                numero_real = 1;
                lexema[i++] = codigo_fonte[indice_leitura++]; 
                while (isdigit(codigo_fonte[indice_leitura]) && i < 99) {
                    lexema[i++] = codigo_fonte[indice_leitura++];
                }
            }
            lexema[i] = '\0';
            strcpy(lexema_atual, lexema);
            
            // Salva o valor exato no atributo do Token conforme o tipo
            if (numero_real) {   
                token.type = TOKEN_NUM_FLOAT;
                token.attribute.float_value = atof(lexema);
            } else {
                token.type = TOKEN_NUM_INT;
                token.attribute.int_value = atoi(lexema);
            }
            
            return token;
        }

        // ----------------------------------------------------------
        // RECONHECIMENTO DE STRINGS LITERAIS
        // ----------------------------------------------------------
        
        if (c == '"') {
            int i = 0;
            char lexema[100];
            lexema[i++] = codigo_fonte[indice_leitura++]; 
            
            // Lê tudo até encontrar o fechamento das aspas (permite acentos no texto)
            while (codigo_fonte[indice_leitura] != '"' && codigo_fonte[indice_leitura] != '\0' && codigo_fonte[indice_leitura] != '\n') {
                if (i < 99) lexema[i++] = codigo_fonte[indice_leitura];
                indice_leitura++;
            }
            
            if (codigo_fonte[indice_leitura] == '"') {
                if (i < 99) lexema[i++] = codigo_fonte[indice_leitura];
                indice_leitura++; 
            }
            
            lexema[i] = '\0';
            strcpy(lexema_atual, lexema);
            
            // Classifica como ID para simplificar o tratamento no analisador sintático
            token.type = TOKEN_ID; 
            return token;
        }

        // ----------------------------------------------------------
        // LOOKAHEAD DE 1 CARACTERE PARA OPERADORES DUPLOS
        // ----------------------------------------------------------
        
        // Verifica o caractere da frente para decidir se é um operador duplo (ex: >=, <-) ou simples (ex: >)
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
            // Operador de atribuição (<-)
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
                token.attribute.op_code = OP_NE;
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
        
        // ----------------------------------------------------------
        // PONTUAÇÕES E OPERADORES MATEMÁTICOS SIMPLES
        // ----------------------------------------------------------
        
        // Captura os caracteres que representam operadores e delimitadores
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == ':' || c == ',' || 
            c == '+' || c == '-' || c == '*' || c == '/' || c == '\\' || c == '.') {
            lexema_atual[0] = c;
            lexema_atual[1] = '\0';
            token.type = TOKEN_KEYWORD; 
            indice_leitura++;
            return token;
        }
                   
        // Se encontrar um caractere que não faz parte da gramática, gera Erro Léxico
        printf("ERRO LÉXICO\n");
        printf("Linha: %d\n", linha_atual);
        printf("Sequência incorreta: %c\n", c);
        exit(1);
    }

    return token;
}

// ============================================================================
// ETAPA 3: ANALISADOR SINTÁTICO (PARSER)
// O parser foi construído no modelo Descendente Recursivo LL(1).
// ============================================================================

void nextToken();
void consome(TokenNome tokenEsperado);
void consome_palavra(const char *palavraEsperada);

void analisa_algoritmo_principal();
void processa_bloco_variaveis();
void avalia_lista_comandos();
void avalia_comando_condicional();
void avalia_comando_repeticao();
void analisa_subrotinas();
void avalia_expressao();
void avalia_fator();
void avalia_termo();
void avalia_expressao_simples();

/*
 * Função responsável por chamar o Léxico sob demanda e atualizar o lookahead.
 * Aqui também aproveitamos para imprimir as informações e gerar o txt de saída.
 */
void nextToken() {
    lookahead = obterToken(); 
    
    if (lookahead.type == TOKEN_EOF) return;

    static int id_counter = 0; // Índice usado para diferenciar os Identificadores na saída

    // Verificações para imprimir o relatório no formato exigido pelo projeto
    if (lookahead.type == TOKEN_KEYWORD) {
        if (isalpha(lexema_atual[0])) {
            printf("%d# PALAVRA_RESERVADA | 0\n", lookahead.line);
            if (arquivo_saida) fprintf(arquivo_saida, "%d# PALAVRA_RESERVADA | 0\n", lookahead.line);
        } else {
            if (strcmp(lexema_atual, "<-") == 0) {
                printf("%d# OPERADOR_ATRIBUICAO | 0\n", lookahead.line);
                if (arquivo_saida) fprintf(arquivo_saida, "%d# OPERADOR_ATRIBUICAO | 0\n", lookahead.line);
            } else {
                printf("%d# OPERADOR_DELIMITADOR | %d\n", lookahead.line, lexema_atual[0]);
                if (arquivo_saida) fprintf(arquivo_saida, "%d# OPERADOR_DELIMITADOR | %d\n", lookahead.line, lexema_atual[0]);
            }
        }
    } else if (lookahead.type == TOKEN_ID) {
        printf("%d# IDENTIFICADOR | %d\n", lookahead.line, id_counter);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# IDENTIFICADOR | %d\n", lookahead.line, id_counter);
        id_counter++;
    } else if (lookahead.type == TOKEN_NUM_INT) {
        printf("%d# NUMERO_INTEIRO | %d\n", lookahead.line, lookahead.attribute.int_value);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# NUMERO_INTEIRO | %d\n", lookahead.line, lookahead.attribute.int_value);
    } else if (lookahead.type == TOKEN_NUM_FLOAT) {
        printf("%d# NUMERO_REAL | %g\n", lookahead.line, lookahead.attribute.float_value);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# NUMERO_REAL | %g\n", lookahead.line, lookahead.attribute.float_value);
    } else if (lookahead.type == TOKEN_OP_REL) {
        printf("%d# OPERADOR_RELACIONAL | %d\n", lookahead.line, lookahead.attribute.op_code);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# OPERADOR_RELACIONAL | %d\n", lookahead.line, lookahead.attribute.op_code);
    }
}

/*
 * Função "consumir" básica. Verifica se o token atual é o que a gramática espera.
 * Se for, avança pro próximo. Se não for, acusa Erro Sintático e para.
 */
void consome(TokenNome tokenEsperado) {
    if (lookahead.type == tokenEsperado) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Token inesperado encontrado: %s\n", lexema_atual); 
        exit(1);
    }
}

/*
 * Função "consumir" para palavras exatas.
 * Usada para checar palavras reservadas específicas (como "inicio", "se", "fimalgoritmo").
 */
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

// ============================================================================
// ANÁLISE SINTÁTICA DE BLOCOS E ESTRUTURAS
// ============================================================================

/*
 * Função raiz do Analisador Sintático.
 * Verifica a estrutura inicial obrigatória do arquivo (algoritmo -> variáveis -> comandos).
 */
void analisa_algoritmo_principal() {
    consome_palavra("algoritmo");
    consome(TOKEN_ID); 

    // Lê todas as declarações de variáveis (var) e funções/procedimentos misturadas antes do início
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "var") == 0 || strcmp(lexema_atual, "procedimento") == 0 || strcmp(lexema_atual, "funcao") == 0)) {
        if (strcmp(lexema_atual, "var") == 0) {
            processa_bloco_variaveis();
        } else {
            analisa_subrotinas();
        }
    }

    consome_palavra("inicio");
    avalia_lista_comandos();
    consome_palavra("fimalgoritmo");
}

/*
 * Faz a análise sintática do bloco 'var' onde se criam as variáveis.
 */
void processa_bloco_variaveis() {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "var") == 0) {
        consome_palavra("var");

        // Continua lendo enquanto encontrar nomes de variáveis
        while (lookahead.type == TOKEN_ID) {
            
            // Aceita a declaração de múltiplas variáveis na mesma linha (ex: a, b: inteiro)
            while (1) 
            {
                consome(TOKEN_ID); 
                if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0)
                {
                    consome_palavra(",");
                } 
                else 
                {
                    break; 
                }
            }
            consome_palavra(":");

            // Verifica se é a declaração de um array (vetor) e seus índices (ex: vetor[1..10])
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "vetor") == 0) {
                consome_palavra("vetor");
                consome_palavra("[");
    
                if (lookahead.type == TOKEN_NUM_INT)
                {
                    consome(TOKEN_NUM_INT);
                } 
                else 
                {
                    consome(TOKEN_ID);
                }

                consome_palavra(".");
                consome_palavra(".");

                if (lookahead.type == TOKEN_NUM_INT) 
                {
                    consome(TOKEN_NUM_INT);
                } 
                else
                {
                    consome(TOKEN_ID);
                }

                consome_palavra("]");
                consome_palavra("de");

                if (lookahead.type == TOKEN_KEYWORD && (
                    strcmp(lexema_atual, "inteiro") == 0 ||
                    strcmp(lexema_atual, "real") == 0 ||
                    strcmp(lexema_atual, "caractere") == 0 ||
                    strcmp(lexema_atual, "logico") == 0))
                {
                    consome_palavra(lexema_atual);
                } 
                else 
                {
                    printf("ERRO SINTÁTICO\n");
                    printf("Linha: %d\n", lookahead.line);
                    printf("Tipo de vetor inválido ou inesperado: '%s'\n", lexema_atual);
                    exit(1);
                }

            } 
            else if 
                (lookahead.type == TOKEN_KEYWORD && (
                strcmp(lexema_atual, "inteiro") == 0 ||
                strcmp(lexema_atual, "real") == 0 ||
                strcmp(lexema_atual, "caractere") == 0 ||
                strcmp(lexema_atual, "logico") == 0)) {
                
                // Consome os tipos de dados simples
                consome_palavra(lexema_atual);

            } 
            else 
            {
                printf("ERRO SINTÁTICO\n");
                printf("Linha: %d\n", lookahead.line);
                printf("Esperado um tipo de dado (inteiro, real, caractere, logico, vetor), mas encontrado '%s'\n", lexema_atual);
                exit(1);
            }
        }
    }
}

/*
 * Lê as instruções dentro dos blocos (como inicio ... fimalgoritmo).
 * Analisa o lookahead para saber qual função do parser chamar em seguida.
 */
void avalia_lista_comandos() {
    // Fica no loop analisando comandos enquanto encontrar identificadores ou comandos-chave
    while (lookahead.type == TOKEN_ID || (lookahead.type == TOKEN_KEYWORD &&
            (strcmp(lexema_atual, "se") == 0 ||
              strcmp(lexema_atual, "enquanto") == 0 ||
              strcmp(lexema_atual, "para") == 0 ||
              strcmp(lexema_atual, "escreva") == 0 ||
              strcmp(lexema_atual, "escreval") == 0 ||
              strcmp(lexema_atual, "leia") == 0 ||
              strcmp(lexema_atual, "retorne") == 0
          ))) {
        
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "se") == 0) {
            avalia_comando_condicional();
        }
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "enquanto") == 0 || strcmp(lexema_atual, "para") == 0)) {
            avalia_comando_repeticao();
        }
        else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "retorne") == 0) {
            consome_palavra("retorne");
            avalia_expressao();
        }
        // Comandos de leitura e escrita (I/O)
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "escreva") == 0 || strcmp(lexema_atual, "escreval") == 0 || strcmp(lexema_atual, "leia") == 0)) {
            
            char comando[100];
            strcpy(comando, lexema_atual);
            
            consome_palavra(comando); 
            consome_palavra("(");     
            
            avalia_expressao();       
            
            // Permite passagem de várias variáveis/expressões separadas por vírgula
            while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                consome_palavra(",");
                avalia_expressao();
            }
            consome_palavra(")");     
        }
        // Tratamento de Identificadores (atribuições de variáveis, índices de vetor ou chamadas de função)
        else if (lookahead.type == TOKEN_ID) {
            consome(TOKEN_ID); 
            
            // Acessando posição do vetor
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
                consome_palavra("[");
                avalia_expressao();
                consome_palavra("]");
            }
            // Passando parâmetros para funções
            else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                if (lookahead.type != TOKEN_KEYWORD || strcmp(lexema_atual, ")") != 0) {
                    avalia_expressao();
                    while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                        consome_palavra(",");
                        avalia_expressao();
                    }
                }
                consome_palavra(")");
            }
            
            // Verifica o símbolo de atribuição para receber um valor
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "<-") == 0) {
                consome_palavra("<-");
                avalia_expressao();   
            }
        }
    }
}

/*
 * Análise Sintática do bloco IF e ELSE.
 */
void avalia_comando_condicional() {
    consome_palavra("se");
    consome_palavra("(");
    avalia_expressao(); 
    consome_palavra(")");
    consome_palavra("entao");

    avalia_lista_comandos();

    // Como o 'senao' é opcional na linguagem, testamos se ele aparece no lookahead
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "senao") == 0) {
        consome_palavra("senao");
        avalia_lista_comandos();
    }

    consome_palavra("fimse");
}

/*
 * Análise Sintática dos laços 'enquanto' e 'para'.
 */
void avalia_comando_repeticao() {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "enquanto") == 0) {
        consome_palavra("enquanto");
        consome_palavra("(");
        avalia_expressao(); 
        consome_palavra(")");
        consome_palavra("faca");
        avalia_lista_comandos();
        consome_palavra("fimenquanto");
    } 
    else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "para") == 0) {
        consome_palavra("para");
        consome(TOKEN_ID); 
        consome_palavra("de");
        avalia_expressao(); 

        consome_palavra("ate");
        avalia_expressao(); 

        // Lê o "passo" do laço Para, que é opcional na sintaxe do Visualg
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "passo") == 0) {
            consome_palavra("passo");
            avalia_expressao(); 
        }

        consome_palavra("faca");
        avalia_lista_comandos();
        consome_palavra("fimpara");
    }
    else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Comando de repetição inválido. Encontrado: %s\n", lexema_atual);
        exit(1);
    }
}

/*
 * Análise Sintática da declaração de Funções e Procedimentos.
 * Lê a assinatura (nome, parâmetros e o tipo dos parâmetros).
 */
void analisa_subrotinas() {
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "procedimento") == 0 || strcmp(lexema_atual, "funcao") == 0)) {
        
        if (strcmp(lexema_atual, "procedimento") == 0) {
            consome_palavra("procedimento");
            consome(TOKEN_ID); 
            
            // Trata a entrada de parâmetros (ex: (a: inteiro, b: real))
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                do {
                    if (strcmp(lexema_atual, ",") == 0) consome_palavra(",");
                    consome(TOKEN_ID);
                    consome_palavra(":");
                    consome(TOKEN_KEYWORD); 
                } while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0);
                consome_palavra(")");
            }
            
            consome_palavra("inicio");
            avalia_lista_comandos();
            consome_palavra("fimprocedimento"); 
            
        } else if (strcmp(lexema_atual, "funcao") == 0) {
            consome_palavra("funcao");
            consome(TOKEN_ID); 
            
            // Trata a entrada de parâmetros na função
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                do {
                    if (strcmp(lexema_atual, ",") == 0) consome_palavra(",");
                    consome(TOKEN_ID);
                    consome_palavra(":");
                    consome(TOKEN_KEYWORD); 
                } while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0);
                consome_palavra(")");
            }
            
            // Consome o tipo de retorno da função
            consome_palavra(":");
            consome(TOKEN_KEYWORD); 
            
            consome_palavra("inicio");
            avalia_lista_comandos();
            consome_palavra("fimfuncao"); 
        }
    }
}

// ============================================================================
// AVALIAÇÃO DE EXPRESSÕES E PRECEDÊNCIA MATEMÁTICA
// A gramática exige avaliar as expressões seguindo a hierarquia correta:
// Parênteses > Multiplicação/Divisão > Soma/Subtração > Operadores Relacionais
// ============================================================================

/*
 * Base da expressão (Fator).
 * Consome números, variáveis e resolve chamadas com parênteses isolados.
 */
void avalia_fator() {
    // Trata sinais negativos ou positivos acompanhando um número
    if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "-") == 0 || strcmp(lexema_atual, "+") == 0)) {
        char op[10];
        strcpy(op, lexema_atual);
        consome_palavra(op);
        avalia_fator(); 
        return;
    }

    // Se for variável ou função
    if (lookahead.type == TOKEN_ID) {
        consome(TOKEN_ID);
        
        // Trata posição de vetor
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
            consome_palavra("[");
            avalia_expressao();
            consome_palavra("]");
        } 
        // Trata chamada de parâmetros da função
        else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
            consome_palavra("(");
            if (lookahead.type != TOKEN_KEYWORD || strcmp(lexema_atual, ")") != 0) {
                avalia_expressao();
                while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                    consome_palavra(",");
                    avalia_expressao();
                }
            }
            consome_palavra(")");
        }
    } else if (lookahead.type == TOKEN_NUM_INT) {
        consome(TOKEN_NUM_INT);
    } else if (lookahead.type == TOKEN_NUM_FLOAT) {
        consome(TOKEN_NUM_FLOAT);
    } else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "verdadeiro") == 0 || strcmp(lexema_atual, "falso") == 0)) {
        if (strcmp(lexema_atual, "verdadeiro") == 0) consome_palavra("verdadeiro");
        else consome_palavra("falso");
    } else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
        // Encontrou um parêntese, então chama avalia_expressao para resolver o que tem dentro com prioridade
        consome_palavra("(");
        avalia_expressao();
        consome_palavra(")");
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Fator inválido na expressão matemática: %s\n", lexema_atual);
        exit(1);
    }
}

/*
 * Resolve multiplicações e divisões (Termo).
 * Tem precedência sobre somas e subtrações.
 */
void avalia_termo() {
    avalia_fator();
    
    // Agrupa e resolve as operações multiplicativas sucessivas
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "*") == 0 || strcmp(lexema_atual, "/") == 0 || 
           strcmp(lexema_atual, "\\") == 0 || strcmp(lexema_atual, "MOD") == 0 || 
           strcmp(lexema_atual, "E") == 0)) {
        
        char operador[10];
        strcpy(operador, lexema_atual);
        consome_palavra(operador);
        avalia_fator();
    }
}

/*
 * Resolve somas e subtrações (Expressão Simples).
 */
void avalia_expressao_simples() {
    avalia_termo();
    
    // Resolve as operações aditivas sucessivas
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "+") == 0 || strcmp(lexema_atual, "-") == 0 || 
           strcmp(lexema_atual, "OU") == 0)) {
        
        char operador[10];
        strcpy(operador, lexema_atual);
        consome_palavra(operador);
        avalia_termo();
    }
}

/*
 * Porta de entrada da validação matemática e lógica.
 * Verifica a presença de operadores relacionais (como >, <, =) ligando duas expressões simples.
 */
void avalia_expressao() {
    avalia_expressao_simples();
    
    if (lookahead.type == TOKEN_OP_REL) {
        consome(TOKEN_OP_REL); 
        avalia_expressao_simples();
    }
}

// ============================================================================
// FUNÇÃO PRINCIPAL (MAIN)
// ============================================================================

int main(int argc, char *argv[]) {
    // Verifica se o arquivo fonte foi passado como parâmetro na execução do terminal
    if (argc < 2) {
        printf("Erro: Ficheiro fonte nao informado.\n");
        printf("Uso: %s <nome_do_arquivo.txt>\n", argv[0]);
        return 1;
    }

    FILE *arquivo_fonte = fopen(argv[1], "r");
    if (arquivo_fonte == NULL) {
        printf("Erro: Nao foi possivel abrir o ficheiro %s\n", argv[1]);
        return 1;
    }

    // Calcula a quantidade de caracteres do arquivo para alocar exatamente 
    // a memória necessária, evitando leitura fora do limite (buffer overflow).
    fseek(arquivo_fonte, 0, SEEK_END);
    long tamanho_ficheiro = ftell(arquivo_fonte);
    rewind(arquivo_fonte);

    codigo_fonte = (char *)malloc((tamanho_ficheiro + 1) * sizeof(char));
    if (codigo_fonte == NULL) {
        printf("Erro: Falha na alocacao de memoria para o codigo_fonte.\n");
        fclose(arquivo_fonte);
        return 1;
    }

    // Passa todo o texto para a string global e fecha o arquivo original
    size_t lidos = fread(codigo_fonte, sizeof(char), tamanho_ficheiro, arquivo_fonte);
    codigo_fonte[lidos] = '\0'; 
    fclose(arquivo_fonte);

    arquivo_saida = fopen("saida_tokens.txt", "w");

    // Dá o pontapé inicial: carrega o primeiro token e chama o analisador sintático
    nextToken(); 
    analisa_algoritmo_principal();
    
    // Verificação de erro para código sujo digitado após a tag "fimalgoritmo".
    // Se o lookahead não for o fim do arquivo (EOF), levanta Erro Sintático.
    if (lookahead.type != TOKEN_EOF) {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Encontrado lixo após o 'fimalgoritmo': %s\n", lexema_atual);
        exit(1);
    }
    
    printf("Compilação concluída com sucesso. Código sintaticamente correto!\n");

    free(codigo_fonte);
    if (arquivo_saida != NULL) fclose(arquivo_saida);

    return 0;
}
