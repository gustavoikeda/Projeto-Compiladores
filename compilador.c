/*Guilherme Limeira de Souza   RA: 10439777
Gustavo Kiyoshi Ikeda        RA: 10439179
Pedro Montarroyos de Pinho   RA: 10440213
*/


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
    TOKEN_EOF = 0,      // Representa o fim do arquivo (End Of File)
    TOKEN_ID,           // Representa nomes de variáveis, arrays ou funções (Identificadores)
    TOKEN_NUM_INT,      // Representa números inteiros
    TOKEN_NUM_FLOAT,    // Representa números com vírgula/ponto (reais)
    TOKEN_OP_REL,       // Representa operadores de comparação (>, <, =, etc)
    TOKEN_KEYWORD       // Representa palavras reservadas (inicio, se, var) e símbolos simples (+, -, *)
} TokenNome;

/*
 * Enum para os atributos dos Operadores Relacionais.
 * Isso permite guardar na tabela de símbolos qual operação foi feita (ex: menor que)
 * sem precisar salvar a string (como "<=").
 */
typedef enum {
    OP_LT,  // Less Than (<)
    OP_LE,  // Less or Equal (<=)
    OP_EQ,  // Equal (=)
    OP_GT,  // Greater Than (>)
    OP_GE,  // Greater or Equal (>=)
    OP_NE   // Not Equal (<>)
} OpRelAtributo;

/*
 * Estrutura do Token, baseada na exigência do documento do projeto (Figura 2).
 * Serve para guardar as informações do token identificado no Scanner para enviar ao Parser.
 */
typedef struct {
    TokenNome type;     // Guarda a categoria geral do token (ex: TOKEN_ID)
    int line;           // Guarda a linha em que o token foi encontrado para exibir nos erros
   
    // Usamos uma 'union' pois um token só pode ter um desses valores por vez, economizando memória
    union {
        int table_index;        // Posição na tabela de símbolos (exigência do pdf)
        int int_value;          // O valor exato se for um número inteiro
        double float_value;     // O valor exato se for um número real
        OpRelAtributo op_code;  // O código do operador se for relacional
    } attribute;
} Token;

// ============================================================================
// VARIÁVEIS GLOBAIS
// Utilizamos variáveis globais para evitar a passagem constante de parâmetros
// nas chamadas recursivas do Analisador Sintático.
// ============================================================================

char *codigo_fonte;       // Ponteiro que vai guardar todo o texto lido do arquivo fonte
Token lookahead;          // Guarda o próximo token lido. Fundamental para a gramática LL(1) prever a próxima regra
char lexema_atual[100];   // Array que guarda temporariamente a string (nome) do token que está sendo lido
int linha_atual = 1;      // Contador de linhas, sempre começa na linha 1
int indice_leitura = 0;   // Índice que marca em qual caractere do código_fonte estamos lendo no momento
FILE *arquivo_saida;      // Ponteiro para o arquivo "saida_tokens.txt" onde o resultado será gravado

// ============================================================================
// ETAPA 2: ANALISADOR LÉXICO (SCANNER)
// ============================================================================

/*
 * Função principal do Scanner.
 * Ela varre o texto, caractere por caractere, para montar e classificar o próximo Token.
 * Espaços em branco e comentários são ignorados e não chegam no Analisador Sintático.
 */
