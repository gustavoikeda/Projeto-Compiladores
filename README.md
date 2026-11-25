# Compilador MiniVisualg

Compilador da linguagem **MiniVisualg** escrito em C, feito em duas fases para a disciplina de Compiladores (Mackenzie):

| Fase | O que o compilador faz | Saída |
| --- | --- | --- |
| **Fase 1** | Análise léxica e análise sintática (front-end) | `saida_tokens.txt` |
| **Fase 2** | Análise semântica (regras R1 a R9) e geração de código intermediário MEPA | `saida_mepa.txt` |

Tudo acontece **numa passada só**: o analisador sintático pede tokens ao léxico, chama as verificações semânticas e manda o gerador escrever o MEPA enquanto lê o programa.

## 👥 Alunos

| Nome | RA |
| --- | --- |
| Guilherme Limeira de Souza | 10439777 |
| Gustavo Kiyoshi Ikeda | 10439179 |
| Pedro Montarroyos de Pinho | 10440213 |

---

## 🚀 Como compilar e executar

Precisa só do GCC (MinGW no Windows, ou nativo no Linux/Mac).

```bash
gcc -g -Og -Wall compilador.c -o compilador
./compilador teste.txt
```

O código compila sem warnings. Ao final da execução são gerados dois arquivos na pasta atual:

| Arquivo | Conteúdo |
| --- | --- |
| `saida_tokens.txt` | Os tokens reconhecidos pelo léxico, uma linha por token: `linha# CATEGORIA \| atributo` (Fase 1) |
| `saida_mepa.txt` | O código intermediário MEPA (Fase 2) |

Se o programa estiver correto, a última mensagem é:

```
Compilação concluída com sucesso. Código sem erros léxicos, sintáticos ou semânticos!
Código MEPA gerado em saida_mepa.txt
```

Se houver erro, o compilador para no **primeiro** erro e mostra:

```
ERRO LÉXICO | ERRO SINTÁTICO | ERRO SEMÂNTICO
Linha: N
<descrição do erro>
```

Os 21 programas de exemplo do anexo do enunciado compilam sem erro.

---

## 🏗️ Arquitetura

```
main ──► PARSER (sintático) ──┬──► LÉXICO     "me dê o próximo token"
                              ├──► SEMÂNTICO  "isto é permitido?"
                              └──► GERADOR    "escreva esta instrução MEPA"
```

O `compilador.c` está dividido em etapas:

| Etapa | Fase | Conteúdo | Funções principais |
| --- | --- | --- | --- |
| 1 | 1 e 2 | Estruturas de dados e variáveis globais | `Token`, `TipoDado`, `Categoria`, `TNo`, `TEscopo` |
| 2 | 1 | Analisador léxico | `obterToken` |
| 3 | 1 e 2 | Analisador sintático (descendente recursivo LL(1)) | `nextToken`, `consome*`, `analisa_*`, `avalia_*` |
| 4 | 2 | Erros semânticos e conversão de tipos | `erro_semantico`, `lexema_para_tipo`, `tipo_para_texto`, `texto_para_tipo` |
| 5 | 2 | Tabela de símbolos (pilha de tabelas por escopo) | `abre_escopo`, `fecha_escopo`, `insere_simbolo`, `busca_simbolo`, `busca_tabela_simbolos` |
| 6 | 2 | Verificações semânticas | `verifica_operacao`, `verifica_atribuicao`, `verifica_argumentos`, `verifica_retorno`... |
| 7 | 2 | Geração de código MEPA | `proximo_rotulo`, `gera_codigo`, `gera_rotulo`, `gera_desvio` |

### Fase 1: léxico e sintático

**Estruturas (`structs` e `enums`).** `TokenNome` classifica as categorias gerais (identificador, número, operador relacional, palavra reservada). A `struct Token` guarda o atributo numa `union` (valor inteiro, real ou código do operador), porque um token só tem um desses valores por vez.

**Analisador léxico.** A função `obterToken()` funciona como um autômato que lê o código-fonte, já carregado na memória, caractere por caractere:

- **Filtro:** ignora espaços, quebras de linha e comentários `//`.
- **Formação de lexemas:** agrupa os caracteres e decide se é identificador, número ou palavra reservada.
- **Lookahead de 1 caractere:** resolve operadores duplos. Ao ler `<`, olha o próximo: `-` forma a atribuição `<-`, `=` forma `<=`, `>` forma `<>`; senão é só `<`.

**Analisador sintático (descendente recursivo LL(1)).** Cada regra da gramática é uma função. O parser chama o léxico sob demanda (`nextToken()`) e decide o que fazer olhando só o token atual (`lookahead`).

- **Cascata de estruturas:** começa em `analisa_algoritmo_principal()`, que consome `algoritmo`, as declarações (variáveis e sub-rotinas), `inicio`, os comandos e `fimalgoritmo`.
- **Precedência:** a hierarquia desce `avalia_expressao` (relacionais) → `avalia_expressao_simples` (`+ - OU`) → `avalia_termo` (`* / \ MOD E`) → `avalia_fator` (números, variáveis, chamadas, parênteses). Assim `1 + 2 * 3` multiplica antes de somar.
- **EOF Lock:** depois do `fimalgoritmo`, a `main` exige o fim do arquivo. Qualquer texto sobrando é erro sintático.

