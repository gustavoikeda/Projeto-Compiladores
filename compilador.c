/*
 * ============================================================================
 * COMPILADOR MINIVISUALG
 * Fase 1: Análise Léxica e Sintática
 * Fase 2: Análise Semântica e Geração de Código Intermediário (MEPA)
 * ============================================================================
 *
 * Integrantes:
 *   Guilherme Limeira de Souza   RA: 10439777
 *   Gustavo Kiyoshi Ikeda        RA: 10439179
 *   Pedro Montarroyos de Pinho   RA: 10440213
 *
 * Uso de Inteligência Artificial (exigência do enunciado):
 *   A Fase 2 foi desenvolvida com auxílio do Claude (Anthropic): o esqueleto
 *   (estruturas e assinaturas das funções), a implementação da tabela de símbolos,
 *   das verificações semânticas [R1]-[R9] e da geração de código MEPA, e os
 *   comentários explicativos. O grupo revisou, testou e estudou o código gerado.
 *   A mesma declaração está no Readme.txt.
 *
 * Regras semânticas do enunciado (referenciadas nos comentários como [R1] ... [R9]):
 *   [R1] toda variável deve ser declarada antes de ser usada
 *   [R2] palavra reservada não pode ser usada como nome de variável
 *   [R3] identificador único e associado ao mesmo tipo (no mesmo escopo)
 *   [R4] variável declarada e com valor atribuído deve ser usada
 *   [R5] variáveis de uma mesma expressão devem ter tipos compatíveis
 *   [R6] tipo e número dos argumentos de uma chamada devem ser compatíveis
 *   [R7] procedimento não pode ser usado em expressões
 *   [R8] função não pode ser usada como procedimento (só em expressões/atribuições)
 *   [R9] valor de retorno compatível com o tipo declarado da função
 *
 * Tabela de símbolos (diretiva do professor):
 *   uma tabela por escopo, organizadas em PILHA (global na base, cada sub-rotina
 *   empilha a sua). Dentro de cada tabela ficam TODAS as categorias juntas
 *   (variáveis, parâmetros, procedimentos e funções).
 *
 * Marcações usadas nos comentários:
 *   [Rn]   -> trecho que implementa a regra semântica n
 *   [MEPA] -> trecho que gera o código intermediário
 *
 * Saídas: saida_tokens.txt (tokens, como na Fase 1) e saida_mepa.txt (código MEPA).
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
    TOKEN_KEYWORD,      // Representa palavras reservadas (inicio, se, var) e símbolos simples (+, -, *)
    TOKEN_STRING        // FASE 2: texto entre aspas ("Olá"). Antes era TOKEN_ID; foi separado para que
                        // a análise semântica não procure textos literais na tabela de símbolos.
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
// FASE 2: ESTRUTURAS DA ANÁLISE SEMÂNTICA
// ============================================================================

#define TAM_ID      16   // tamanho do campo ID do TNo (15 letras + '\0'), definido no enunciado
#define TAM_TIPO    7    // tamanho do campo tipo do TNo (6 letras + '\0'), definido no enunciado
#define MAX_PARAMS  20   // quantidade máxima de parâmetros por sub-rotina
#define MAX_NOMES_DECLARACAO 50  // nomes numa mesma declaração (ex.: a, b, c: inteiro)

/*
 * Tipos de dados da linguagem, usados internamente na verificação de tipos:
 * as funções de expressão devolvem um TipoDado.
 * No nó da tabela (TNo) o tipo fica guardado como texto, conforme o enunciado.
 */
typedef enum {
    TIPO_INDEFINIDO = 0,  // ainda não calculado (ou erro)
    TIPO_INTEIRO,
    TIPO_REAL,
    TIPO_CARACTERE,
    TIPO_LOGICO
} TipoDado;

/* Categoria do símbolo: todas as categorias ficam na mesma tabela do escopo */
typedef enum {
    CAT_VARIAVEL,
    CAT_PARAMETRO,
    CAT_PROCEDIMENTO,
    CAT_FUNCAO
} Categoria;

/*
 * Nó da tabela de símbolos (lista encadeada).
 * Os 4 primeiros campos são exatamente os exigidos pelo enunciado.
 * Os demais foram acrescentados porque a tabela de cada escopo guarda todas
 * as categorias juntas (variáveis, parâmetros, procedimentos e funções), e as
 * regras [R4], [R6], [R7] e [R8] precisam saber o que cada nome é.
 */
typedef struct _TNo {
    char ID[TAM_ID];                    // nome do identificador
    int endereco;                       // ordem de declaração dentro do escopo: 0, 1, 2...
    char tipo[TAM_TIPO];                // tipo da variável/parâmetro ou tipo de retorno da função
    struct _TNo *prox;                  // próximo nó da lista

    // --- campos acrescentados na fase 2 ---
    Categoria categoria;                // variável, parâmetro, procedimento ou função
    int eh_vetor;                       // 1 se a variável foi declarada como vetor
    int num_params;                     // sub-rotinas: quantidade de parâmetros
    TipoDado tipos_params[MAX_PARAMS];  // sub-rotinas: tipo de cada parâmetro, na ordem
    int atribuida;                      // 1 se a variável recebeu valor em algum ponto  [R4]
    int usada;                          // 1 se o valor da variável foi lido em algum ponto [R4]
    int linha_declaracao;               // linha da declaração (mensagem de erro da [R4])
    int rotulo_entrada;                 // sub-rotinas: rótulo da entrada (ENPR) usado no CHPR [MEPA]
} TNo;

/*
 * Uma tabela de símbolos (lista encadeada de TNo) por escopo.
 * As tabelas formam uma pilha: o escopo global fica na base e cada
 * sub-rotina empilha a sua ao começar e desempilha ao terminar.
 */
typedef struct _TEscopo {
    TNo *inicio;                  // primeiro nó da lista deste escopo
    int proximo_endereco;         // endereço que o próximo símbolo declarado vai receber
    struct _TEscopo *anterior;    // escopo de baixo na pilha (NULL no escopo global)
} TEscopo;

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

// --- Fase 2 ---
TEscopo *topo_escopo = NULL;  // topo da pilha de tabelas de símbolos
TNo *rotina_atual = NULL;     // sub-rotina sendo analisada (NULL no programa principal); usada no 'retorne' [R9]
FILE *arquivo_mepa = NULL;    // arquivo .txt com o código MEPA gerado
int contador_rotulos = 0;     // último rótulo entregue por proximo_rotulo()

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
            
            // FASE 2: textos literais têm categoria própria (na Fase 1 eram classificados como TOKEN_ID).
            // O parser os aceita no nome do algoritmo e como fator de expressão (ex.: escreval("Olá")).
            token.type = TOKEN_STRING; 
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
// FASE 2 - ETAPA 4: ERROS SEMÂNTICOS E CONVERSÃO DE TIPOS
// ============================================================================

/*
 * Imprime um erro semântico e encerra a compilação.
 * Formato igual ao dos outros erros:
 *   ERRO SEMÂNTICO
 *   Linha: N
 *   <mensagem>
 * Quem chama monta a mensagem (ex.: com snprintf "Variável 'x' não declarada").
 */
void erro_semantico(int linha, const char *mensagem) {
    // Mesmo formato do ERRO LÉXICO e do ERRO SINTÁTICO
    printf("ERRO SEMÂNTICO\n");
    printf("Linha: %d\n", linha);
    printf("%s\n", mensagem);

    // Fecha os arquivos que estiverem abertos, para não deixar saída pela metade sem gravar
    if (arquivo_saida) { fclose(arquivo_saida); arquivo_saida = NULL; }
    if (arquivo_mepa)  { fclose(arquivo_mepa);  arquivo_mepa  = NULL; }

    // Mesmo código de saída dos erros léxico e sintático (exit(1)).
    // DÚVIDA PARA O PROFESSOR: se o programa deve terminar com 0 também em caso de erro, trocar aqui.
    exit(1);
}

/*
 * Converte a palavra reservada de tipo ("inteiro", "real", "caractere", "logico")
 * no TipoDado correspondente. Usada nas declarações.
 */
TipoDado lexema_para_tipo(const char *lexema) {
    // O léxico diferencia maiúsculas de minúsculas, então a comparação é exata (strcmp)
    if (strcmp(lexema, "inteiro") == 0)   return TIPO_INTEIRO;
    if (strcmp(lexema, "real") == 0)      return TIPO_REAL;
    if (strcmp(lexema, "caractere") == 0) return TIPO_CARACTERE;
    if (strcmp(lexema, "logico") == 0)    return TIPO_LOGICO;
    return TIPO_INDEFINIDO; // não é palavra de tipo
}

/*
 * Converte um TipoDado no texto guardado no campo 'tipo' do TNo.
 * ATENÇÃO: o campo tem 7 posições (6 letras + '\0'), então "inteiro" (7 letras)
 * e "caractere" (9 letras) não cabem. Por isso usamos abreviações:
 *   inteiro -> "int"   real -> "real"   caractere -> "char"   logico -> "logico"
 * DÚVIDA PARA O PROFESSOR: confirmar se essas abreviações são aceitas.
 */
