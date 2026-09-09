# SSGA-PMSTR

**Steady-State Genetic Algorithm** para o problema de **Escalonamento em Máquinas Paralelas Idênticas com Reentrada e Restrições de Ferramentas** (PMSTR).

> Código-fonte e instâncias utilizados no artigo *"Escalonamento em Máquinas Paralelas Idênticas com Reentrada e Restrições de Ferramentas: Uma Abordagem Evolutiva"*, submetido ao **LVIII Simpósio Brasileiro de Pesquisa Operacional (SBPO 2026)**.

---

## Visão Geral

O problema abordado envolve o escalonamento de operações em máquinas paralelas idênticas onde:

- **Jobs** possuem múltiplas operações com relações de **precedência** estritas (reentrada: uma operação de um job pode retornar ao centro de trabalho).
- Cada operação exige um **conjunto de ferramentas** específico, cujo tamanho é sempre igual ou menor a capacidade do magazine.
- A troca de ferramentas gera um **tempo de setup** (τ = 1 hora).
- A função objetivo minimiza uma combinação ponderada de **tardiness** e **custo de setup**:

$$f = w_d \cdot \text{tardiness} + w_s \cdot \tau \cdot \text{setups}$$

A heurística de troca de ferramentas implementada é baseada na política **KTNS (Keep Tool Needed Soonest)**: quando o magazine excede sua capacidade, descarta-se primeiro a ferramenta que nunca mais será utilizada na sequência e, caso não haja, aquela cuja próxima utilização é mais distante — mantendo sempre as ferramentas necessárias mais cedo.

---

## Estrutura do Repositório

```
SSGA-PMSTR/
├── samplecode.cpp        # Ponto de entrada do algoritmo (leitura da instância, execução do SSGA, escrita da saída)
├── SSGA.h                # Algoritmo genético steady-state com busca local integrada
├── SampleDecoder.h       # Codificação/decodificação do cromossomo e construção da solução inicial
├── ObjectiveFunction.cpp # Função objetivo: tardiness + custo de setup com política NTF
├── ObjectiveFunction.h   # Header da função objetivo
├── Operation.h           # Estrutura de dados de uma operação (job, toolset, tempos)
├── Operation.cpp         # Implementação do construtor de Operation
├── run.cpp               # Runner em lote: múltiplas instâncias e repetições independentes
├── Makefile              # Build com g++-14, -O3, C++11
└── instancias/           # Instâncias de benchmark
    ├── 2M38/             # 2 máquinas, ~38 operações
    ├── 2M46/             # 2 máquinas, ~46 operações
    ├── 6M140/            # 6 máquinas, ~140 operações
    └── 6M163/            # 6 máquinas, ~163 operações
```

---

## Algoritmo

### SSGA — Steady-State Genetic Algorithm

O algoritmo mantém uma **população de tamanho fixo** e substitui apenas um indivíduo por vez (steady-state), sem necessidade de gerações completas.

| Componente              | Detalhes |
|-------------------------|----------|
| **Representação**       | Cromossomo de reais em \[0, $m$\) onde `m` é a quantidade de máquinas o índice da máquina atribuída à operação; a ordem relativa dos decimais dentro do intervalo define a sequência na máquina |
| **Inicialização**       | Heurística construtiva gulosa: randomiza a ordem de jobs e atribui cada operação à máquina com menor tempo de conclusão previsto |
| **Seleção**             | Torneio binário (dois candidatos aleatórios; vence o de menor fitness) |
| **Crossover**           | Uniforme: cada gene é herdado aleatoriamente de um dos dois pais |
| **Mutação**             | Por posição com probabilidade `MUT_GENE_PROB` (substituição por valor aleatório em \[0,1\)) |
| **Substituição**        | O filho substitui o **pior** indivíduo da população (se seu fitness não for pior que o do pior atual) |
| **Critério de parada**  | Limite de tempo (7200 s) **ou** `MAX_VOID` iterações consecutivas sem melhoria global

---

## Parâmetros do Algoritmo

Configurados no topo de [`samplecode.cpp`](samplecode.cpp) e calibrados automaticamente com **iRace**:

| Parâmetro         | Variável        | Default   | Descrição |
|-------------------|-----------------|-----------|-----------|
| Tamanho da pop.   | `POP_SIZE`      | `15`      | Multiplicado pelo nº de operações da instância (`o × POP_SIZE`) |
| Iter. sem melhora | `MAX_VOID`      | `95000`   | Critério de parada por estagnação |
| Prob. mutação     | `MUT_GENE_PROB` | `0.03`    | Probabilidade de mutação por gene |
| Limite de tempo   | —               | `7200 s`  | Tempo máximo de execução por instância |

---

## Formato das Instâncias

Cada arquivo de instância segue o formato texto:

```
o, <número de operações>
m, <número de máquinas>
t, <número de conjuntos de ferramentas>
c, <capacidade do magazine>
<linha em branco>
<cabeçalho>
idJob  idOp  releaseTime  processingTime  dueDate  toolSetId  toolSetSize
...
```

---

## Compilação

Requer **GCC 14** e o pacote `build-essential`. O código é compatível apenas com **Linux**.

```bash
make
```

Isso gera dois executáveis:
- **`samplecode`** — executa o SSGA em uma única instância.
- **`run`** — executa em lote sobre um diretório de instâncias.

---

## Execução

### Instância única

```bash
./samplecode <arquivo_de_saida> < <arquivo_de_instancia>
```

### Lote (múltiplas instâncias e repetições)

Recomendado para experimentos. O `run` itera sobre todos os arquivos de uma pasta, executa `samplecode` para cada par (instância, repetição) e gera um resumo consolidado em CSV.

```bash
nohup ./run <pasta_com_instancias> <número_de_execuções_por_instância> &
```

**Exemplo:**

```bash
nohup ./run instancias/2M38/ 10 &
```

O runner:
1. Cria o subdiretório `<pasta>/solucoes/` automaticamente.
2. **Pula** execuções já concluídas (verifica se o arquivo de saída existe e não está vazio).
3. Ao final, gera `<pasta>/solucoes/RESUMO.csv` com as colunas:

| Coluna              | Descrição |
|---------------------|-----------|
| `Arquivo`           | Nome do arquivo de saída |
| `Makespan_Inicial`  | Fitness da solução inicial gerada pela heurística construtiva |
| `Melhor_Makespan`   | Melhor valor de função objetivo encontrado |
| `Tempo_Execucao(s)` | Tempo total de execução em segundos |
| `Iteracoes`         | Total de gerações executadas |


### Debug de progresso

Para imprimir o progresso iteração a iteração no `stdout`:

```bash
export PMSTR_DEBUG_PROGRESS=1
./samplecode saida.txt < instancia.txt
```

---

## Saída por Execução

Cada chamada ao `samplecode` gera um arquivo `.out` contendo:

- **Linha de resumo** (parseável pelo `run`): `<makespan_inicial> <melhor_makespan> <tempo_s> <iterações>`
- Estatísticas do SSGA: filhos gerados, rejeitados (por invalidade ou por serem piores), inseridos
- Parâmetros da execução e da instância
- Sequência de tarefas por máquina da melhor solução encontrada

---

## Requisitos

| Requisito    | Especificação |
|--------------|---------------|
| Sistema Operacional | Linux |
| Compilador   | GCC 14 (`g++-14`) |
| Padrão C++   | C++11 |
| Dependências | `build-essential` |

---

## Contato

Dúvidas e perguntas podem ser enviadas para: [anaclpereirasouz@gmail.com](mailto:anaclpereirasouza@gmail.com)