### Fase 2: semântico e MEPA

**Semântica dirigida pela sintaxe.** As verificações e a geração de código foram inseridas dentro das funções do parser, nos pontos em que a informação necessária já foi lida. Nenhuma árvore sintática é montada: ela fica implícita na pilha de chamadas recursivas.

**Tipos como atributo sintetizado.** As funções de expressão (`avalia_expressao`, `avalia_expressao_simples`, `avalia_termo`, `avalia_fator`) devolvem o `TipoDado` do pedaço que analisaram. O tipo sobe de `avalia_fator` até `avalia_expressao`, sendo conferido a cada operador.

**MEPA em ordem posfixa.** Cada operando é empilhado ao ser lido (`CRVL`, `CRCT`) e o operador é emitido depois dos dois operandos (`SOMA`, `MULT`, `CMMA`...). É a ordem em que a recursão termina.

**Regras semânticas** (todas geram `ERRO SEMÂNTICO` com a linha e uma mensagem):

| Regra | O que verifica | Onde é verificada |
| --- | --- | --- |
| R1 | variável declarada antes de ser usada | `busca_tabela_simbolos` (via `busca_nome_usado`) |
| R2 | palavra reservada não pode ser nome | `verifica_nome_nao_reservado` |
| R3 | identificador único no mesmo escopo | `insere_simbolo` |
| R4 | variável com valor atribuído deve ser usada | `verifica_variaveis_nao_usadas` (ao fechar cada escopo) |
| R5 | tipos compatíveis em expressões e atribuições | `verifica_operacao`, `verifica_atribuicao` |
| R6 | número e tipo dos argumentos das chamadas | `verifica_argumentos` |
| R7 | procedimento não pode ser usado em expressão | `avalia_fator` |
| R8 | função não pode ser usada como procedimento (comando) | `avalia_lista_comandos` |
| R9 | retorno compatível com o tipo da função | `verifica_retorno` |

Além delas, também são verificados: a condição do `se`/`enquanto` precisa ser `logico`; índice de vetor, variável de controle e valores do `para` precisam ser inteiros; o `leia` só aceita variáveis.

**Funções pedidas no enunciado:** `busca_tabela_simbolos()`, que devolve o endereço ou dá o erro de não declarado, e `proximo_rotulo()`, que devolve L1, L2, L3...

---

## 🧭 Nossas escolhas na Fase 2

O enunciado não define tudo. Esta seção lista cada decisão do grupo, o motivo e o que ela muda no comportamento do compilador.

### Léxico

| Escolha | Motivo |
| --- | --- |
| Textos entre aspas passaram a ser `TOKEN_STRING` (na Fase 1 eram `TOKEN_ID`) | Sem isso, `escreval("Olá")` faria o semântico procurar `"Olá"` na tabela de símbolos e dar erro de não declarado. Foi a única mudança no léxico. |

### Tabela de símbolos

| Escolha | Motivo |
| --- | --- |
| **Uma tabela por escopo, organizadas em pilha** (global na base; cada procedimento/função empilha a sua ao começar e desempilha no fim) | Orientação do professor. A busca vai do topo para a base, então um nome local esconde um global de mesmo nome. |
| **Todas as categorias na mesma tabela** (variável, parâmetro, procedimento, função) | Orientação do professor. Um nome é um só, seja ele variável ou rotina. |
| **Os 4 campos do `TNo` do enunciado foram mantidos** (`ID[16]`, `endereco`, `tipo[7]`, `prox`) e **acrescentamos**: `categoria`, `eh_vetor`, `num_params`, `tipos_params`, `atribuida`, `usada`, `linha_declaracao`, `rotulo_entrada` | Com todas as categorias juntas, as regras precisam saber o que cada nome é: R7 e R8 olham a categoria, R6 olha os parâmetros, R4 olha `atribuida`/`usada`, e o `CHPR` precisa do rótulo de entrada. |
| **Endereço = ordem de declaração dentro do escopo** (0, 1, 2...) | Cada escopo recomeça do 0. Procedimentos e funções não ocupam memória de dados e ficam com endereço -1. |
| **O nome da rotina entra no escopo de fora** e os parâmetros no escopo dela | Assim a rotina continua visível (pode ser chamada) depois que o escopo dela é desempilhado. |
| **R3 só dentro do mesmo escopo** | Um parâmetro pode ter o mesmo nome de uma variável global; vale o mais interno. |


### Tipos