const char *tipo_para_texto(TipoDado tipo) {
    switch (tipo) {
        case TIPO_INTEIRO:   return "int";
        case TIPO_REAL:      return "real";
        case TIPO_CARACTERE: return "char";
        case TIPO_LOGICO:    return "logico";
        default:             return ""; // TIPO_INDEFINIDO (ex.: procedimento, que não tem tipo)
    }
}

/*
 * Converte o texto do campo 'tipo' do TNo de volta para TipoDado.
 * Usada quando um símbolo é encontrado na tabela e precisamos do seu tipo.
 */
TipoDado texto_para_tipo(const char *texto) {
    // Inverso de tipo_para_texto(): usa as mesmas abreviações
    if (strcmp(texto, "int") == 0)    return TIPO_INTEIRO;
    if (strcmp(texto, "real") == 0)   return TIPO_REAL;
    if (strcmp(texto, "char") == 0)   return TIPO_CARACTERE;
    if (strcmp(texto, "logico") == 0) return TIPO_LOGICO;
    return TIPO_INDEFINIDO; // texto vazio ou desconhecido
}

// ============================================================================
// FASE 2 - ETAPA 5: TABELA DE SÍMBOLOS (PILHA DE TABELAS POR ESCOPO)
// ============================================================================

void verifica_variaveis_nao_usadas(TEscopo *escopo); // definida na Etapa 6

/*
 * Empilha uma tabela vazia: início do programa (escopo global)
 * e início de cada procedimento/função.
 */
void abre_escopo(void) {
    TEscopo *novo = (TEscopo *)malloc(sizeof(TEscopo));
    if (novo == NULL) {
        printf("Erro: Falha na alocacao de memoria para a tabela de simbolos.\n");
        exit(1);
    }
    novo->inicio = NULL;            // tabela começa vazia
    novo->proximo_endereco = 0;     // a primeira variável deste escopo terá endereço 0
    novo->anterior = topo_escopo;   // o escopo que era o topo fica "embaixo" deste
    topo_escopo = novo;             // push: a nova tabela vira o topo da pilha
}

/*
 * Desempilha a tabela do topo: fim de cada sub-rotina e fim do programa.
 * Antes de liberar, verifica a regra [R4] para os símbolos deste escopo.
 */
void fecha_escopo(void) {
    if (topo_escopo == NULL) return;

    verifica_variaveis_nao_usadas(topo_escopo); // [R4]

    // Libera cada nó da lista encadeada deste escopo
    TNo *atual = topo_escopo->inicio;
    while (atual != NULL) {
        TNo *proximo = atual->prox;
        free(atual);
        atual = proximo;
    }

    // pop: o escopo de baixo volta a ser o topo
    TEscopo *removido = topo_escopo;
    topo_escopo = topo_escopo->anterior;
    free(removido);
}

/*
 * Procura um identificador SOMENTE na tabela do topo.
 * Usada na declaração, para detectar redeclaração no mesmo escopo [R3].
 * Devolve o nó ou NULL.
 */
TNo *busca_no_escopo_atual(const char *id) {
    if (topo_escopo == NULL) return NULL;
    for (TNo *no = topo_escopo->inicio; no != NULL; no = no->prox) {
        if (strcmp(no->ID, id) == 0) return no;
    }
    return NULL;
}

/*
 * Resolução de nomes: procura do topo da pilha até o escopo global.
 * Devolve o nó encontrado ou NULL (não gera erro).
 * Como a busca começa no topo, um nome local "esconde" um global de mesmo nome.
 */
TNo *busca_simbolo(const char *id) {
    for (TEscopo *escopo = topo_escopo; escopo != NULL; escopo = escopo->anterior) {
        for (TNo *no = escopo->inicio; no != NULL; no = no->prox) {
            if (strcmp(no->ID, id) == 0) return no;
        }
    }
    return NULL;
}

/*
 * Função exigida pelo enunciado.
 * Recebe o lexema do token corrente (o ID) e devolve o endereço da variável.
 * Se o identificador não estiver em nenhuma tabela, gera erro semântico [R1].
 * Deve ser chamada ANTES de consumir o ID, enquanto lexema_atual e
 * lookahead.line ainda se referem a ele.
 * (Procedimentos e funções não ocupam endereço: para eles devolve -1.)
 */
int busca_tabela_simbolos(const char *atributo_ID) {
    TNo *no = busca_simbolo(atributo_ID);
    if (no == NULL) {
        char mensagem[200];
        snprintf(mensagem, sizeof(mensagem), "Identificador '%s' não declarado", atributo_ID);
        erro_semantico(lookahead.line, mensagem);
        return -1; // não chega aqui: erro_semantico encerra o programa
    }
    return no->endereco;
}

/*
 * Insere um símbolo na tabela do topo.
 * - [R3] se já existir no escopo atual -> erro semântico de redeclaração
 * - nomes com mais de 15 letras não cabem em ID[16] -> erro semântico
 *   (truncar faria dois nomes longos diferentes virarem o mesmo nome)
 * - variáveis e parâmetros recebem endereço = ordem de declaração no escopo;
 *   procedimentos e funções não ocupam memória de dados (endereço -1)
 * - o nó é encadeado no FIM da lista, para a lista ficar na ordem de declaração
 * Devolve o nó criado (usado depois para adicionar parâmetros às rotinas).
 */
TNo *insere_simbolo(const char *id, Categoria categoria, TipoDado tipo, int linha) {
    char mensagem[200];

    if (strlen(id) >= TAM_ID) {
        snprintf(mensagem, sizeof(mensagem),
                 "Identificador '%s' tem mais de %d caracteres", id, TAM_ID - 1);
        erro_semantico(linha, mensagem);
    }

    if (busca_no_escopo_atual(id) != NULL) { // [R3]
        snprintf(mensagem, sizeof(mensagem), "Identificador '%s' já declarado neste escopo", id);
        erro_semantico(linha, mensagem);
    }

    TNo *novo = (TNo *)malloc(sizeof(TNo));
    if (novo == NULL) {
        printf("Erro: Falha na alocacao de memoria para a tabela de simbolos.\n");
        exit(1);
    }

    // Campos exigidos pelo enunciado
    strcpy(novo->ID, id);
    strcpy(novo->tipo, tipo_para_texto(tipo));
    if (categoria == CAT_VARIAVEL || categoria == CAT_PARAMETRO) {
        novo->endereco = topo_escopo->proximo_endereco++;
    } else {
        novo->endereco = -1;
    }
    novo->prox = NULL;

    // Campos acrescentados na fase 2
    novo->categoria = categoria;
    novo->eh_vetor = 0;
    novo->num_params = 0;
    novo->atribuida = (categoria == CAT_PARAMETRO); // parâmetro já chega com o valor passado na chamada
    novo->usada = 0;
    novo->linha_declaracao = linha;
    novo->rotulo_entrada = 0;

    // Encadeia no fim da lista do topo
    if (topo_escopo->inicio == NULL) {
        topo_escopo->inicio = novo;
    } else {
        TNo *ultimo = topo_escopo->inicio;
        while (ultimo->prox != NULL) ultimo = ultimo->prox;
        ultimo->prox = novo;
    }
    return novo;
}

/*
 * Registra o tipo de mais um parâmetro na rotina (procedimento/função).
 * Chamada na declaração dos parâmetros; usada depois na checagem [R6].
 */
void adiciona_parametro(TNo *rotina, TipoDado tipo, int linha) {
    if (rotina->num_params >= MAX_PARAMS) {
        char mensagem[200];
        snprintf(mensagem, sizeof(mensagem),
                 "A rotina '%s' excede o limite de %d parâmetros", rotina->ID, MAX_PARAMS);
        erro_semantico(linha, mensagem);
    }
    rotina->tipos_params[rotina->num_params++] = tipo;
}

// ============================================================================
// FASE 2 - ETAPA 6: VERIFICAÇÕES SEMÂNTICAS
// ============================================================================

/*
 * [R2] Verifica se um lexema é palavra reservada.
 * A lista é a mesma usada em obterToken() (que ficou como na Fase 1).
 */
int eh_palavra_reservada(const char *lexema) {
    static const char *reservadas[] = {
        "algoritmo", "var", "inicio", "fimalgoritmo", "se", "entao", "senao", "fimse",
        "escreva", "escreval", "leia", "enquanto", "faca", "fimenquanto",
        "para", "de", "ate", "passo", "fimpara",
        "inteiro", "real", "caractere", "logico", "vetor",
        "procedimento", "fimprocedimento", "funcao", "retorne", "fimfuncao",
        "verdadeiro", "falso", "MOD", "E", "OU"
    };
    int quantidade = sizeof(reservadas) / sizeof(reservadas[0]);
    for (int i = 0; i < quantidade; i++) {
        if (strcmp(lexema, reservadas[i]) == 0) return 1;
    }
    return 0;
}

