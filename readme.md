# Hunick Language

Bem-vindo ao Hunick, uma linguagem de programação moderna, compilada e com foco em segurança de memória, inspirada em conceitos de linguagens como C e Rust. Este repositório contém o código-fonte do interpretador para a linguagem Hunick, escrito em C.

## Visão Geral

Hunick é uma linguagem de programação estaticamente tipada que visa fornecer performance e segurança. Seu design incorpora um sistema de verificação de empréstimo (borrow checker) e tempo de vida (lifetimes), garantindo a segurança de memória sem a necessidade de um garbage collector.

Os arquivos de código-fonte da linguagem Hunick utilizam a extensão **`.hk`**.

## Características Principais

  * **Segurança de Memória:** Inspirado no Rust, o Hunick implementa um sistema de *borrowing* e *lifetimes* para garantir que não ocorram erros de acesso à memória, como *null pointer exceptions* ou *data races*.
  * **Tipagem Estática:** Todos os tipos são verificados em tempo de compilação, o que ajuda a detectar erros antes da execução do programa.
  * **Sintaxe Moderna:** A sintaxe é limpa e expressiva, com palavras-chave intuitivas como `let`, `const`, `func`, `if` e `return`.
  * **Sistema de Módulos (Planejado):** O design modular permitirá a criação de bibliotecas e a organização de código em larga escala.
  * **Funções de Primeira Classe:** Funções podem ser tratadas como qualquer outro valor: atribuídas a variáveis, passadas como argumentos e retornadas de outras funções.
  * **Operador Pipe (`|>`):** Facilita a escrita de código no estilo funcional, permitindo encadear chamadas de função de forma legível.

## Como Compilar e Executar

Atualmente, o projeto não possui um script de build automatizado. Para compilar o interpretador, você pode usar um compilador C (como o GCC) e compilar todos os arquivos `.c`:

```bash
gcc -o hunick *.c -lm
```

Depois de compilar, você poderá executar um arquivo de código-fonte Hunick (`.hk`):

```bash
./hunick seu_codigo.hk
```

*Observação: A funcionalidade de ler um arquivo como argumento ainda não foi implementada. Atualmente, o código a ser executado precisa ser inserido diretamente na função `main` do interpretador.*

## Estrutura do Projeto

O interpretador é dividido em módulos, cada um com uma responsabilidade clara no processo de compilação e execução:

  * **`lexer`**: Converte o código-fonte em uma sequência de tokens.
  * **`parser`**: Constrói uma Árvore de Sintaxe Abstrata (AST) a partir dos tokens, respeitando a gramática da linguagem.
  * **`ast`**: Define as estruturas de dados que representam o código na AST, como expressões, declarações e tipos.
  * **`semantic`**: Realiza a análise semântica, incluindo verificação de tipos, análise de escopo e as regras de segurança de memória (borrow checking).
  * **`evaluator`**: Percorre a AST e executa o código, realizando os cálculos e manipulando os objetos da linguagem.
  * **`environment`**: Gerencia o escopo de variáveis e funções durante a execução.
  * **`object`**: Define a representação interna dos tipos de dados da linguagem, como inteiros, strings e funções.

## Roadmap de Desenvolvimento

Aqui está um roadmap sugerido para as próximas grandes implementações no projeto Hunick.

### Roadmap para a Biblioteca Padrão (`stdlib`)

Uma biblioteca padrão robusta é essencial para tornar a linguagem útil. A implementação pode ser dividida em fases:

**Fase 1: Módulos Essenciais**

  * **Módulo `io`:**
      * `print(...)`: Para imprimir texto e outros tipos no console.
      * `println(...)`: Similar ao `print`, mas adiciona uma nova linha.
      * `read_line()`: Para ler input do usuário.
      * Funções para manipulação de arquivos: `open`, `read`, `write`, `close`.
  * **Módulo `string`:**
      * Funções para manipulação de strings: `len`, `trim`, `split`, `contains`, `replace`.
      * Conversão de e para outros tipos (ex: `to_int`, `to_float`).
  * **Módulo `math`:**
      * Constantes: `PI`, `E`.
      * Funções trigonométricas: `sin`, `cos`, `tan`.
      * Funções de potência e raiz: `pow`, `sqrt`.

**Fase 2: Estruturas de Dados**

  * **Módulo `collections`:**
      * `List<T>`: Uma lista dinâmica (vetor) genérica.
      * `HashMap<K, V>`: Uma implementação de tabela hash.
      * `Option<T>`: Para tratamento de valores que podem ser nulos (similar ao `Option` do Rust ou `Optional` do Java).
      * `Result<T, E>`: Para tratamento de erros, encapsulando um sucesso (`T`) ou um erro (`E`).

**Fase 3: Funcionalidades Avançadas**

  * **Módulo `threading`:**
      * Suporte para criação e gerenciamento de threads para programação concorrente.
  * **Módulo `net`:**
      * Funcionalidades básicas de networking para criar clientes e servidores TCP/HTTP.
  * **Módulo `os`:**
      * Interação com o sistema operacional: acesso a variáveis de ambiente, informações do sistema, etc.
-----