| Escolha | Motivo |
| --- | --- |
| **Abreviações no campo `tipo[7]`:** `int`, `real`, `char`, `logico` | `"inteiro"` (7 letras + `'\0'`) e `"caractere"` não cabem em 7 posições. Por isso existem as conversões `lexema_para_tipo` (texto do fonte → enum), `tipo_para_texto` (enum → campo do nó) e `texto_para_tipo` (campo do nó → enum). |
| **Internamente os tipos são um `enum TipoDado`** | Comparar tipos com `==` é mais simples e seguro do que comparar strings. |
| `+ - *`: inteiro com inteiro dá inteiro; se um dos lados for real, dá real | Promoção numérica usual. |
| **`/` sempre dá real** (gera `DIVF`); **`\` é a divisão inteira** (gera `DIVI`) e `MOD` só aceita inteiros | Segue o VisuAlg, em que `5 / 2` vale 2.5 e a divisão inteira tem operador próprio. |
| `E`, `OU`: só `logico`. `< <= > >=`: só números. `= <>`: números, ou dois valores do mesmo tipo. Toda comparação dá `logico` | Evita comparar valores sem sentido (ex.: `logico < inteiro`). |
| **Atribuição: `real <- inteiro` é permitida; `inteiro <- real` é erro** | O inteiro cabe no real sem perder nada; o contrário perderia a parte decimal. A mesma regra vale para argumentos (R6) e para o `retorne` (R9). |

### Regras semânticas

| Escolha | Motivo |
| --- | --- |
| **R2 é verificada antes do sintático tentar consumir o nome** | O léxico já classifica `se` como palavra reservada, então `var se: inteiro` daria erro **sintático**. A função `verifica_nome_nao_reservado` olha o token antes, para o erro sair como **semântico** (R2), como pede o enunciado. No começo de cada linha do `var`, as palavras `inicio`, `var`, `procedimento` e `funcao` são aceitas porque encerram o bloco. |
| **R4: parâmetro já nasce "atribuído"** | O valor vem de quem chamou. Então um parâmetro que nunca é usado é erro. |
| **R4: variável declarada e nunca atribuída NÃO é erro** | O enunciado fala de variável "com valor atribuído". |
| **R4 é conferida ao fechar cada escopo**, e o erro aponta a linha da declaração | Só no fim do escopo dá para saber que a variável nunca foi usada. |
| **`leia` exige variável** | `leia(5)` passou a ser erro: o argumento do `leia` recebe um valor. |
| **Variável sozinha numa linha é erro** | `x` sem `<-` não é um comando. |
| **Procedimento sem parênteses é reconhecido como chamada** | `p` sozinho numa linha, sendo `p` um procedimento, é tratado como `p()`. |

### Geração de código MEPA

| Escolha | Motivo |
| --- | --- |
| **Variáveis referenciadas pelo nome** (`CRVL x`, `ARMZ x`) | Exigência do enunciado. |
| **Formato:** instruções com 4 espaços na frente; rótulos na coluna 0 seguidos da instrução (`L3: NADA`) | Fica legível e separa visualmente os rótulos. |
| **`INPP` no início; `AMEM n` a cada bloco `var`; `DMEM` e `PARA` no fim** | O `DMEM` total e o `PARA` são gerados na `main`, depois da R4 do escopo global. |
| **Os rótulos de cada estrutura são pegos no começo da função**, antes das chamadas recursivas | Como o parser é recursivo, um `se` dentro de outro `se` pegaria os mesmos números se os rótulos fossem pedidos depois. |
| **`se`:** condição, `DSVF L_senao`, então, `DSVS L_fim`, `L_senao: NADA`, senão, `L_fim: NADA` | Molde padrão de desvio condicional. |
| **`enquanto`:** `L_ini: NADA`, condição, `DSVF L_fim`, corpo, `DSVS L_ini`, `L_fim: NADA` | Molde padrão de laço com teste no início. |
| **`para`:** o código do `passo` é guardado num buffer e escrito **depois do corpo** | No fonte o passo aparece antes do corpo, mas precisa ser executado depois dele. Com passo negativo, a comparação usa `CMAG` em vez de `CMEG`. |
| **Sub-rotinas:** `DSVS L_pula`, `L_entrada: ENPR`, `ARMZ` dos parâmetros, corpo, `L_saida: NADA`, `RTPR`, `L_pula: NADA` | O `DSVS` impede que o programa principal execute a rotina "de passagem". |
| **Os parâmetros são guardados do último para o primeiro** | Quem chama empilha os argumentos na ordem; o último fica no topo da pilha. |
| **Chamada:** argumentos empilhados e depois `CHPR L_entrada` | O rótulo de entrada fica guardado no nó da rotina (`rotulo_entrada`). |
| **O valor de retorno de uma função fica guardado no nome da própria função** (`retorne` → `ARMZ nome_da_funcao`; quem chamou faz `CRVL nome_da_funcao`) | Convenção simples que dispensa uma área de retorno separada. O `retorne` também desvia para `L_saida`. |
| `verdadeiro` e `falso` viram `CRCT 1` e `CRCT 0` | A MEPA não tem tipo lógico. |
| **Toda escrita passa por `gera_codigo`, `gera_rotulo` e `gera_desvio`** | Se o formato da MEPA da disciplina for diferente, o ajuste fica concentrado nessas funções. |

---

## 🤖 Uso de Inteligência Artificial

A Fase 2 foi desenvolvida com auxílio do Claude. A IA foi usada para gerar o esqueleto da Fase 2 (estruturas e assinaturas das funções), entender como a implementação das verificações semânticas devem ser feitas e a validação final do projeto.