/*
 * [R2] Chamada nos pontos em que a gramática espera o NOME de algo novo
 * (variável, parâmetro, procedimento ou função).
 * Se no lugar do nome veio uma palavra reservada, o erro é SEMÂNTICO
 * (sem esta checagem ele apareceria como sintático).
 * 'aceita_fim_do_bloco' = 1 no começo de cada linha do bloco 'var', onde
 * 'inicio', 'var', 'procedimento' e 'funcao' encerram o bloco normalmente.
 */
void verifica_nome_nao_reservado(int aceita_fim_do_bloco) {
    if (lookahead.type != TOKEN_KEYWORD || !eh_palavra_reservada(lexema_atual)) return;

    if (aceita_fim_do_bloco &&
        (strcmp(lexema_atual, "inicio") == 0 || strcmp(lexema_atual, "var") == 0 ||
         strcmp(lexema_atual, "procedimento") == 0 || strcmp(lexema_atual, "funcao") == 0)) {
        return;
    }

    char mensagem[200];
    snprintf(mensagem, sizeof(mensagem),
             "Palavra reservada '%s' não pode ser usada como nome", lexema_atual);
    erro_semantico(lookahead.line, mensagem);
}

/* Nome completo do tipo, usado nas mensagens de erro (o TNo guarda a abreviação). */
const char *nome_do_tipo(TipoDado tipo) {
    switch (tipo) {
        case TIPO_INTEIRO:   return "inteiro";
        case TIPO_REAL:      return "real";
        case TIPO_CARACTERE: return "caractere";
        case TIPO_LOGICO:    return "logico";
        default:             return "indefinido";
    }
}

/* 1 se o tipo é numérico (inteiro ou real) */
int eh_numerico(TipoDado tipo) {
    return tipo == TIPO_INTEIRO || tipo == TIPO_REAL;
}

/*
 * Um valor do tipo 'origem' pode ser guardado num lugar do tipo 'destino'?
 * Tipos iguais sempre podem. A única conversão aceita é inteiro -> real
 * (o inteiro cabe no real sem perder nada; o contrário perderia a parte decimal).
 */
int tipos_compativeis(TipoDado destino, TipoDado origem) {
    return destino == origem || (destino == TIPO_REAL && origem == TIPO_INTEIRO);
}

/*
 * [R5] Verifica se um operador pode ser aplicado aos dois tipos e devolve
 * o tipo do resultado:
 *   + - *            numérico com numérico -> inteiro se os dois forem inteiros, senão real
 *   /                numérico com numérico -> real
 *   \ e MOD          inteiro com inteiro   -> inteiro
 *   E, OU            logico com logico     -> logico
 *   < <= > >=        numérico com numérico -> logico
 *   = <>             numérico com numérico, ou dois valores do mesmo tipo -> logico
 * Se forem incompatíveis -> erro semântico.
 */
TipoDado verifica_operacao(TipoDado esquerda, const char *operador, TipoDado direita, int linha) {
    TipoDado resultado = TIPO_INDEFINIDO;
    int ambos_numericos = eh_numerico(esquerda) && eh_numerico(direita);

    if (strcmp(operador, "+") == 0 || strcmp(operador, "-") == 0 || strcmp(operador, "*") == 0) {
        if (ambos_numericos) {
            resultado = (esquerda == TIPO_INTEIRO && direita == TIPO_INTEIRO) ? TIPO_INTEIRO : TIPO_REAL;
        }
    } else if (strcmp(operador, "/") == 0) {
        if (ambos_numericos) resultado = TIPO_REAL;
    } else if (strcmp(operador, "\\") == 0 || strcmp(operador, "MOD") == 0) {
        if (esquerda == TIPO_INTEIRO && direita == TIPO_INTEIRO) resultado = TIPO_INTEIRO;
    } else if (strcmp(operador, "E") == 0 || strcmp(operador, "OU") == 0) {
        if (esquerda == TIPO_LOGICO && direita == TIPO_LOGICO) resultado = TIPO_LOGICO;
    } else if (strcmp(operador, "=") == 0 || strcmp(operador, "<>") == 0) {
        if (ambos_numericos || esquerda == direita) resultado = TIPO_LOGICO;
    } else { // < <= > >=
        if (ambos_numericos) resultado = TIPO_LOGICO;
    }

    if (resultado == TIPO_INDEFINIDO) {
        char mensagem[200];
        snprintf(mensagem, sizeof(mensagem), "Tipos incompatíveis: %s %s %s",
                 nome_do_tipo(esquerda), operador, nome_do_tipo(direita));
        erro_semantico(linha, mensagem);
    }
    return resultado;
}

/*
 * [R5] Verifica se o valor de uma expressão pode ser atribuído à variável.
 * Decisão do grupo: real <- inteiro é permitido; inteiro <- real não.
 */
void verifica_atribuicao(TipoDado destino, TipoDado origem, const char *nome, int linha) {
    if (!tipos_compativeis(destino, origem)) {
        char mensagem[200];
        snprintf(mensagem, sizeof(mensagem),
                 "Atribuição incompatível: a variável '%s' é %s e o valor é %s",
                 nome, nome_do_tipo(destino), nome_do_tipo(origem));
        erro_semantico(linha, mensagem);
    }
}

/*
 * Verifica se a condição de 'se' / 'enquanto' é do tipo lógico.
 */
void verifica_condicao(TipoDado tipo, const char *comando, int linha) {
    if (tipo != TIPO_LOGICO) {
        char mensagem[200];
        snprintf(mensagem, sizeof(mensagem),
                 "A condição do '%s' deve ser logico, mas é %s", comando, nome_do_tipo(tipo));
        erro_semantico(linha, mensagem);
    }
}

/*
 * [R5] Usada no 'para' e no índice de vetor: o valor precisa ser inteiro.
 */
void verifica_inteiro(TipoDado tipo, const char *o_que, int linha) {
    if (tipo != TIPO_INTEIRO) {
        char mensagem[200];
        snprintf(mensagem, sizeof(mensagem), "%s deve ser inteiro, mas é %s", o_que, nome_do_tipo(tipo));
        erro_semantico(linha, mensagem);
    }
}

/*
 * [R6] Compara os argumentos de uma chamada com os parâmetros da rotina:
 * mesma quantidade e tipos compatíveis, na mesma ordem.
 */
void verifica_argumentos(TNo *rotina, TipoDado tipos_args[], int num_args, int linha) {
    char mensagem[200];

    if (num_args != rotina->num_params) {
        snprintf(mensagem, sizeof(mensagem), "'%s' recebeu %d argumento(s); esperado(s) %d",
                 rotina->ID, num_args, rotina->num_params);
        erro_semantico(linha, mensagem);
    }
    for (int i = 0; i < num_args; i++) {
        if (!tipos_compativeis(rotina->tipos_params[i], tipos_args[i])) {
            snprintf(mensagem, sizeof(mensagem), "Argumento %d de '%s': esperado %s, recebido %s",
                     i + 1, rotina->ID, nome_do_tipo(rotina->tipos_params[i]), nome_do_tipo(tipos_args[i]));
            erro_semantico(linha, mensagem);
        }
    }
}

/*
 * [R9] Verifica o 'retorne': precisa estar dentro de uma função e o tipo
 * da expressão precisa ser compatível com o tipo declarado da função.
 */
void verifica_retorno(TipoDado tipo_retornado, int linha) {
    char mensagem[200];

    if (rotina_atual == NULL || rotina_atual->categoria != CAT_FUNCAO) {
        erro_semantico(linha, "'retorne' fora de uma função");
        return; // não chega aqui: erro_semantico encerra o programa
    }
    TipoDado tipo_funcao = texto_para_tipo(rotina_atual->tipo);
    if (!tipos_compativeis(tipo_funcao, tipo_retornado)) {
        snprintf(mensagem, sizeof(mensagem),
                 "Retorno incompatível: a função '%s' é %s e o valor é %s",
                 rotina_atual->ID, nome_do_tipo(tipo_funcao), nome_do_tipo(tipo_retornado));
        erro_semantico(linha, mensagem);
    }
}

/*
 * [R4] Percorre a tabela de um escopo procurando variáveis que receberam
 * valor (atribuida == 1) mas nunca foram usadas (usada == 0).
 * Chamada por fecha_escopo() antes de liberar a tabela.
 * Decisão do grupo: variável declarada e nunca atribuída NÃO é erro
 * (o enunciado só fala de variável "com valor atribuído").
 */
void verifica_variaveis_nao_usadas(TEscopo *escopo) {
    for (TNo *no = escopo->inicio; no != NULL; no = no->prox) {
        if ((no->categoria == CAT_VARIAVEL || no->categoria == CAT_PARAMETRO) &&
            no->atribuida && !no->usada) {
            char mensagem[200];
            snprintf(mensagem, sizeof(mensagem), "%s '%s' recebeu valor mas nunca foi usado(a)",
                     no->categoria == CAT_PARAMETRO ? "Parâmetro" : "Variável", no->ID);
            erro_semantico(no->linha_declaracao, mensagem);
        }
    }
}

