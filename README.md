# Compilador MiniVisualg

## 📌 Descrição do Projeto
O projeto consiste na implementação integral do **Front-end (Análise Léxica e Análise Sintática)** de um compilador desenvolvido do zero em linguagem C. O compilador foi construído para processar e validar o "MiniVisualg", um subconjunto da linguagem didática e procedural *Visualg* (sintaxe em português). 

Nenhuma ferramenta geradora de autômatos (como *Lex* ou *Yacc*) foi utilizada. A máquina de estados, tokenização (Scanner) e as regras da gramática preditiva (Parser Descendente Recursivo) foram programadas manualmente, com foco em alta eficiência e execução sob demanda de memória.

## ⚙️ Funcionalidades do Projeto
* **Análise Léxica Contínua:** Leitura e separação inteligente de tokens estruturais e relacionais (identificação de operadores duplos via *lookahead*, como `<-` e `<>`).
* **Análise Sintática Top-Down:** Validação da estrutura LL(1), validando blocos, loops infinitos, matrizes unidimensionais e sub-rotinas com múltiplos parâmetros.
* **Avaliação Matemática em Cascata:** Garantia de precedência matemática isolada através de recursão em andares hierárquicos (Expressão > Termo > Fator).
* **Filtro de Ruído:** Identificação e descarte automático de comentários (`//`), garantindo que o analisador foque puramente em código útil.
* **Geração de Tabela de Símbolos:** Emissão em tempo real e criação do arquivo `saida_tokens.txt` contendo as linhas e naturezas atômicas consumidas no algoritmo processado.

## 🧪 Testes de Software
O compilador foi forjado sobre uma rigorosa bateria de 14 testes estruturais, alcançando sucesso em 100% dos escopos.
* **Testes de Funcionamento:** Validação funcional de atribuição mista de variáveis lógicas e primitivas, *nested-ifs* (Se-Senão), Laços (*Enquanto* e *Para* com *Passo*), Arrays/Vetores e Retornos Funcionais Matemáticos.
* **Testes de Depuração:** Implementação da trava **EOF Lock**. O teste consistiu em submeter sujeira e códigos vazados após a palavra chave obrigatória final (`fimalgoritmo`). O compilador blindou o falso positivo declarando erro sintático imediato de finalização imperfeita.
* **Testes Léxicos (Edge-cases):** Identificadores contendo acentos que tentaram burlar o `isalpha()` resultaram apropriadamente em falha léxica (comportamento desejado), com os acentos sendo estritamente abertos e permitidos internamente dentro de strings limitadas por aspas.

## 💻 Tecnologias e Linguagens
* C

## 📚 Bibliotecas e Frameworks
* **Zero dependências externas.** O sistema foi integralmente concebido utilizando apenas as bibliotecas "built-in" nativas do C (`stdio.h`, `stdlib.h`, `string.h` e `ctype.h`).

## 🛠️ Pré-requisitos e Instalação
Para utilizar este projeto você precisará apenas do compilador GCC instalado na sua máquina (via **MinGW** no Windows, ou nativamente no Mac/Linux).

Para instalar, clone o repositório na sua máquina local:
```bash
git clone https://github.com/SeuUsuario/Projeto-Compiladores.git
```

## 🚀 Instruções de Uso
Abra o terminal dentro da pasta do repositório clonado e compile o programa raiz:
```bash
gcc -Wall -Wno-unused-result -g -Og compilador.c -o compilador
```

Para rodar, chame o executável recém gerado passando o nome do arquivo que contém o algoritmo que deseja validar:
```bash
# Se utilizar Windows:
.\compilador.exe arquivo_de_exemplo.txt

# Se utilizar Mac / Linux:
./compilador arquivo_de_exemplo.txt
```
O console acusará erro sintático (com a linha e o contexto do erro) caso a linguagem seja violada, ou exibirá "Compilação concluída com sucesso". 

## 📖 Documentação (Resumo da Arquitetura)

O código-fonte (`compilador.c`) foi estruturado em três grandes blocos, separando claramente as responsabilidades de leitura e validação:

### 1. Estruturas de Dados e Tipagem (`structs` e `enums`)
No topo do código, definimos os moldes de comunicação do compilador. 
* **`TokenNome`**: Classifica as categorias gerais (Identificadores, Números, Operadores e Palavras Reservadas). 
* **`Token`**: A estrutura principal. O destaque aqui é o uso de `union` para guardar o atributo específico do token (seja um valor inteiro, real ou um código de operador). O uso da `union` faz com que o programa utilize o mesmo espaço de memória para valores excludentes, otimizando o gasto de RAM durante a execução.

### 2. Analisador Léxico (Scanner)
A função `obterToken()` age como uma máquina de estados que lê o código-fonte (previamente carregado na memória) caractere por caractere.
* **Filtro:** Ignora automaticamente espaços em branco, quebras de linha e comentários.
* **Formação de Lexemas:** Agrupa os caracteres válidos em palavras e decide se são variáveis (IDs), números ou palavras reservadas da linguagem.
* **Lookahead de 1 caractere:** Resolve colisões de símbolos. Por exemplo, ao ler `<`, o léxico "olha" o próximo caractere. Se for `-`, ele entende como o operador de atribuição `<-`. Se for `=`, entende como menor ou igual `<=`. Se não for nenhum dos dois, entende que é apenas o operador menor `<`.

### 3. Analisador Sintático (Parser Descendente Recursivo LL(1))
É o coração lógico do sistema. Ele não processa o arquivo inteiro de uma vez; ele chama o Léxico **sob demanda** (via `nextToken()`) perguntando qual é a próxima palavra e verificando se ela obedece à gramática.
* **Cascata de Estruturas:** Inicia pela função `analisa_algoritmo_principal()`, que "consome" obrigatoriamente a palavra `algoritmo`, depois chama as funções que validam declarações de variáveis/sub-rotinas, entra no `inicio` e finalmente encerra com `fimalgoritmo`.
* **Precedência Matemática:** As regras de matemática são validadas através de chamadas recursivas rígidas. Para evitar que `1 + 2 * 3` some antes de multiplicar, a hierarquia desce na ordem: `Expressão Relacional` > `Expressão Simples` (Somas) > `Termo` (Multiplicações) > `Fator` (Números ou Parênteses). O compilador avalia a multiplicação antes porque o Parser mergulha na árvore do `Fator` primeiro.
* **EOF Lock:** Após todo o código ser validado e o `fimalgoritmo` consumido na `main()`, o sistema faz uma checagem final obrigatória de `EOF` (Fim de Arquivo), barrando qualquer código "sujo" que o usuário possa ter digitado ao final do arquivo de texto.

## 📄 Licença
Este projeto possui licença de **Uso Educacional e Acadêmico**, não voltado a aplicações comerciais.


## 👥 Contribuidores
Projeto desenhado, estudado e codificado por Guilherme Limeira de Souza, Gustavo Kiyoshi Ikeda e Pedro Montarroyos de Pinho. 
Agradecimentos especiais à professora Daniela Vieira Cunha e aos materiais base da disciplina de Compiladores!