Token obterToken() {
    Token token;             // Variável local que vai guardar e retornar o token formado
    token.type = TOKEN_EOF;  // Por padrão, se não achar nada, assume que o arquivo acabou
    lexema_atual[0] = '\0';  // Limpa o lexema anterior preenchendo o começo com fim de string
    char c;                  // Variável temporária para guardar o caractere sendo analisado

    // Laço principal de leitura: continua até encontrar o caractere nulo ('\0') de fim de string
    while (codigo_fonte[indice_leitura] != '\0') {
        c = codigo_fonte[indice_leitura];

        // Ignora espaços e formatações para focar apenas em código útil
        if (c == ' ' || c == '\t' || c == '\r') {
            indice_leitura++; // Avança a fita de leitura
            continue;         // Volta pro começo do laço while
        }
        
        // Conta as linhas toda vez que encontra uma quebra de linha ('\n')
        if (c == '\n') {
            linha_atual++;
            indice_leitura++;
            continue;
        }

        // Ignora comentários: se achar '//', avança a leitura até encontrar a próxima linha
        if (c == '/' && codigo_fonte[indice_leitura + 1] == '/') {
            while (codigo_fonte[indice_leitura] != '\n' && codigo_fonte[indice_leitura] != '\0') {
                indice_leitura++;
            }
            continue; 
        }

        // A partir daqui, sabemos que encontramos algo válido, então salvamos a linha onde ele apareceu
        token.line = linha_atual; 

        // ----------------------------------------------------------
        // RECONHECIMENTO DE IDENTIFICADORES E PALAVRAS RESERVADAS
        // ----------------------------------------------------------
        
        // Se começar com letra, forma um lexema para ver se é ID ou Palavra Reservada
        if (isalpha(c)) {
            char lexema[100]; // Vetor temporário local para agrupar as letras do nome
            int i = 0;        // Contador de posição dentro do vetor temporário
            
            // Um identificador válido pode ter letras, números ou underline
            while (isalpha(codigo_fonte[indice_leitura]) || isdigit(codigo_fonte[indice_leitura]) || codigo_fonte[indice_leitura] == '_') {
                if (i < 99) { 
                    lexema[i++] = codigo_fonte[indice_leitura];
                }
                indice_leitura++;
            }
            lexema[i] = '\0'; // Fecha a string corretamente
            
            strcpy(lexema_atual, lexema); // Passa o nome formado para a variável global
            
            // Como são muitas palavras reservadas, agrupamos tudo nesse IF extenso para classificar como TOKEN_KEYWORD
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
                // Se não deu match com nenhuma palavra da linguagem, então com certeza é o nome de uma variável criada pelo usuário
                token.type = TOKEN_ID; 
            }
            
            return token; // Retorna o token para o sintático
        }

        // ----------------------------------------------------------
        // RECONHECIMENTO DE NÚMEROS (INTEIROS E REAIS)
        // ----------------------------------------------------------
        
        // Se começar com dígito numérico, agrupa os dígitos até formar o número completo
        if (isdigit(codigo_fonte[indice_leitura])) {
            int i = 0;              // Índice local para preencher o lexema
            char lexema[100];       // Vetor temporário para os números
            int numero_real = 0;    // Flag (bandeira) indicando se encontramos um ponto (float)

            // Vai consumindo tudo que for número e copiando
            while (isdigit(codigo_fonte[indice_leitura]) && i < 99) {
                lexema[i++] = codigo_fonte[indice_leitura++];
            }
            
            // Verifica se possui o caractere ponto ('.') seguido de um número para identificar que é um Float (Real)
            if (codigo_fonte[indice_leitura] == '.' && isdigit(codigo_fonte[indice_leitura + 1])) {
                numero_real = 1;
                lexema[i++] = codigo_fonte[indice_leitura++]; 
                // Consome os números depois da vírgula (casas decimais)
                while (isdigit(codigo_fonte[indice_leitura]) && i < 99) {
                    lexema[i++] = codigo_fonte[indice_leitura++];
                }
            }
            lexema[i] = '\0';
            strcpy(lexema_atual, lexema);
            
            // Converte a string (char) para número de fato e salva no atributo apropriado usando funções nativas do C
            if (numero_real) {   
                token.type = TOKEN_NUM_FLOAT;
                token.attribute.float_value = atof(lexema); // atof = "Ascii to Float"
            } else {
                token.type = TOKEN_NUM_INT;
                token.attribute.int_value = atoi(lexema);   // atoi = "Ascii to Integer"
            }
            
            return token;
        }

        // ----------------------------------------------------------
        // RECONHECIMENTO DE STRINGS LITERAIS
        // ----------------------------------------------------------
        
        // Strings começam com aspas duplas, então lemos tudo até encontrar as próximas aspas
        if (c == '"') {
            int i = 0;
            char lexema[100];
            lexema[i++] = codigo_fonte[indice_leitura++]; // Salva a primeira aspas
            
            // Lê o texto interno (permite acentos, diferente das variáveis normais)
            while (codigo_fonte[indice_leitura] != '"' && codigo_fonte[indice_leitura] != '\0' && codigo_fonte[indice_leitura] != '\n') {
                if (i < 99) lexema[i++] = codigo_fonte[indice_leitura];
                indice_leitura++;
            }
            
            // Salva a aspas de fechamento se não tivermos atingido o limite
            if (codigo_fonte[indice_leitura] == '"') {
                if (i < 99) lexema[i++] = codigo_fonte[indice_leitura];
                indice_leitura++; 
            }
            
            lexema[i] = '\0';
            strcpy(lexema_atual, lexema);
            
            // Classificamos textos literais como ID para simplificar o tratamento genérico no parser
            token.type = TOKEN_ID; 
            return token;
        }

        // ----------------------------------------------------------
        // LOOKAHEAD DE 1 CARACTERE PARA OPERADORES DUPLOS
        // ----------------------------------------------------------
        
        // Verifica o caractere da frente (lookahead interno) para decidir se é um operador duplo (ex: >=, <-) ou simples (ex: >)
        // Isso evita que o compilador confunda o "<-" de atribuição com um "<" relacional e um "-" matemático.
        if (c == '>') {
            if (codigo_fonte[indice_leitura + 1] == '=') {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GE; // Greater or Equal (>=)
                strcpy(lexema_atual, ">=");
                indice_leitura += 2;
                return token;
            } else {
                token.type = TOKEN_OP_REL;
                token.attribute.op_code = OP_GT; // Greater Than (>)
                strcpy(lexema_atual, ">");
                indice_leitura++;
                return token;
            }
        } else if (c == '<') {
            // Verifica operador de atribuição (<-) específico do Visualg
            if (codigo_fonte[indice_leitura + 1] == '-') {
                token.type = TOKEN_KEYWORD; 
                strcpy(lexema_atual, "<-");
                indice_leitura += 2;
                return token;
            } 
            // Os blocos abaixo tratam <= e <> (menor ou igual e diferente, respectivamente)
            else if (codigo_fonte[indice_leitura + 1] == '=') {
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
        
        // Captura símbolos unitários como parênteses, colchetes, ponto e vírgula, e operações de soma/subtração
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == ':' || c == ',' || 
            c == '+' || c == '-' || c == '*' || c == '/' || c == '\\' || c == '.') {
            lexema_atual[0] = c;
            lexema_atual[1] = '\0';
            token.type = TOKEN_KEYWORD; // Todos esses são considerados keywords para facilitar
            indice_leitura++;
            return token;
        }
                   
        // Se chegou aqui e o caractere não entrou em nenhuma das lógicas acima, é um erro.
        // O caractere não faz parte da gramática aceita (ex: um '@' solto no código).
        printf("ERRO LÉXICO\n");
        printf("Linha: %d\n", linha_atual);
        printf("Sequência incorreta: %c\n", c);
        exit(1); // Encerra o programa imediatamente na presença de erro léxico
    }

    return token;
}