/*
 * Procura um nome usado no corpo do programa.
 * Primeiro chama busca_tabela_simbolos() (exigida pelo enunciado), que gera o
 * erro [R1] se o nome não existir; depois devolve o nó, para as outras regras
 * consultarem categoria e tipo. Deve ser chamada ANTES de consumir o ID.
 */
TNo *busca_nome_usado(void) {
    busca_tabela_simbolos(lexema_atual); // [R1]
    return busca_simbolo(lexema_atual);
}

// ============================================================================
// FASE 2 - ETAPA 7: GERAÇÃO DE CÓDIGO INTERMEDIÁRIO (MEPA)
// Formato do arquivo saida_mepa.txt:
//   - uma instrução por linha, com 4 espaços na frente:   "    CRVL x"
//   - rótulos na coluna 0, seguidos da instrução:          "L3: NADA"
//   - variáveis referenciadas pelo NOME (exigência do enunciado)
// Instruções cuja forma depende da MEPA da disciplina (confirmar com o professor):
//   DIVF (divisão real), MODI (resto), CRCT com real ou texto, CRVI/ARMI (vetor),
//   e a convenção de chamada de rotinas (ENPR / CHPR / RTPR).
// ============================================================================

#define TAM_BUFFER_PASSO 4096

int gravando_passo = 0;                    // 1 enquanto o código do 'passo' é guardado
char buffer_passo[TAM_BUFFER_PASSO];       // código do 'passo', escrito depois do corpo do 'para'
int total_memoria_global = 0;              // posições reservadas com AMEM (liberadas com DMEM no fim)
int rotulo_saida_rotina = 0;               // rótulo de saída da sub-rotina atual (usado no 'retorne')

/*
 * Função recomendada pelo enunciado: devolve o próximo rótulo positivo (1, 2, 3...).
 * ATENÇÃO: como o parser é recursivo, cada função pega os seus rótulos
 * em variáveis locais NO INÍCIO, antes de chamar as funções internas.
 */
int proximo_rotulo(void) {
    return ++contador_rotulos;
}

/*
 * Ponto único de escrita: manda a linha para o arquivo MEPA ou, durante o
 * 'passo' de um 'para', para o buffer que será escrito depois do corpo do laço.
 */
void emite_linha(const char *linha) {
    if (gravando_passo) {
        if (strlen(buffer_passo) + strlen(linha) + 2 >= TAM_BUFFER_PASSO) {
            printf("Erro: expressao do 'passo' grande demais.\n");
            exit(1);
        }
        strcat(buffer_passo, linha);
        strcat(buffer_passo, "\n");
    } else if (arquivo_mepa != NULL) {
        fprintf(arquivo_mepa, "%s\n", linha);
    }
}

/*
 * Escreve uma instrução no arquivo MEPA, ex.: "CRVL x", "SOMA", "CRCT 5".
 * argumento == NULL para instruções sem argumento.
 */
void gera_codigo(const char *instrucao, const char *argumento) {
    char linha[160];
    if (argumento != NULL) {
        snprintf(linha, sizeof(linha), "    %s %s", instrucao, argumento);
    } else {
        snprintf(linha, sizeof(linha), "    %s", instrucao);
    }
    emite_linha(linha);
}

/* Versão de gera_codigo para argumentos numéricos (AMEM 3, DMEM 3) */
void gera_codigo_numero(const char *instrucao, int numero) {
    char argumento[20];
    snprintf(argumento, sizeof(argumento), "%d", numero);
    gera_codigo(instrucao, argumento);
}

/*
 * Escreve a definição de um rótulo, ex.: "L3: NADA" (ou "L2: ENPR" na entrada de uma rotina).
 */
void gera_rotulo(int rotulo, const char *instrucao) {
    char linha[40];
    snprintf(linha, sizeof(linha), "L%d: %s", rotulo, instrucao);
    emite_linha(linha);
}

/*
 * Escreve um desvio para um rótulo, ex.: "DSVF L3", "DSVS L1" ou "CHPR L2".
 */
