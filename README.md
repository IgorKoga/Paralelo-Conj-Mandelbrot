# Cálculo Paralelo do Conjunto de Mandelbrot

Este projeto implementa a geração do conjunto de Mandelbrot em C++, explorando e comparando 4 diferentes abordagens de paralelização exigidas no trabalho: **Sequencial**, **std::thread**, **OpenMP** e **Intel TBB**.

O Conjunto de Mandelbrot foi escolhido por gerar um resultado visual e por apresentar um forte desafio de **Desbalanceamento de Carga (Load Imbalance)**: pixels próximos ou dentro do conjunto (escuros) exigem o número máximo de iterações, enquanto pixels distantes exigem muito poucas. 

Abaixo detalhamos como cada abordagem lida com esse desafio.

---

## 1. Abordagem Sequencial
Na versão sequencial, o programa varre os 1920x1080 pixels (linha por linha, coluna por coluna) utilizando um único fluxo de execução (1 núcleo da CPU). É a abordagem mais lenta, pois o trabalho não é dividido, mas serve como *baseline* (base de tempo) obrigatória para o cálculo do **Speedup** ($S = \text{Tempo Sequencial} / \text{Tempo Paralelo}$) das demais versões.

## 2. Abordagem com `std::thread` (Fila Dinâmica)
Para evitar que uma divisão estática de linhas sobrecarregasse a thread responsável pelo centro geográfico do fractal (a área mais densa e pesada), implementamos uma **Fila Dinâmica de Trabalho**.
- Utilizamos um contador atômico `std::atomic<int> proximaLinha(0)`.
- Cada thread ativa dentro do grupo busca a próxima linha disponível sob demanda, invocando `fetch_add(1)`.
- Com isso, se uma thread pegar uma linha extremamente custosa, as demais threads continuam distribuindo e concluindo as linhas leves restantes sem ficar aguardando, balanceando a carga perfeitamente entre os núcleos.

## 3. Abordagem com OpenMP
O OpenMP paraleliza blocos iterativos (`for`) através de diretivas pragmas injetadas no código. Para resolver a variação de carga do fractal de forma otimizada, utilizamos:
```cpp
#pragma omp parallel for num_threads(qtdThreads) schedule(dynamic)
```
- A cláusula `schedule(dynamic)` instrui o escalonador interno do OpenMP a agir exatamente como uma fila dinâmica, distribuindo as linhas do laço principal sob demanda para as threads conforme elas se liberam, em oposição à divisão estática (`schedule(static)`).

## 4. Abordagem com Intel TBB (2D Tiling & Work Stealing)
Com a biblioteca Intel Threading Building Blocks, foi possível explorar o uso do `tbb::blocked_range2d` (particionamento bidimensional) ao invés do tradicional fatiamento por linhas de matriz.

### Vantagens técnicas aplicadas no TBB:
1. **Subdivisão em Ladrilhos (Tiling 2D):**
   Ao passar `tbb::blocked_range2d<int, int>(0, 1080, 0, 1920)`, a biblioteca subdivide o processamento da imagem em pequenos blocos retangulares iterativos (ladrilhos ou *tiles*) e não em grandes faixas horizontais de 1920 colunas.
2. **Roubo de Trabalho (Work Stealing):**
   O agendador de tarefas do Intel TBB é regido nativamente por *work-stealing*. Caso uma thread finalize todo o processamento de sua lista de ladrilhos (ex: ladrilhos da borda vazia da imagem), ela acessa a fila de threads mais ocupadas e "rouba" os ladrilhos não processados da região central do Mandelbrot. Isso garante ocupação total dos núcleos lógicos da máquina durante toda a etapa de renderização.

---

## Como Compilar e Executar (Linux)

Para compilar o código vinculando as bibliotecas essenciais para as 4 versões (OpenMP, Intel TBB e Pthreads nativo) garantindo otimização de tempo real de máquina (`-O3`), rode no terminal:

```bash
g++ -std=c++17 -O3 main.cpp -o benchmark -fopenmp -ltbb -pthread
```

Após o build gerar o executável, rode o programa com:

```bash
./benchmark
```

*O programa rodará as quatro abordagens sequencialmente, imprimirá os respectivos milissegundos na tela, e salvará arquivos .ppm (ex: sequencial.ppm, std_thread.ppm, etc) correspondentes à imagem final gerada.*