// ============================================================================
// ETAPA 3: ANALISADOR SINTÁTICO (PARSER)
// O parser foi construído no modelo Descendente Recursivo LL(1).
// Cada função representa uma regra de produção da gramática.
// ============================================================================

// Protótipos das funções, necessários pois as funções do parser chamam umas às outras recursivamente
void nextToken();
void consome(TokenNome tokenEsperado);
void consome_palavra(const char *palavraEsperada);
void consome_tipo();

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
 * Função responsável por chamar o Léxico sob demanda e atualizar o 'lookahead'.
 * Aqui também aproveitamos para imprimir as informações formatadas (e gerar o txt)
 * sempre que um token novo entra no fluxo, conforme exigido no pdf.
 */
void nextToken() {
    lookahead = obterToken(); 
    
    // Se não há mais texto, não precisa imprimir mais nada
    if (lookahead.type == TOKEN_EOF) return;

    // Variável estática (persiste o valor entre chamadas da função) para enumerar IDs sequencialmente
    static int id_counter = 0; 

    // Bateria de if/elses para imprimir no formato "<linha># <token> | <atributo>" exigido
    if (lookahead.type == TOKEN_KEYWORD) 
    {
        if (isalpha(lexema_atual[0])) 
        { // Se a keyword começar com letra, é uma palavra reservada
            printf("%d# PALAVRA_RESERVADA | 0\n", lookahead.line);
            if (arquivo_saida) fprintf(arquivo_saida, "%d# PALAVRA_RESERVADA | 0\n", lookahead.line);
        } 
        else 
        {
            if (strcmp(lexema_atual, "<-") == 0) 
            { // Símbolo especial de atribuição
                printf("%d# OPERADOR_ATRIBUICAO | 0\n", lookahead.line);
                if (arquivo_saida) fprintf(arquivo_saida, "%d# OPERADOR_ATRIBUICAO | 0\n", lookahead.line);
            } 
            else
            { // Outros símbolos unitários (+, - , *, (, )) são tratados como delimitadores/operadores matemáticos
                printf("%d# OPERADOR_DELIMITADOR | %d\n", lookahead.line, lexema_atual[0]);
                if (arquivo_saida) fprintf(arquivo_saida, "%d# OPERADOR_DELIMITADOR | %d\n", lookahead.line, lexema_atual[0]);
            }
        }
    } 
    else if (lookahead.type == TOKEN_ID) 
    {
        printf("%d# IDENTIFICADOR | %d\n", lookahead.line, id_counter);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# IDENTIFICADOR | %d\n", lookahead.line, id_counter);
        id_counter++; // Incrementa o contador de IDs toda vez que encontra um
    } 
    else if (lookahead.type == TOKEN_NUM_INT) {
        printf("%d# NUMERO_INTEIRO | %d\n", lookahead.line, lookahead.attribute.int_value);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# NUMERO_INTEIRO | %d\n", lookahead.line, lookahead.attribute.int_value);
    } 
    else if (lookahead.type == TOKEN_NUM_FLOAT) {
        printf("%d# NUMERO_REAL | %g\n", lookahead.line, lookahead.attribute.float_value); // %g omite os zeros inúteis do float
        if (arquivo_saida) fprintf(arquivo_saida, "%d# NUMERO_REAL | %g\n", lookahead.line, lookahead.attribute.float_value);
    } 
    else if (lookahead.type == TOKEN_OP_REL) {
        printf("%d# OPERADOR_RELACIONAL | %d\n", lookahead.line, lookahead.attribute.op_code);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# OPERADOR_RELACIONAL | %d\n", lookahead.line, lookahead.attribute.op_code);
    }
}