void gera_desvio(const char *instrucao, int rotulo) {
    char linha[40];
    snprintf(linha, sizeof(linha), "    %s L%d", instrucao, rotulo);
    emite_linha(linha);
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
// FASE 2: as funções de expressão passam a devolver o tipo calculado [R5]
TipoDado avalia_expressao();
TipoDado avalia_fator();
TipoDado avalia_termo();
TipoDado avalia_expressao_simples();
void avalia_argumento_leia();

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
    // FASE 2: textos literais ("Olá") são impressos como STRING e não incrementam o contador de identificadores
    else if (lookahead.type == TOKEN_STRING) 
    {
        printf("%d# STRING | 0\n", lookahead.line);
        if (arquivo_saida) fprintf(arquivo_saida, "%d# STRING | 0\n", lookahead.line);
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
    gera_codigo("INPP", NULL); // [MEPA] início do programa
    consome_palavra("algoritmo"); // Todo programa em Visualg obrigatoriamente inicia com isso
    // O nome do algoritmo NÃO entra na tabela de símbolos.
    // Ele pode ser um texto entre aspas (algoritmo "Teste") ou um nome simples (algoritmo Teste).
    if (lookahead.type == TOKEN_STRING) {
        consome(TOKEN_STRING);
    } else {
        consome(TOKEN_ID);
    }

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

    // Depois de declarar tudo, entraremos nos comandos lógicos.
    // [MEPA] A memória das globais já foi reservada com AMEM em cada bloco 'var', e o código
    //        de cada sub-rotina é pulado com um DSVS próprio (ver analisa_subrotinas).
    consome_palavra("inicio");
    avalia_lista_comandos();
    consome_palavra("fimalgoritmo");
    // [MEPA] DMEM e PARA são gerados na main, depois da verificação [R4] do escopo global
}

/*
 * Faz a análise sintática do bloco 'var' onde se criam as variáveis.
 */
void processa_bloco_variaveis() {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "var") == 0) {
        consome_palavra("var");
        int memoria_do_bloco = 0; // [MEPA] quantas posições este bloco 'var' reserva (AMEM)

        // [R2] palavra reservada no lugar do nome (ex.: "var se: inteiro") é erro SEMÂNTICO;
        //      sem esta checagem o laço abaixo terminaria e o erro sairia como sintático
        verifica_nome_nao_reservado(1);

        // Fica em loop consumindo e validando variáveis (ex: 'nota1: inteiro') até o lookahead deixar de ser TOKEN_ID
        while (lookahead.type == TOKEN_ID) {
            // [R3] Os nomes (a, b, c) aparecem ANTES do tipo: ficam guardados nesta lista
            //      temporária e só entram na tabela quando o tipo for lido
            char nomes[MAX_NOMES_DECLARACAO][100];
            int linhas[MAX_NOMES_DECLARACAO];
            int quantidade_nomes = 0;
            
            // Loop interno (while 1) para permitir declaração em lote na mesma linha, como em 'a, b, c: inteiro'
            while (1) 
            {
                // Guarda o nome e a linha ANTES de consumir (depois lexema_atual já é o próximo token)
                if (lookahead.type == TOKEN_ID) {
                    if (quantidade_nomes >= MAX_NOMES_DECLARACAO) {
                        erro_semantico(lookahead.line, "Nomes demais na mesma declaração");
                    }
                    strcpy(nomes[quantidade_nomes], lexema_atual);
                    linhas[quantidade_nomes] = lookahead.line;
                    quantidade_nomes++;
                }
                consome(TOKEN_ID); 
                
                // Se encontrar uma vírgula, consome ela e o loop continua esperando mais um TOKEN_ID
                if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0)
                {
                    consome_palavra(",");
                    verifica_nome_nao_reservado(0); // [R2] depois da vírgula tem que vir um nome
                } 
                else 
                {
                    break; // Sai do laço infinito quando acabar a lista de variáveis separadas por vírgula
                }
            }
            consome_palavra(":");

            TipoDado tipo = TIPO_INDEFINIDO; // tipo desta declaração, lido logo abaixo
            int eh_vetor = 0;                // 1 se for 'vetor[..] de <tipo>'
            int tamanho = 1;                 // [MEPA] posições ocupadas por cada nome (vetor: fim - início + 1)

            // Verifica se as variáveis declaradas são na verdade um vetor (array) (ex: vetor[1..10])
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "vetor") == 0) {
                eh_vetor = 1;
                int limite_inicial = 0, limite_final = 0, limites_numericos = 1;
                consome_palavra("vetor");
                consome_palavra("[");
    
                // O índice inicial do vetor pode ser um número explícito ou uma constante (que é um TOKEN_ID)
                if (lookahead.type == TOKEN_NUM_INT)
                {
                    limite_inicial = lookahead.attribute.int_value;
                    consome(TOKEN_NUM_INT);
                } 
                else 
                {
                    if (lookahead.type == TOKEN_ID) busca_nome_usado()->usada = 1; // [R1]
                    limites_numericos = 0;
                    consome(TOKEN_ID);
                }

                // O Visualg exige duas sequências de pontos (ex: 1..10) para definir o alcance do vetor
                consome_palavra(".");
                consome_palavra(".");

                // O índice final também pode ser número ou constante
                if (lookahead.type == TOKEN_NUM_INT) 
                {
                    limite_final = lookahead.attribute.int_value;
                    consome(TOKEN_NUM_INT);
                } 
                else
                {
                    if (lookahead.type == TOKEN_ID) busca_nome_usado()->usada = 1; // [R1]
                    limites_numericos = 0;
                    consome(TOKEN_ID);
                }

                // [MEPA] com limites numéricos sabemos o tamanho; com nomes, reserva 1 posição
                if (limites_numericos && limite_final >= limite_inicial) {
                    tamanho = limite_final - limite_inicial + 1;
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
                    tipo = lexema_para_tipo(lexema_atual); // lido ANTES de consumir
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
                
                tipo = lexema_para_tipo(lexema_atual); // lido ANTES de consumir
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

            // [R3] Agora que o tipo é conhecido, os nomes guardados entram na tabela do escopo atual
            for (int i = 0; i < quantidade_nomes; i++) {
                TNo *simbolo = insere_simbolo(nomes[i], CAT_VARIAVEL, tipo, linhas[i]);
                simbolo->eh_vetor = eh_vetor;
            }
            memoria_do_bloco += quantidade_nomes * tamanho;

            verifica_nome_nao_reservado(1); // [R2] próxima linha do bloco 'var'
        }

        // [MEPA] reserva a memória das variáveis deste bloco
        if (memoria_do_bloco > 0) {
            gera_codigo_numero("AMEM", memoria_do_bloco);
            total_memoria_global += memoria_do_bloco;
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
            int linha_retorne = lookahead.line; // usada na verificação [R9]
            consome_palavra("retorne");
            TipoDado tipo_retornado = avalia_expressao(); // Retornos carregam uma expressão matemática ou lógica consigo
            verifica_retorno(tipo_retornado, linha_retorne); // [R9]
            // [MEPA] convenção do grupo: o valor de retorno fica guardado no nome da função
            //        e o 'retorne' pula para a saída da rotina
            gera_codigo("ARMZ", rotina_atual->ID);
            gera_desvio("DSVS", rotulo_saida_rotina);
        }
        // Tratamento dos Comandos de Entrada/Saída (I/O) nativos do Visualg
        else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "escreva") == 0 || strcmp(lexema_atual, "escreval") == 0 || strcmp(lexema_atual, "leia") == 0)) {
            
            char comando[100];            // Salva qual foi o comando chamado 
            strcpy(comando, lexema_atual); // pois o consome_palavra exige a palavra exata
            
            consome_palavra(comando); 
            consome_palavra("(");     
            
            if (strcmp(comando, "leia") == 0) {
                // leia: cada argumento precisa ser uma variável, que RECEBE o valor lido
                avalia_argumento_leia();
                while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                    consome_palavra(",");
                    avalia_argumento_leia();
                }
            } else {
                // escreva/escreval: cada argumento é uma expressão (as variáveis dela ficam marcadas
                // como usadas [R4]); [MEPA] o código da expressão deixa o valor na pilha e IMPR o imprime
                avalia_expressao();       
                gera_codigo("IMPR", NULL);
                
                // Permite passagem de várias variáveis/expressões separadas por vírgula no escreva()
                while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                    consome_palavra(",");
                    avalia_expressao();
                    gera_codigo("IMPR", NULL);
                }
            }
            consome_palavra(")");     
        }
        // Tratamento de Identificadores (Pode ser: atribuição de variável, acesso a índice de vetor ou chamada de função solta)
        else if (lookahead.type == TOKEN_ID) {
            // [R1] Busca o nome ANTES do consome, enquanto lexema_atual ainda é o nome
            int linha = lookahead.line;
            TNo *simbolo = busca_nome_usado();
            char nome[TAM_ID];
            strcpy(nome, simbolo->ID);
            int eh_rotina = (simbolo->categoria == CAT_PROCEDIMENTO || simbolo->categoria == CAT_FUNCAO);
            int acessou_vetor = 0;
            char mensagem[200];
            consome(TOKEN_ID); 
            
            // Se após o ID vier um colchete, então é o acesso a um vetor (ex: notas[1])
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
                if (!simbolo->eh_vetor) {
                    snprintf(mensagem, sizeof(mensagem), "'%s' não é um vetor", nome);
                    erro_semantico(linha, mensagem);
                }
                consome_palavra("[");
                // A posição do vetor pode ser uma expressão complexa (ex: notas[x+1]) e precisa ser inteira
                verifica_inteiro(avalia_expressao(), "O índice do vetor", linha);
                consome_palavra("]");
                acessou_vetor = 1;
            }
            // Se após o ID vier um parênteses (ou se o nome for de uma rotina chamada sem parênteses),
            // o usuário está chamando uma sub-rotina e passando os argumentos
            else if ((lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) ||
                     (eh_rotina && (lookahead.type != TOKEN_KEYWORD || strcmp(lexema_atual, "<-") != 0))) {
                // [R8] função não pode ser usada como comando (o valor dela seria jogado fora)
                if (simbolo->categoria == CAT_FUNCAO) {
                    snprintf(mensagem, sizeof(mensagem), "Função '%s' não pode ser usada como comando", nome);
                    erro_semantico(linha, mensagem);
                }
                if (simbolo->categoria != CAT_PROCEDIMENTO) {
                    snprintf(mensagem, sizeof(mensagem), "'%s' não é um procedimento", nome);
                    erro_semantico(linha, mensagem);
                }

                // [R6] guarda o tipo de cada argumento, na ordem
                TipoDado tipos_args[MAX_PARAMS];
                int num_args = 0;
                if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                    consome_palavra("(");
                    // Se o próximo não for parênteses de fechamento, então de fato há parâmetros lá dentro
                    if (lookahead.type != TOKEN_KEYWORD || strcmp(lexema_atual, ")") != 0) {
                        TipoDado tipo_arg = avalia_expressao();
                        if (num_args < MAX_PARAMS) tipos_args[num_args] = tipo_arg;
                        num_args++;
                        while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                            consome_palavra(",");
                            tipo_arg = avalia_expressao();
                            if (num_args < MAX_PARAMS) tipos_args[num_args] = tipo_arg;
                            num_args++;
                        }
                    }
                    consome_palavra(")");
                }
                verifica_argumentos(simbolo, tipos_args, num_args, linha);
                // [MEPA] os argumentos já estão na pilha; desvia para a entrada do procedimento
                gera_desvio("CHPR", simbolo->rotulo_entrada);
                continue; // chamada de procedimento não tem atribuição
            }
            
            // Verifica o símbolo de atribuição para a variável ou vetor receber um valor
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "<-") == 0) {
                // Só variável/parâmetro recebe valor (não dá para atribuir a procedimento/função)
                if (eh_rotina) {
                    snprintf(mensagem, sizeof(mensagem), "Não é possível atribuir valor a '%s'", nome);
                    erro_semantico(linha, mensagem);
                }
                consome_palavra("<-");
                TipoDado tipo_valor = avalia_expressao();
                verifica_atribuicao(texto_para_tipo(simbolo->tipo), tipo_valor, nome, linha); // [R5]
                simbolo->atribuida = 1; // [R4]
                // [MEPA] o valor da expressão está no topo da pilha: guarda na variável
                gera_codigo(acessou_vetor ? "ARMI" : "ARMZ", nome);
            }
            else {
                // Variável sozinha não é um comando (ex.: "x" numa linha, sem "<-")
                snprintf(mensagem, sizeof(mensagem), "Variável '%s' usada como comando", nome);
                erro_semantico(linha, mensagem);
            }
        }
    }
}

/*
 * Análise Sintática do bloco 'se' e 'senao'.
 */
