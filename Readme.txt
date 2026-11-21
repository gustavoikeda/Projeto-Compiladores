=====================================================================
          COMPILADOR MINIVISUALG - PROJETO FASE 2
          Análise Semântica e Geração de Código Intermediário (MEPA)
=====================================================================

Integrantes:
  Guilherme Limeira de Souza   RA: 10439777
  Gustavo Kiyoshi Ikeda        RA: 10439179
  Pedro Montarroyos de Pinho   RA: 10440213


---------------------------------------------------------------------
1. COMO COMPILAR E EXECUTAR
---------------------------------------------------------------------

  gcc -g -Og -Wall compilador.c -o compilador
  ./compilador teste.txt

O compilador compila sem warnings. Ao final ele gera dois arquivos:
  saida_tokens.txt  -> tokens reconhecidos pelo léxico (igual à Fase 1)
  saida_mepa.txt    -> código intermediário MEPA (Fase 2)

Se o programa estiver correto, a última mensagem é:
  Compilação concluída com sucesso. Código sem erros léxicos, sintáticos ou semânticos!
Se houver erro, o compilador para no primeiro erro e mostra:
  ERRO LÉXICO / ERRO SINTÁTICO / ERRO SEMÂNTICO
  Linha: N
  <descrição do erro>


---------------------------------------------------------------------
2. O QUE FOI FEITO
---------------------------------------------------------------------

Tudo o que o enunciado da Fase 2 pede foi implementado:

[x] Análise léxica e sintática da Fase 1 mantidas (mesma struct Token).
    Única mudança no léxico: textos entre aspas passaram a ser
    TOKEN_STRING (antes eram TOKEN_ID), para não serem procurados na
    tabela de símbolos.

[x] Tabela de símbolos em lista encadeada com o TNo do enunciado
    (ID[16], endereco, tipo[7], prox). Endereço = ordem de declaração
    (0, 1, 2...). Seguindo a orientação do professor, há uma tabela por
    escopo, organizadas em pilha (global na base; cada procedimento ou
    função empilha a sua), e cada tabela guarda todas as categorias
    juntas (variáveis, parâmetros, procedimentos e funções).

[x] Funções pedidas no enunciado: busca_tabela_simbolos() (devolve o
    endereço ou gera erro semântico de não declarado) e proximo_rotulo().

[x] Regras semânticas, todas com mensagem de erro:
    R1 variável declarada antes de ser usada
    R2 palavra reservada não pode ser nome
    R3 identificador único no mesmo escopo
    R4 variável com valor atribuído deve ser usada
    R5 tipos compatíveis nas expressões e atribuições
    R6 tipo e número dos argumentos das chamadas
    R7 procedimento não pode ser usado em expressão
    R8 função não pode ser usada como procedimento
    R9 retorno compatível com o tipo da função
    Também são verificados: condição de 'se'/'enquanto' do tipo logico,
    índice de vetor inteiro, variável de controle do 'para' inteira e
    argumentos do 'leia' que precisam ser variáveis.

[x] Geração de código MEPA para todos os comandos: atribuição,
    expressões, escreva/escreval, leia, se/senao, enquanto, para (com
    passo opcional, inclusive negativo), procedimentos, funções e
    retorne. As variáveis são referenciadas pelo nome.


---------------------------------------------------------------------
3. DECISÕES DO GRUPO (pontos que o enunciado não define)
---------------------------------------------------------------------

Tipos
  - Abreviações no campo tipo[7], porque "inteiro" e "caractere" não
    cabem em 7 posições: int, real, char, logico.
  - + - *  : inteiro com inteiro dá inteiro; com real dá real.
  - /      : sempre real.   \ e MOD : só inteiros.
  - E, OU  : só logico.
  - < <= > >= : números.   = <> : números ou dois valores do mesmo tipo.
  - Atribuição: real <- inteiro é permitido; inteiro <- real é erro.

Tabela de símbolos
  - Campos acrescentados ao TNo (os 4 do enunciado foram mantidos):
    categoria, eh_vetor, num_params, tipos_params, atribuida, usada,
    linha_declaracao e rotulo_entrada. Eles são necessários porque a
    tabela de cada escopo guarda todas as categorias juntas e as regras
    R4, R6, R7 e R8 precisam saber o que cada nome é.
  - Unicidade (R3) verificada dentro do mesmo escopo: um parâmetro pode
    ter o mesmo nome de uma variável global (o mais interno vale).
  - Procedimentos e funções não ocupam endereço (endereço -1).
  - Nomes com mais de 15 caracteres geram erro semântico (não cabem em
    ID[16]).

Regra R4
  - Parâmetros já chegam com valor (passado na chamada): parâmetro não
    usado é erro.
  - Variável declarada e nunca atribuída NÃO é erro (o enunciado só fala
    de variável com valor atribuído).

MEPA
  - Rótulos no formato "L3: NADA"; instruções com 4 espaços na frente.
  - Sub-rotinas: DSVS (pula a rotina), "Ln: ENPR" na entrada, ARMZ dos
    parâmetros (do último ao primeiro), corpo, RTPR. A chamada empilha os
    argumentos e usa CHPR Ln. O valor de retorno de uma função fica
    guardado no nome da própria função (retorne -> ARMZ nome_da_funcao).
  - O código do 'passo' do 'para' é guardado e escrito depois do corpo
    do laço.
  - Instruções que dependem da MEPA usada pela disciplina e podem precisar
    de ajuste: DIVF (divisão real), MODI (resto), CRCT com número real ou
    texto, CRVI/ARMI (vetores) e a convenção de chamada de rotinas.
    Toda a escrita passa pelas funções gera_codigo, gera_rotulo e
    gera_desvio, então um ajuste de formato fica concentrado nelas.


---------------------------------------------------------------------
4. LIMITAÇÕES CONHECIDAS
---------------------------------------------------------------------

  - Como as variáveis são referenciadas pelo nome, uma função recursiva
    compartilha a mesma variável para todos os níveis de chamada.
  - Vetor com limites dados por nomes (ex.: vetor[1..n]) reserva só uma
    posição, porque o tamanho não é conhecido na compilação.
  - String sem aspas de fechamento não gera erro léxico (comportamento
    herdado da Fase 1).
  - O compilador termina com código 1 quando encontra erro (léxico,
    sintático ou semântico) e com 0 quando a compilação é bem-sucedida.


---------------------------------------------------------------------
5. USO DE INTELIGÊNCIA ARTIFICIAL
---------------------------------------------------------------------

A Fase 2 foi desenvolvida com auxílio do Claude (Anthropic). A IA foi
usada para gerar o esqueleto da Fase 2 (estruturas e assinaturas das
funções), implementar a tabela de símbolos, as verificações semânticas
R1-R9 e a geração de código MEPA, escrever os comentários explicativos e
este Readme, e montar materiais de estudo e casos de teste. O grupo
revisou, testou e estudou o código gerado. A mesma declaração está no
cabeçalho do compilador.c.