/*
 * Função "consumir" para checar Categorias Gerais (como um TOKEN_ID).
 * Verifica se o token atual (lookahead) pertence à categoria que a regra sintática exige.
 * Se sim, pede o próximo token (nextToken). Se não, lança erro de sintaxe.
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
 * Função "consumir" focada em Palavras Exatas e Símbolos (como "inicio", "se", ":").
 * Usada pois o enum TOKEN_KEYWORD é muito amplo, então precisamos validar usando a string salva no lexema_atual.
 */
void consome_palavra(const char *palavraEsperada) {
    // Compara o lexema atual com a string desejada (strcmp retorna 0 quando são iguais)
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, palavraEsperada) == 0) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Esperado '%s', mas encontrado '%s'\n", palavraEsperada, lexema_atual);
        exit(1);
    }
}

/*
 * Consome apenas tipos de dados válidos (inteiro, real, caractere, logico).
 * Qualquer outra palavra reservada, símbolo ou identificador gera erro sintático.
 */
void consome_tipo() {
    if (lookahead.type == TOKEN_KEYWORD && (
        strcmp(lexema_atual, "inteiro") == 0 ||
        strcmp(lexema_atual, "real") == 0 ||
        strcmp(lexema_atual, "caractere") == 0 ||
        strcmp(lexema_atual, "logico") == 0)) {
        nextToken();
    } else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Esperado um tipo de dado, mas encontrado '%s'\n", lexema_atual);
        exit(1);
    }
}

// ============================================================================
// ANÁLISE SINTÁTICA DE BLOCOS E ESTRUTURAS
// ============================================================================

/*
 * Função raiz do Analisador Sintático.
 * Verifica a macroestrutura obrigatória do arquivo: cabeçalho 'algoritmo',
 * seguido da declaração de dados, depois o 'inicio' dos comandos, e termina com 'fimalgoritmo'.
 */
