#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <thread>
#include <vector>

// TBB Includes
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/global_control.h>
#include <tbb/parallel_for.h>

using namespace std;

// código Sequencial
vector<int> sequentialVersion(vector<int> &imagem, int maxIter) {
  // Percorre cada linha da imagem
  for (int y = 0; y < 1080; y++) {
    // Mapeia a coordenada Y
    double c_imag = -1.2 + (double)y / 1080.0 * (1.2 - (-1.2));

    // Percorre cada coluna da imagem
    for (int x = 0; x < 1920; x++) {
      // Mapeia a coordenada X
      double c_real = -2.0 + (double)x / 1920.0 * (1.0 - (-2.0));

      double zx = 0.0;
      double zy = 0.0;
      int iter = 0;

      // Executa a fórmula de Mandelbrot:
      while (zx * zx + zy * zy <= 4.0 && iter < maxIter) {
        double temp_zx = zx * zx - zy * zy + c_real;
        zy = 2.0 * zx * zy + c_imag;
        zx = temp_zx;
        iter++;
      }

      // Salva o número de iterações
      imagem[y * 1920 + x] = iter;
    }
  }

  return imagem;
}

// código que cada thread irá fazer
void worker(vector<int> &imagem, atomic<int> &proximaLinha, int maxIter) {
  while (true) {
    int y = proximaLinha.fetch_add(1);

    if (y >= 1080)
      break;

    double c_imag = -1.2 + (double)y / 1080.0 * (1.2 - (-1.2));

    // calculando o pixel.
    for (int x = 0; x < 1920; x++) {
      double c_real = -2.0 + (double)x / 1920.0 * (1.0 - (-2.0));

      double zx = 0.0;
      double zy = 0.0;
      int iter = 0;

      // roda o algoritmo de Mandelbrot para o pixel (x, y)
      while (zx * zx + zy * zy <= 4.0 && iter < maxIter) {
        double temp_zx = zx * zx - zy * zy + c_real;
        zy = 2.0 * zx * zy + c_imag;
        zx = temp_zx;
        iter++;
      }

      // salva o número de iterações calculado para o pixel
      imagem[y * 1920 + x] = iter;
    }
  }
}

// código inicializador da abordagem std::thread
vector<int> stdThreads(vector<int> &imagem, int qtdThreads, int maxIter) {
  atomic<int> proximaLinha(0);

  vector<thread> threads(qtdThreads);

  for (int i = 0; i < qtdThreads; i++) {
    threads[i] = thread(worker, ref(imagem), ref(proximaLinha), maxIter);
  }

  for (int i = 0; i < qtdThreads; i++) {
    threads[i].join();
  }

  return imagem;
}

// salvar o vetor em imagem .ppm
void salvarImagemPPM(const string &filename, const vector<int> &imagem,
                     int width, int height, int maxIter) {
  ofstream file(filename);
  file << "P3\n" << width << " " << height << "\n255\n";

  for (int i = 0; i < width * height; i++) {
    int iter = imagem[i];

    // Se pertence ao conjunto (1000 iter), pinta de preto (0,0,0)
    // Se escapou, gera um tom baseado nas iterações
    if (iter == maxIter) {
      file << "0 0 0 ";
    } else {
      int r = (iter * 5) % 256;
      int g = (iter * 7) % 256;
      int b = (iter * 11) % 256;
      file << r << " " << g << " " << b << " ";
    }
  }
}

// código inicializador da abordagem OpenMP
vector<int> openmpThreads(vector<int> &imagem, int qtdThreads, int maxIter) {
  #pragma omp parallel for num_threads(qtdThreads) schedule(dynamic)
  for (int y = 0; y < 1080; y++) {
    double c_imag = -1.2 + (double)y / 1080.0 * (1.2 - (-1.2));

    for (int x = 0; x < 1920; x++) {
      double c_real = -2.0 + (double)x / 1920.0 * (1.0 - (-2.0));

      double zx = 0.0;
      double zy = 0.0;
      int iter = 0;

      while (zx * zx + zy * zy <= 4.0 && iter < maxIter) {
        double temp_zx = zx * zx - zy * zy + c_real;
        zy = 2.0 * zx * zy + c_imag;
        zx = temp_zx;
        iter++;
      }

      imagem[y * 1920 + x] = iter;
    }
  }

  return imagem;
}

