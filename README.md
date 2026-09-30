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

## 📖 Documentação


## 📄 Licença
Este projeto possui licença de **Uso Educacional e Acadêmico**, não voltado a aplicações comerciais.


## 👥 Contribuidores
Projeto desenhado, estudado e codificado por Guilherme Limeira de souza, Gustavo Kiyoshi Ikeda e Pedro Montarroyos de Pinho. 
Agradecimentos especiais à professora Daniela Vieira Cunha e aos materiais base da disciplina de Compiladores!