void analisa_algoritmo_principal() {
    consome_palavra("algoritmo"); // Todo programa em Visualg obrigatoriamente inicia com isso
    consome(TOKEN_ID);            // O nome do algoritmo

    // Como as sub-rotinas e variáveis podem vir misturadas, entramos num laço que verifica quem é o próximo
    // Continua repetindo enquanto o lookahead for 'var', 'procedimento' ou 'funcao'
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "var") == 0 || strcmp(lexema_atual, "procedimento") == 0 || strcmp(lexema_atual, "funcao") == 0)) {
        
        if (strcmp(lexema_atual, "var") == 0) {
            processa_bloco_variaveis();
        } else {
            analisa_subrotinas();
        }
    }

    // Depois de declarar tudo, entraremos nos comandos lógicos
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

        // Fica em loop consumindo e validando variáveis (ex: 'nota1: inteiro') até o lookahead deixar de ser TOKEN_ID
        while (lookahead.type == TOKEN_ID) {
            
            // Loop interno (while 1) para permitir declaração em lote na mesma linha, como em 'a, b, c: inteiro'
            while (1) 
            {
                consome(TOKEN_ID); 
                
                // Se encontrar uma vírgula, consome ela e o loop continua esperando mais um TOKEN_ID
                if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0)
                {
                    consome_palavra(",");
                } 
                else 
                {
                    break; // Sai do laço infinito quando acabar a lista de variáveis separadas por vírgula
                }
            }
            consome_palavra(":");

            // Verifica se as variáveis declaradas são na verdade um vetor (array) (ex: vetor[1..10])
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "vetor") == 0) {
                consome_palavra("vetor");
                consome_palavra("[");
    
                // O índice inicial do vetor pode ser um número explícito ou uma constante (que é um TOKEN_ID)
                if (lookahead.type == TOKEN_NUM_INT)
                {
                    consome(TOKEN_NUM_INT);
                } 
                else 
                {
                    consome(TOKEN_ID);
                }

                // O Visualg exige duas sequências de pontos (ex: 1..10) para definir o alcance do vetor
                consome_palavra(".");
                consome_palavra(".");

                // O índice final também pode ser número ou constante
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

                // Valida se o tipo do vetor é um dos tipos suportados pela gramática (inteiro, real, etc)
                if (lookahead.type == TOKEN_KEYWORD && (
                    strcmp(lexema_atual, "inteiro") == 0 ||
                    strcmp(lexema_atual, "real") == 0 ||
                    strcmp(lexema_atual, "caractere") == 0 ||
                    strcmp(lexema_atual, "logico") == 0))
                {
                    consome_palavra(lexema_atual); // Consome dinamicamente a palavra correspondente
                } 
                else 
                {
                    printf("ERRO SINTÁTICO\n");
                    printf("Linha: %d\n", lookahead.line);
                    printf("Tipo de vetor inválido ou inesperado: '%s'\n", lexema_atual);
                    exit(1);
                }

            } 
            // Caso não seja um vetor, caímos no Else If e tratamos como um tipo de dado simples
            else if 
                (lookahead.type == TOKEN_KEYWORD && (
                strcmp(lexema_atual, "inteiro") == 0 ||
                strcmp(lexema_atual, "real") == 0 ||
                strcmp(lexema_atual, "caractere") == 0 ||
                strcmp(lexema_atual, "logico") == 0)) {
                
                consome_palavra(lexema_atual);

            } 
            else 
            {
                // Se após os dois pontos (:) não houver nem 'vetor' nem um tipo de dado válido, levanta erro
                printf("ERRO SINTÁTICO\n");
                printf("Linha: %d\n", lookahead.line);
                printf("Esperado um tipo de dado (inteiro, real, caractere, logico, vetor), mas encontrado '%s'\n", lexema_atual);
                exit(1);
            }
        }
    }
}

/*
 * Lê as instruções dentro dos blocos de 'inicio'.
 * Analisa o lookahead constantemente para saber qual função específica do parser chamar a seguir.
 */
void avalia_lista_comandos() {
    // Loop longo para abranger os possíveis inícios de instrução da gramática
    // Continua executando enquanto for um identificador (x <- 1) ou um comando-chave (se, enquanto, escreva)
    while (lookahead.type == TOKEN_ID || (lookahead.type == TOKEN_KEYWORD &&
            (strcmp(lexema_atual, "se") == 0 ||
              strcmp(lexema_atual, "enquanto") == 0 ||
              strcmp(lexema_atual, "para") == 0 ||
              strcmp(lexema_atual, "escreva") == 0 ||
              strcmp(lexema_atual, "escreval") == 0 ||
              strcmp(lexema_atual, "leia") == 0 ||
              strcmp(lexema_atual, "retorne") == 0
          ))) {
        
        // Verifica bloco condicional e chama a função apropriada
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "se") == 0) {
            avalia_comando_condicional();
        }
        // Verifica laços de repetição
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "enquanto") == 0 || strcmp(lexema_atual, "para") == 0)) {
            avalia_comando_repeticao();
        }
        // Verifica comando de retorno de função
        else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "retorne") == 0) {
            consome_palavra("retorne");
            avalia_expressao(); // Retornos carregam uma expressão matemática ou lógica consigo
        }
        // Tratamento dos Comandos de Entrada/Saída (I/O) nativos do Visualg
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "escreva") == 0 || strcmp(lexema_atual, "escreval") == 0 || strcmp(lexema_atual, "leia") == 0)) {
            
            char comando[100];            // Salva qual foi o comando chamado 
            strcpy(comando, lexema_atual); // pois o consome_palavra exige a palavra exata
            
            consome_palavra(comando); 
            consome_palavra("(");     
            
            avalia_expressao();       
            
            // Permite passagem de várias variáveis/expressões separadas por vírgula no escreva() ou leia()
            while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                consome_palavra(",");
                avalia_expressao();
            }
            consome_palavra(")");     
        }
        // Tratamento de Identificadores (Pode ser: atribuição de variável, acesso a índice de vetor ou chamada de função solta)
        else if (lookahead.type == TOKEN_ID) {
            consome(TOKEN_ID); 
            
            // Se após o ID vier um colchete, então é o acesso a um vetor (ex: notas[1])
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
                consome_palavra("[");
                avalia_expressao(); // A posição do vetor pode ser uma expressão complexa (ex: notas[x+1])
                consome_palavra("]");
            }
            // Se após o ID vier um parênteses, o usuário está chamando uma sub-rotina e passando os argumentos
            else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                // Se o próximo não for parênteses de fechamento, então de fato há parâmetros lá dentro
                if (lookahead.type != TOKEN_KEYWORD || strcmp(lexema_atual, ")") != 0) {
                    avalia_expressao();
                    while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                        consome_palavra(",");
                        avalia_expressao();
                    }
                }
                consome_palavra(")");
            }
            
            // Verifica o símbolo de atribuição para a variável ou vetor receber um valor
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "<-") == 0) {
                consome_palavra("<-");
                avalia_expressao();   
            }
        }
    }
}