void avalia_comando_condicional() {
    // [MEPA] Os rótulos são pegos AQUI, antes das chamadas recursivas, para que um 'se'
    //        aninhado nos comandos internos não use os mesmos números.
    // Padrão:
    //   <código da condição>
    //   DSVF L_senao
    //   <comandos do então>
    //   DSVS L_fim
    //   L_senao: NADA
    //   <comandos do senão>
    //   L_fim: NADA
    int rotulo_senao = proximo_rotulo();
    int rotulo_fim = proximo_rotulo();

    consome_palavra("se");
    consome_palavra("(");
    int linha = lookahead.line;
    TipoDado tipo_condicao = avalia_expressao(); // Aqui será lida a lógica da condição do IF
    verifica_condicao(tipo_condicao, "se", linha); // a condição precisa ser lógica
    consome_palavra(")");
    consome_palavra("entao");
    gera_desvio("DSVF", rotulo_senao); // condição falsa: pula o bloco do 'entao'

    // Chama avalia_lista_comandos() recursivamente, para processar as linhas dentro do IF
    avalia_lista_comandos();
    gera_desvio("DSVS", rotulo_fim);   // terminou o 'entao': pula o 'senao'
    gera_rotulo(rotulo_senao, "NADA");

    // Como o 'senao' é opcional (nem todo IF tem ELSE), nós testamos e só processamos caso ele exista
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "senao") == 0) {
        consome_palavra("senao");
        avalia_lista_comandos();
    }

    gera_rotulo(rotulo_fim, "NADA");
    consome_palavra("fimse");
}

/*
 * Análise Sintática dos laços 'enquanto' e 'para'.
 */
void avalia_comando_repeticao() {
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "enquanto") == 0) {
        // [MEPA] rótulos pegos antes das chamadas recursivas
        // Padrão:
        //   L_inicio: NADA
        //   <código da condição>
        //   DSVF L_fim
        //   <comandos>
        //   DSVS L_inicio
        //   L_fim: NADA
        int rotulo_inicio = proximo_rotulo();
        int rotulo_fim = proximo_rotulo();

        consome_palavra("enquanto");
        gera_rotulo(rotulo_inicio, "NADA");
        consome_palavra("(");
        int linha = lookahead.line;
        TipoDado tipo_condicao = avalia_expressao(); 
        verifica_condicao(tipo_condicao, "enquanto", linha);
        consome_palavra(")");
        consome_palavra("faca");
        gera_desvio("DSVF", rotulo_fim);       // condição falsa: sai do laço
        avalia_lista_comandos(); // Executa tudo que tem dentro do while
        gera_desvio("DSVS", rotulo_inicio);    // volta para testar a condição de novo
        gera_rotulo(rotulo_fim, "NADA");
        consome_palavra("fimenquanto");
    } 
    // Trata o segundo tipo de laço possível no Visualg
    else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "para") == 0) {
        // [MEPA] rótulos pegos antes das chamadas recursivas
        // Padrão (passo positivo):
        //   <início>   ARMZ i
        //   L_inicio: NADA
        //   CRVL i     <fim>   CMEG      (passo negativo: CMAG)
        //   DSVF L_fim
        //   <comandos>
        //   CRVL i     <passo ou 1>   SOMA   ARMZ i
        //   DSVS L_inicio
        //   L_fim: NADA
        // O 'passo' é lido ANTES do corpo, mas o código dele roda DEPOIS do corpo:
        // por isso ele é gravado no buffer_passo e escrito no arquivo só no fim do laço.
        int rotulo_inicio = proximo_rotulo();
        int rotulo_fim = proximo_rotulo();
        char mensagem[200];

        consome_palavra("para");

        // [R1] a variável de controle precisa estar declarada, ser variável e ser inteira
        int linha = lookahead.line;
        char nome[TAM_ID] = "";
        TNo *controle = NULL;
        if (lookahead.type == TOKEN_ID) {
            controle = busca_nome_usado();
            strcpy(nome, controle->ID);
            if (controle->categoria != CAT_VARIAVEL && controle->categoria != CAT_PARAMETRO) {
                snprintf(mensagem, sizeof(mensagem), "'%s' não é uma variável", nome);
                erro_semantico(linha, mensagem);
            }
            snprintf(mensagem, sizeof(mensagem), "A variável de controle '%s'", nome);
            verifica_inteiro(texto_para_tipo(controle->tipo), mensagem, linha);
        }
        consome(TOKEN_ID);       // A variável de iteração do FOR (ex: 'i')
        consome_palavra("de");
        verifica_inteiro(avalia_expressao(), "O valor inicial do 'para'", linha); // O valor de início (ex: '1')
        gera_codigo("ARMZ", nome);
        controle->atribuida = 1; // [R4] o próprio laço escreve e lê a variável de controle
        controle->usada = 1;
        gera_rotulo(rotulo_inicio, "NADA");

        consome_palavra("ate");
        gera_codigo("CRVL", nome);
        verifica_inteiro(avalia_expressao(), "O valor final do 'para'", linha); // O valor final

        // No FOR do Visualg, o passo (incremento) é opcional (o padrão é +1). Verificamos se foi digitado.
        char *codigo_passo = NULL; // cópia do código do passo (um 'para' interno reutiliza o buffer)
        int passo_negativo = 0;
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "passo") == 0) {
            consome_palavra("passo");
            // Passo escrito com sinal de menos (ex.: passo -2) conta de forma decrescente
            passo_negativo = (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "-") == 0);
            buffer_passo[0] = '\0';
            gravando_passo = 1;   // a partir daqui o gerador escreve no buffer
            verifica_inteiro(avalia_expressao(), "O passo do 'para'", linha); 
            gravando_passo = 0;
            codigo_passo = (char *)malloc(strlen(buffer_passo) + 1);
            if (codigo_passo == NULL) {
                printf("Erro: Falha na alocacao de memoria para o passo do 'para'.\n");
                exit(1);
            }
            strcpy(codigo_passo, buffer_passo);
        }
        // Teste de saída: crescente continua enquanto i <= fim; decrescente, enquanto i >= fim
        gera_codigo(passo_negativo ? "CMAG" : "CMEG", NULL);
        gera_desvio("DSVF", rotulo_fim);

        consome_palavra("faca");
        avalia_lista_comandos();

        // Incremento: i <- i + passo (o código do passo estava guardado no buffer)
        gera_codigo("CRVL", nome);
        if (codigo_passo != NULL) {
            if (arquivo_mepa != NULL) fputs(codigo_passo, arquivo_mepa);
            free(codigo_passo);
        } else {
            gera_codigo("CRCT", "1");
        }
        gera_codigo("SOMA", NULL);
        gera_codigo("ARMZ", nome);
        gera_desvio("DSVS", rotulo_inicio);
        gera_rotulo(rotulo_fim, "NADA");
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
 * FASE 2: um parâmetro na declaração de uma sub-rotina ('nome: tipo').
 * [R2] o nome não pode ser palavra reservada
 * [R3] entra na tabela da rotina (topo da pilha) como parâmetro
 * [R6] o tipo é registrado no nó da rotina, para conferir as chamadas
 */
void analisa_parametro() {
    verifica_nome_nao_reservado(0);
    char nome[100] = "";
    int linha = lookahead.line;
    if (lookahead.type == TOKEN_ID) strcpy(nome, lexema_atual); // guardado ANTES de consumir
    consome(TOKEN_ID);
    consome_palavra(":");
    TipoDado tipo = lexema_para_tipo(lexema_atual);             // lido ANTES de consumir
    consome_tipo(); // O tipo do dado esperado
    insere_simbolo(nome, CAT_PARAMETRO, tipo, linha);
    adiciona_parametro(rotina_atual, tipo, linha);
}

/*
 * FASE 2 [MEPA]: no começo da rotina, os argumentos estão na pilha na ordem da
 * chamada (o último no topo). Por isso são guardados do último parâmetro para o primeiro.
 * Neste ponto a tabela do topo só tem os parâmetros, na ordem de declaração.
 */