// código inicializador da abordagem Intel TBB
vector<int> tbbThreads(vector<int> &imagem, int qtdThreads, int maxIter) {
  // Define o número de threads/concorrência no TBB
  tbb::global_control global_limit(tbb::global_control::max_allowed_parallelism,
                                   qtdThreads);

  // tbb::parallel_for dividindo as linhas (0 a 1080) e colunas (0 a 1920) em
  // blocos 2D
  tbb::parallel_for(
      tbb::blocked_range2d<int, int>(0, 1080, 0, 1920),
      [&](const tbb::blocked_range2d<int, int> &r) {
        for (int y = r.rows().begin(); y != r.rows().end(); ++y) {
          double c_imag = -1.2 + (double)y / 1080.0 * (1.2 - (-1.2));

          for (int x = r.cols().begin(); x != r.cols().end(); ++x) {
            double c_real = -2.0 + (double)x / 1920.0 * (1.0 - (-2.0));

            double zx = 0.0;
            double zy = 0.0;
            int iter = 0;

            while (zx * zx + zy * zy <= 4.0 && iter < maxIter) {
              double temp_zx = zx * zx - zy * zy + c_real;
              zy = 2.0 * zx * zy + c_imag;
              zx = temp_zx;
              iter++;
            }

            imagem[y * 1920 + x] = iter;
          }
        }
      });

  return imagem;
}

int main() {
  int qtdThreads = 4;
  int maxIter = 1000;

  // 0. Executa Sequencial
  vector<int> imagemSeq(1920 * 1080, 0);
  auto inicioSeq = chrono::high_resolution_clock::now();
  sequentialVersion(imagemSeq, maxIter);
  auto fimSeq = chrono::high_resolution_clock::now();

  chrono::duration<double, milli> tempoTotalSeq = fimSeq - inicioSeq;
  cout << "Tempo Sequencial  (1 thread): " << tempoTotalSeq.count() << " ms" << endl;

  salvarImagemPPM("sequencial.ppm", imagemSeq, 1920, 1080, maxIter);

  // 1. Executa com std::thread
  vector<int> imagemStd(1920 * 1080, 0);
  auto inicioStd = chrono::high_resolution_clock::now();
  stdThreads(imagemStd, qtdThreads, maxIter);
  auto fimStd = chrono::high_resolution_clock::now();

  chrono::duration<double, milli> tempoTotalStdThread = fimStd - inicioStd;
  cout << "Tempo std::thread (" << qtdThreads
       << " threads): " << tempoTotalStdThread.count() << " ms" << endl;

  salvarImagemPPM("std_thread.ppm", imagemStd, 1920, 1080, maxIter);

  // 2. Executa com OpenMP
  vector<int> imagemOmp(1920 * 1080, 0);
  auto inicioOmp = chrono::high_resolution_clock::now();
  openmpThreads(imagemOmp, qtdThreads, maxIter);
  auto fimOmp = chrono::high_resolution_clock::now();

  chrono::duration<double, milli> tempoTotalOmp = fimOmp - inicioOmp;
  cout << "Tempo OpenMP      (" << qtdThreads
       << " threads): " << tempoTotalOmp.count() << " ms" << endl;

  salvarImagemPPM("openmp_thread.ppm", imagemOmp, 1920, 1080, maxIter);

  // 3. Executa com Intel TBB
  vector<int> imagemTbb(1920 * 1080, 0);
  auto inicioTbb = chrono::high_resolution_clock::now();
  tbbThreads(imagemTbb, qtdThreads, maxIter);
  auto fimTbb = chrono::high_resolution_clock::now();

  chrono::duration<double, milli> tempoTotalTbb = fimTbb - inicioTbb;
  cout << "Tempo Intel TBB   (" << qtdThreads
       << " threads): " << tempoTotalTbb.count() << " ms" << endl;

  salvarImagemPPM("tbb_thread.ppm", imagemTbb, 1920, 1080, maxIter);

  return 0;
}