/*
 * Análise Sintática do bloco 'se' e 'senao'.
 */
void avalia_comando_condicional() {
    consome_palavra("se");
    consome_palavra("(");
    avalia_expressao(); // Aqui será lida a lógica da condição do IF
    consome_palavra(")");
    consome_palavra("entao");

    // Chama avalia_lista_comandos() recursivamente, para processar as linhas dentro do IF
    avalia_lista_comandos();

    // Como o 'senao' é opcional (nem todo IF tem ELSE), nós testamos e só processamos caso ele exista
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
        avalia_lista_comandos(); // Executa tudo que tem dentro do while
        consome_palavra("fimenquanto");
    } 
    // Trata o segundo tipo de laço possível no Visualg
    else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "para") == 0) {
        consome_palavra("para");
        consome(TOKEN_ID);       // A variável de iteração do FOR (ex: 'i')
        consome_palavra("de");
        avalia_expressao();      // O valor de início (ex: '1')

        consome_palavra("ate");
        avalia_expressao();      // O valor final

        // No FOR do Visualg, o passo (incremento) é opcional (o padrão é +1). Verificamos se foi digitado.
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "passo") == 0) {
            consome_palavra("passo");
            avalia_expressao(); 
        }

        consome_palavra("faca");
        avalia_lista_comandos();
        consome_palavra("fimpara");
    }
    else {
        // Se a função de repetição foi chamada, mas não foi nem 'enquanto' nem 'para', há um problema de lógica
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Comando de repetição inválido. Encontrado: %s\n", lexema_atual);
        exit(1);
    }
}

/*
 * Análise Sintática da declaração de Funções e Procedimentos.
 * Lê e valida toda a assinatura do bloco, os parâmetros formais e o escopo interno.
 */
void analisa_subrotinas() {
    // Fica rodando para suportar a declaração de múltiplas sub-rotinas antes do algoritmo principal
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "procedimento") == 0 || strcmp(lexema_atual, "funcao") == 0)) {
        
        // Procedimentos (métodos void que não possuem um tipo de retorno)
        if (strcmp(lexema_atual, "procedimento") == 0) {
            consome_palavra("procedimento");
            consome(TOKEN_ID); // Consome o nome escolhido pelo usuário para o procedimento
            
            // Trata a entrada de parâmetros obrigatórios entre parênteses (ex: (a: inteiro, b: real))
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                do {
                    // Se não for o primeiro parâmetro, então haverá uma vírgula antes do identificador
                    if (strcmp(lexema_atual, ",") == 0) consome_palavra(",");
                    consome(TOKEN_ID);
                    consome_palavra(":");
                    consome_tipo(); // O tipo do dado esperado
                } while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0); // Repete o 'do' se encontrar outra vírgula
                consome_palavra(")");
            }
            
            consome_palavra("inicio");
            avalia_lista_comandos();
            consome_palavra("fimprocedimento"); 
            
        } 
        // Funções (semelhantes aos Procedimentos, porém ao final declaram um tipo de retorno obrigatório)
        else if (strcmp(lexema_atual, "funcao") == 0) {
            consome_palavra("funcao");
            consome(TOKEN_ID); 
            
            // A lógica de declaração de parâmetros é idêntica à do Procedimento
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                do {
                    if (strcmp(lexema_atual, ",") == 0) consome_palavra(",");
                    consome(TOKEN_ID);
                    consome_palavra(":");
                    consome_tipo(); 
                } while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0);
                consome_palavra(")");
            }
            
            // Ao contrário dos Procedimentos, aqui obrigatoriamente declaramos o tipo que a Função vai retornar
            consome_palavra(":");
            consome_tipo(); 
            
            consome_palavra("inicio");
            avalia_lista_comandos();
            consome_palavra("fimfuncao"); 
        }
    }
}