void gera_armazenamento_parametros() {
    TNo *parametros[MAX_PARAMS];
    int quantidade = 0;
    for (TNo *no = topo_escopo->inicio; no != NULL && quantidade < MAX_PARAMS; no = no->prox) {
        parametros[quantidade++] = no;
    }
    for (int i = quantidade - 1; i >= 0; i--) {
        gera_codigo("ARMZ", parametros[i]->ID);
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
        
        // [MEPA] Padrão de uma sub-rotina (convenção do grupo; confirmar com a MEPA da disciplina):
        //   DSVS L_pula          o programa principal não executa a rotina "de passagem"
        //   L_entrada: ENPR      ponto de entrada (destino do CHPR)
        //   ARMZ <params>        tira da pilha os argumentos empilhados por quem chamou (do último ao primeiro)
        //   <comandos>
        //   L_saida: NADA        destino do 'retorne'
        //   RTPR                 volta para quem chamou
        //   L_pula: NADA
        // Os três rótulos são pegos no começo, antes das chamadas recursivas.
        int rotulo_pula = proximo_rotulo();
        int rotulo_entrada = proximo_rotulo();
        int rotulo_saida = proximo_rotulo();
        gera_desvio("DSVS", rotulo_pula);

        // Procedimentos (métodos void que não possuem um tipo de retorno)
        if (strcmp(lexema_atual, "procedimento") == 0) {
            consome_palavra("procedimento");
            // [R2]/[R3] o nome entra no escopo ATUAL (global), ANTES de abrir o escopo do procedimento,
            // para continuar visível depois que o escopo dele for descartado
            verifica_nome_nao_reservado(0);
            if (lookahead.type == TOKEN_ID) {
                rotina_atual = insere_simbolo(lexema_atual, CAT_PROCEDIMENTO, TIPO_INDEFINIDO, lookahead.line);
                rotina_atual->rotulo_entrada = rotulo_entrada;
            }
            consome(TOKEN_ID); // Consome o nome escolhido pelo usuário para o procedimento
            abre_escopo();     // push: tabela do procedimento (parâmetros)
            rotulo_saida_rotina = rotulo_saida;
            gera_rotulo(rotulo_entrada, "ENPR");
            
            // Trata a entrada de parâmetros obrigatórios entre parênteses (ex: (a: inteiro, b: real))
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                do {
                    // Se não for o primeiro parâmetro, então haverá uma vírgula antes do identificador
                    if (strcmp(lexema_atual, ",") == 0) consome_palavra(",");
                    analisa_parametro(); // [R2]/[R3]/[R6] nome, ':' e tipo do parâmetro
                } while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0); // Repete o 'do' se encontrar outra vírgula
                consome_palavra(")");
            }
            gera_armazenamento_parametros(); // [MEPA]
            
            consome_palavra("inicio");
            avalia_lista_comandos();
            consome_palavra("fimprocedimento"); 
            gera_rotulo(rotulo_saida, "NADA");
            gera_codigo("RTPR", NULL);
            fecha_escopo();      // [R4] confere os parâmetros e pop
            rotina_atual = NULL;
            
        } 
        // Funções (semelhantes aos Procedimentos, porém ao final declaram um tipo de retorno obrigatório)
        else if (strcmp(lexema_atual, "funcao") == 0) {
            consome_palavra("funcao");
            // [R2]/[R3] mesmo tratamento do nome do procedimento.
            // O tipo de retorno só é conhecido depois dos parâmetros: entra como indefinido e é gravado abaixo
            verifica_nome_nao_reservado(0);
            if (lookahead.type == TOKEN_ID) {
                rotina_atual = insere_simbolo(lexema_atual, CAT_FUNCAO, TIPO_INDEFINIDO, lookahead.line);
                rotina_atual->rotulo_entrada = rotulo_entrada;
            }
            consome(TOKEN_ID); 
            abre_escopo();     // push: tabela da função (parâmetros)
            rotulo_saida_rotina = rotulo_saida;
            gera_rotulo(rotulo_entrada, "ENPR");
            
            // A lógica de declaração de parâmetros é idêntica à do Procedimento
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                do {
                    if (strcmp(lexema_atual, ",") == 0) consome_palavra(",");
                    analisa_parametro(); // [R2]/[R3]/[R6]
                } while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0);
                consome_palavra(")");
            }
            gera_armazenamento_parametros(); // [MEPA]
            
            // Ao contrário dos Procedimentos, aqui obrigatoriamente declaramos o tipo que a Função vai retornar
            consome_palavra(":");
            // [R9] grava o tipo de retorno no nó da função (lido ANTES de consumir)
            strcpy(rotina_atual->tipo, tipo_para_texto(lexema_para_tipo(lexema_atual)));
            consome_tipo(); 
            
            consome_palavra("inicio");
            avalia_lista_comandos();
            consome_palavra("fimfuncao"); 
            gera_rotulo(rotulo_saida, "NADA");
            gera_codigo("RTPR", NULL);
            fecha_escopo();      // [R4] confere os parâmetros e pop
            rotina_atual = NULL;
        }
        gera_rotulo(rotulo_pula, "NADA");
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
TipoDado avalia_fator() {
    // Trata sinais negativos ou positivos unários (ex: 'x <- -1') 
    // Em vez de subtrair dois números, apenas muda o sinal do número a seguir
    if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "-") == 0 || strcmp(lexema_atual, "+") == 0)) {
        char op[10];
        strcpy(op, lexema_atual); // Guarda o sinal +/-
        int linha = lookahead.line;
        consome_palavra(op);
        TipoDado tipo = avalia_fator(); // Chama a si mesma novamente para consumir o número alvo desse sinal
        // [R5] o sinal só se aplica a inteiro/real
        if (!eh_numerico(tipo)) {
            char mensagem[200];
            snprintf(mensagem, sizeof(mensagem), "O sinal '%s' não se aplica a %s", op, nome_do_tipo(tipo));
            erro_semantico(linha, mensagem);
        }
        if (strcmp(op, "-") == 0) gera_codigo("INVR", NULL); // [MEPA] inverte o sinal do topo da pilha
        return tipo;
    }

    TipoDado tipo = TIPO_INDEFINIDO; // [R5] tipo deste fator, devolvido no final (atributo sintetizado)

    // Se a base da expressão for uma variável (ID) ou uma função
    if (lookahead.type == TOKEN_ID) {
        // [R1] busca ANTES de consumir, enquanto lexema_atual ainda é o nome
        int linha = lookahead.line;
        TNo *simbolo = busca_nome_usado();
        char nome[TAM_ID];
        strcpy(nome, simbolo->ID);
        char mensagem[200];
        // [R7] procedimento não devolve valor, então não pode estar em expressão
        if (simbolo->categoria == CAT_PROCEDIMENTO) {
            snprintf(mensagem, sizeof(mensagem), "Procedimento '%s' não pode ser usado em expressão", nome);
            erro_semantico(linha, mensagem);
        }
        tipo = texto_para_tipo(simbolo->tipo); // [R5] o tipo vem do caderno
        consome(TOKEN_ID);
        
        // Trata caso seja o acesso à posição de um vetor dentro de um cálculo matemático
        if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
            if (!simbolo->eh_vetor) {
                snprintf(mensagem, sizeof(mensagem), "'%s' não é um vetor", nome);
                erro_semantico(linha, mensagem);
            }
            consome_palavra("[");
            verifica_inteiro(avalia_expressao(), "O índice do vetor", linha);
            consome_palavra("]");
            simbolo->usada = 1;          // [R4]
            gera_codigo("CRVI", nome);   // [MEPA] empilha o elemento (o índice já está na pilha)
        } 
        // Trata chamada de função no meio do cálculo e lê os parâmetros passados para ela
        // (uma função sem parâmetros também pode ser chamada sem os parênteses)
        else if ((lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) ||
                 simbolo->categoria == CAT_FUNCAO) {
            if (simbolo->categoria != CAT_FUNCAO) {
                snprintf(mensagem, sizeof(mensagem), "'%s' não é uma função", nome);
                erro_semantico(linha, mensagem);
            }
            // [R6] guarda o tipo de cada argumento, na ordem
            TipoDado tipos_args[MAX_PARAMS];
            int num_args = 0;
            if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
                consome_palavra("(");
                if (lookahead.type != TOKEN_KEYWORD || strcmp(lexema_atual, ")") != 0) {
                    TipoDado tipo_arg = avalia_expressao();
                    if (num_args < MAX_PARAMS) tipos_args[num_args] = tipo_arg;
                    num_args++;
                    while (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, ",") == 0) {
                        consome_palavra(",");
                        tipo_arg = avalia_expressao();
                        if (num_args < MAX_PARAMS) tipos_args[num_args] = tipo_arg;
                        num_args++;
                    }
                }
                consome_palavra(")");
            }
            verifica_argumentos(simbolo, tipos_args, num_args, linha);
            // [MEPA] os argumentos já estão na pilha; chama a função e depois empilha
            //        o valor que ela guardou no próprio nome (convenção do grupo)
            gera_desvio("CHPR", simbolo->rotulo_entrada);
            gera_codigo("CRVL", nome);
            // [R5] o tipo deste fator é o tipo de retorno da função (já está em 'tipo')
        }
        // Variável ou parâmetro simples
        else {
            simbolo->usada = 1;          // [R4] o valor foi lido
            gera_codigo("CRVL", nome);   // [MEPA] empilha o valor da variável
        }
    } 
    // Outros casos atômicos que formam um Fator:
    else if (lookahead.type == TOKEN_NUM_INT) {
        tipo = TIPO_INTEIRO;
        gera_codigo("CRCT", lexema_atual); // [MEPA] antes de consumir, enquanto lexema_atual é o número
        consome(TOKEN_NUM_INT); // Um número inteiro solto
    } else if (lookahead.type == TOKEN_NUM_FLOAT) {
        tipo = TIPO_REAL;
        gera_codigo("CRCT", lexema_atual); // [MEPA] constante real (depende da MEPA da disciplina)
        consome(TOKEN_NUM_FLOAT); // Um número decimal solto
    } else if (lookahead.type == TOKEN_KEYWORD && (strcmp(lexema_atual, "verdadeiro") == 0 || strcmp(lexema_atual, "falso") == 0)) {
        tipo = TIPO_LOGICO;
        // Valores lógicos booleanos puros. [MEPA] verdadeiro = 1, falso = 0
        if (strcmp(lexema_atual, "verdadeiro") == 0) {
            gera_codigo("CRCT", "1");
            consome_palavra("verdadeiro");
        } else {
            gera_codigo("CRCT", "0");
            consome_palavra("falso");
        }
    } else if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "(") == 0) {
        // Encontrou um parêntese abrindo! O LL(1) prioriza isso: 
        // Chama a raiz avalia_expressao() para resolver toda a cadeia interna isoladamente primeiro
        consome_palavra("(");
        tipo = avalia_expressao(); // o tipo do parêntese é o tipo da expressão de dentro
        consome_palavra(")");
    }
    // FASE 2: texto literal como fator (ex.: escreval("Total: ", x))
    else if (lookahead.type == TOKEN_STRING) {
        tipo = TIPO_CARACTERE;
        gera_codigo("CRCT", lexema_atual); // [MEPA] texto (depende da MEPA da disciplina)
        consome(TOKEN_STRING);
    }
    else {
        printf("ERRO SINTÁTICO\n");
        printf("Linha: %d\n", lookahead.line);
        printf("Fator inválido na expressão matemática: %s\n", lexema_atual);
        exit(1);
    }

    return tipo; // [R5] sobe para quem chamou (atributo sintetizado)
}