// ============================================================================
// AVALIAÇÃO DE EXPRESSÕES E PRECEDÊNCIA MATEMÁTICA
// O coração da lógica LL(1) e onde a recursão acontece pesadamente.
// A gramática exige avaliar as expressões seguindo a hierarquia correta:
// 1º Parênteses (resolvidos primeiro por recursão em fator)
// 2º Multiplicação/Divisão (Termo)
// 3º Soma/Subtração (Expressão Simples)
// 4º Operadores Relacionais (Expressão Base)
// ============================================================================

/*
 * Base da expressão (Fator). É o pilar das resoluções matemáticas.
 * Consome a unidade atômica mais simples (um número, uma variável) ou resolve chamadas fechadas com parênteses.
 */
void avalia_fator() {
    // Trata sinais negativos ou positivos unários (ex: 'x <- -1') 
    // Em vez de subtrair dois números, apenas muda o sinal do número a seguir
    if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "-") == 0 || strcmp(lexema_atual, "+") == 0)) {
        char op[10];
        strcpy(op, lexema_atual); // Guarda o sinal +/-
        consome_palavra(op);
        avalia_fator(); // Chama a si mesma novamente para consumir o número alvo desse sinal
        return;
    }

    // Se a base da expressão for uma variável (ID) ou uma função
    if (lookahead.type == TOKEN_ID) {
        consome(TOKEN_ID);
        
        // Trata caso seja o acesso à posição de um vetor dentro de um cálculo matemático
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
            consome_palavra("[");
            avalia_expressao();
            consome_palavra("]");
        } 
        // Trata chamada de função no meio do cálculo e lê os parâmetros passados para ela
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
    } 
    // Outros casos atômicos que formam um Fator:
    else if (lookahead.type == TOKEN_NUM_INT) {
        consome(TOKEN_NUM_INT); // Um número inteiro solto
    } else if (lookahead.type == TOKEN_NUM_FLOAT) {
        consome(TOKEN_NUM_FLOAT); // Um número decimal solto
    } else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "verdadeiro") == 0 || strcmp(lexema_atual, "falso") == 0)) {
        // Valores lógicos booleanos puros
        if (strcmp(lexema_atual, "verdadeiro") == 0) consome_palavra("verdadeiro");
        else consome_palavra("falso");
    } else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
        // Encontrou um parêntese abrindo! O LL(1) prioriza isso: 
        // Chama a raiz avalia_expressao() para resolver toda a cadeia interna isoladamente primeiro
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
 * Funciona como intermediário entre a soma/subtração e o fator base.
 * Garante que a multiplicação ocorra antes da soma.
 */
void avalia_termo() {
    avalia_fator(); // Primeiramente avalia o termo da esquerda
    
    // O laço agrupa e resolve as operações multiplicativas sucessivas (ex: 5 * 2 / 2)
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "*") == 0 || strcmp(lexema_atual, "/") == 0 || 
           strcmp(lexema_atual, "\\") == 0 || strcmp(lexema_atual, "MOD") == 0 || 
           strcmp(lexema_atual, "E") == 0)) {
        
        char operador[10];
        strcpy(operador, lexema_atual);
        consome_palavra(operador); // Consome o operador (*)
        avalia_fator(); // Processa e encadeia o número ou expressão à direita da multiplicação
    }
}

/*
 * Resolve somas e subtrações (Expressão Simples).
 */
void avalia_expressao_simples() {
    avalia_termo(); // Primeiramente chama o termo, que resolve a multiplicação, e retorna para ser somado
    
    // Semelhante à multiplicação, o laço resolve as operações aditivas sucessivas (ex: 5 + 2 - 2)
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "+") == 0 || strcmp(lexema_atual, "-") == 0 || 
           strcmp(lexema_atual, "OU") == 0)) {
        
        char operador[10];
        strcpy(operador, lexema_atual);
        consome_palavra(operador); // Consome o operador de adição (+)
        avalia_termo(); // Encapsula os termos em cascata
    }
}