/*
 * Resolve multiplicações e divisões (Termo).
 * Funciona como intermediário entre a soma/subtração e o fator base.
 * Garante que a multiplicação ocorra antes da soma.
 */
TipoDado avalia_termo() {
    TipoDado tipo = avalia_fator(); // Primeiramente avalia o termo da esquerda
    
    // O laço agrupa e resolve as operações multiplicativas sucessivas (ex: 5 * 2 / 2)
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "*") == 0 || strcmp(lexema_atual, "/") == 0 || 
           strcmp(lexema_atual, "\\") == 0 || strcmp(lexema_atual, "MOD") == 0 || 
           strcmp(lexema_atual, "E") == 0)) {
        
        char operador[10];
        strcpy(operador, lexema_atual);
        int linha = lookahead.line;
        consome_palavra(operador); // Consome o operador (*)
        TipoDado tipo_direita = avalia_fator(); // Processa e encadeia o número ou expressão à direita da multiplicação
        tipo = verifica_operacao(tipo, operador, tipo_direita, linha); // [R5]

        // [MEPA] o operador vem DEPOIS dos dois operandos (notação posfixa)
        if (strcmp(operador, "*") == 0)        gera_codigo("MULT", NULL);
        else if (strcmp(operador, "/") == 0)   gera_codigo("DIVF", NULL); // divisão real
        else if (strcmp(operador, "\\") == 0) gera_codigo("DIVI", NULL); // divisão inteira
        else if (strcmp(operador, "MOD") == 0) gera_codigo("MODI", NULL); // resto (depende da MEPA da disciplina)
        else                                   gera_codigo("CONJ", NULL); // E
    }

    return tipo;
}

/*
 * Resolve somas e subtrações (Expressão Simples).
 */
TipoDado avalia_expressao_simples() {
    TipoDado tipo = avalia_termo(); // Primeiramente chama o termo, que resolve a multiplicação, e retorna para ser somado
    
    // Semelhante à multiplicação, o laço resolve as operações aditivas sucessivas (ex: 5 + 2 - 2)
    while (lookahead.type == TOKEN_KEYWORD && 
          (strcmp(lexema_atual, "+") == 0 || strcmp(lexema_atual, "-") == 0 || 
           strcmp(lexema_atual, "OU") == 0)) {
        
        char operador[10];
        strcpy(operador, lexema_atual);
        int linha = lookahead.line;
        consome_palavra(operador); // Consome o operador de adição (+)
        TipoDado tipo_direita = avalia_termo(); // Encapsula os termos em cascata
        tipo = verifica_operacao(tipo, operador, tipo_direita, linha); // [R5]

        // [MEPA] operador depois dos dois operandos
        if (strcmp(operador, "+") == 0)      gera_codigo("SOMA", NULL);
        else if (strcmp(operador, "-") == 0) gera_codigo("SUBT", NULL);
        else                                 gera_codigo("DISJ", NULL); // OU
    }

    return tipo;
}

/*
 * Porta de entrada da validação matemática e lógica.
 * Inicia a cascata (chama expressao simples, que chama termo, que chama fator)
 * Verifica a presença de operadores relacionais (como >, <, =) ligando as duas sub-expressões resultantes.
 */
TipoDado avalia_expressao() {
    TipoDado tipo = avalia_expressao_simples(); 
    
    // Testa o token no meio, se for relacional é uma validação booleana (ex: expressao_esq > expressao_dir)
    if (lookahead.type == TOKEN_OP_REL) {
        // Guarda o operador ANTES de consumir (para a mensagem de erro e a instrução MEPA)
        OpRelAtributo codigo_operador = lookahead.attribute.op_code;
        char operador[3];
        strcpy(operador, lexema_atual);
        int linha = lookahead.line;
        consome(TOKEN_OP_REL); 
        TipoDado tipo_direita = avalia_expressao_simples();
        tipo = verifica_operacao(tipo, operador, tipo_direita, linha); // [R5] resultado: logico

        // [MEPA] comparação depois dos dois operandos; deixa 1 (verdadeiro) ou 0 (falso) na pilha
        switch (codigo_operador) {
            case OP_LT: gera_codigo("CMME", NULL); break; // <
            case OP_LE: gera_codigo("CMEG", NULL); break; // <=
            case OP_EQ: gera_codigo("CMIG", NULL); break; // =
            case OP_GT: gera_codigo("CMMA", NULL); break; // >
            case OP_GE: gera_codigo("CMAG", NULL); break; // >=
            case OP_NE: gera_codigo("CMDG", NULL); break; // <>
        }
    }

    return tipo;
}

/*
 * FASE 2: um argumento do 'leia'.
 * Precisa ser uma variável (ou elemento de vetor), que RECEBE o valor lido.
 * [R1] declarada   [R4] marcada como atribuída
 * [MEPA] LEIT empilha o valor lido e ARMZ (ou ARMI, no vetor) guarda na variável
 */
void avalia_argumento_leia() {
    int linha = lookahead.line;
    char mensagem[200];

    if (lookahead.type != TOKEN_ID) {
        snprintf(mensagem, sizeof(mensagem), "'leia' exige uma variável, mas recebeu '%s'", lexema_atual);
        erro_semantico(linha, mensagem);
    }
    TNo *simbolo = busca_nome_usado(); // [R1]
    char nome[TAM_ID];
    strcpy(nome, simbolo->ID);
    if (simbolo->categoria != CAT_VARIAVEL && simbolo->categoria != CAT_PARAMETRO) {
        snprintf(mensagem, sizeof(mensagem), "'%s' não é uma variável", nome);
        erro_semantico(linha, mensagem);
    }
    consome(TOKEN_ID);

    int acessou_vetor = 0;
    if (lookahead.type == TOKEN_KEYWORD && strcmp(lexema_atual, "[") == 0) {
        if (!simbolo->eh_vetor) {
            snprintf(mensagem, sizeof(mensagem), "'%s' não é um vetor", nome);
            erro_semantico(linha, mensagem);
        }
        consome_palavra("[");
        verifica_inteiro(avalia_expressao(), "O índice do vetor", linha);
        consome_palavra("]");
        acessou_vetor = 1;
    }

    simbolo->atribuida = 1; // [R4]
    gera_codigo("LEIT", NULL);
    gera_codigo(acessou_vetor ? "ARMI" : "ARMZ", nome);
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

    // FASE 2 [MEPA]: arquivo com o código intermediário gerado
    arquivo_mepa = fopen("saida_mepa.txt", "w");
    if (arquivo_mepa == NULL) {
        printf("Erro: Nao foi possivel criar o arquivo saida_mepa.txt\n");
        free(codigo_fonte);
        if (arquivo_saida != NULL) fclose(arquivo_saida);
        return 1;
    }

    // FASE 2: cria a tabela do escopo GLOBAL (base da pilha) antes de começar a análise
    abre_escopo();

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
    
    // FASE 2 [R4]: verifica as variáveis globais atribuídas e não usadas e libera a tabela global
    fecha_escopo();

    // [MEPA] libera a memória das globais e encerra o programa MEPA
    if (total_memoria_global > 0) gera_codigo_numero("DMEM", total_memoria_global);
    gera_codigo("PARA", NULL);
    fclose(arquivo_mepa);
    arquivo_mepa = NULL;

    // Se o código passar vivo por todas as funções do analisador sem acionar nenhum erro
    // (léxico, sintático ou semântico), ele está validado e o código MEPA foi gerado.
    printf("Compilação concluída com sucesso. Código sem erros léxicos, sintáticos ou semânticos!\n");
    printf("Código MEPA gerado em saida_mepa.txt\n");

    // Limpa a memória alocada e solta o txt de saída antes de encerrar
    free(codigo_fonte);
    if (arquivo_saida != NULL) fclose(arquivo_saida);

    return 0; // Finaliza o sistema retornando código 0 de encerramento saudável
}