/*
 * Porta de entrada da validação matemática e lógica.
 * Inicia a cascata (chama expressao simples, que chama termo, que chama fator)
 * Verifica a presença de operadores relacionais (como >, <, =) ligando as duas sub-expressões resultantes.
 */
void avalia_expressao() {
    avalia_expressao_simples(); 
    
    // Testa o token no meio, se for relacional é uma validação booleana (ex: expressao_esq > expressao_dir)
    if (lookahead.type == TOKEN_OP_REL) {
        consome(TOKEN_OP_REL); 
        avalia_expressao_simples();
    }
}

// ============================================================================
// FUNÇÃO PRINCIPAL (MAIN)
// O sistema inicia aqui. Recebemos o arquivo-fonte como argumento (argv).
// ============================================================================

int main(int argc, char *argv[]) {
    // Validação inicial para evitar que o compilador crashe sem o nome do arquivo-fonte
    // O argc conta quantos parâmetros foram escritos no console; o mínimo é 2 (compilador.exe e arquivo.txt)
    if (argc < 2) {
        printf("Erro: Ficheiro fonte nao informado.\n");
        printf("Uso: %s <nome_do_arquivo.txt>\n", argv[0]);
        return 1;
    }

    // Abre o código-fonte que o usuário apontou, operando apenas em modo "r" (Read/Leitura)
    FILE *arquivo_fonte = fopen(argv[1], "r");
    if (arquivo_fonte == NULL) {
        printf("Erro: Nao foi possivel abrir o ficheiro %s\n", argv[1]);
        return 1;
    }

    // ------------------------------------------------------------------------
    // PREPARAÇÃO DE MEMÓRIA DINÂMICA
    // Calcula a quantidade exata de caracteres do arquivo para alocar (malloc) 
    // a memória estritamente necessária, evitando buffer overflow/overread.
    // ------------------------------------------------------------------------
    
    fseek(arquivo_fonte, 0, SEEK_END);          // Joga o cursor invisível de leitura até o fim do txt
    long tamanho_ficheiro = ftell(arquivo_fonte); // Pergunta "qual a distância daqui pro começo?", pegando o tamanho do txt
    rewind(arquivo_fonte);                      // Devolve o cursor lá pro começo para lermos o conteúdo depois

    // Aloca (tamanho + 1) para caber o '\0' que encerra strings no C
    codigo_fonte = (char *)malloc((tamanho_ficheiro + 1) * sizeof(char));
    if (codigo_fonte == NULL) {
        printf("Erro: Falha na alocacao de memoria para o codigo_fonte.\n");
        fclose(arquivo_fonte);
        return 1;
    }

    // Passa todo o texto para a string global em uma só tacada e fecha o arquivo original por segurança
    size_t lidos = fread(codigo_fonte, sizeof(char), tamanho_ficheiro, arquivo_fonte);
    codigo_fonte[lidos] = '\0'; // Carimba o final com segurança
    fclose(arquivo_fonte);

    // Abre (ou cria) um arquivo de saída limpo para gravar a tabela de tokens processada pelo Léxico
    arquivo_saida = fopen("saida_tokens.txt", "w");

    // Dá o pontapé inicial no sistema LL(1): carrega o primeiro token preenchendo o lookahead
    // e na sequência chama o Analisador Sintático para validar esse lookahead
    nextToken(); 
    analisa_algoritmo_principal();
    
    // Validação especial de EOF Lock:
    // Uma vez finalizada a análise de regras gramaticais, obrigatoriamente a leitura deve parar no EOF.
    // Caso a gramática seja bem sucedida, mas acuse que ainda tem token sobrando/sujeira fora de escopo, levanta erro.
    if (lookahead.type != TOKEN_EOF) {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Encontrado lixo após o 'fimalgoritmo': %s\n", lexema_atual);
        exit(1);
    }
    
    // Se o código passar vivo por todas as funções do analisador sintático sem acionar a função consome() com erro
    // isso indica sucesso total na derivação da árvore e o código está estruturalmente validado.
    printf("Compilação concluída com sucesso. Código sintaticamente correto!\n");

    // Limpa a memória alocada e solta o txt de saída antes de encerrar
    free(codigo_fonte);
    if (arquivo_saida != NULL) fclose(arquivo_saida);

    return 0; // Finaliza o sistema retornando código 0 de encerramento saudável
}
