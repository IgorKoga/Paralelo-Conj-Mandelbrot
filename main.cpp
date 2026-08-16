#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <fstream>
#include <atomic>

using namespace std;

// código que cada thread irá fazer
void worker(vector<int> &imagem, atomic<int> &proximaLinha, int maxIter){
    while(true)
    {
        int y = proximaLinha.fetch_add(1);

        if(y >= 1080) break;

        double c_imag = -1.2 + (double)y / 1080.0 * (1.2 - (-1.2));

        // calculando o pixel.
        for(int x = 0; x < 1920; x++){
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
vector<int> stdThreads(vector<int> &imagem, int qtdThreads, int maxIter){
    atomic<int> proximaLinha(0);

    vector<thread> threads(qtdThreads);

    for(int i = 0; i < qtdThreads; i++){
        threads[i] = thread(worker, ref(imagem), ref(proximaLinha), maxIter);
    }
    
    for(int i = 0; i < qtdThreads; i++){
        threads[i].join();
    }

    return imagem;
}

// salvar o vetor em imagem .ppm
void salvarImagemPPM(const string& filename, const vector<int>& imagem, int width, int height, int maxIter) {
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

int main(){
    int qtdThreads = 4;
    int maxIter = 1000;
    vector<int> imagem(1920 * 1080, 0);

    auto inicio = chrono::high_resolution_clock::now();
    vector<int> imagemSaida = stdThreads(imagem, qtdThreads, maxIter);
    auto fim = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> tempoTotalStdThread
     = fim - inicio;

    cout << "Tempo std::thread (" << qtdThreads << " threads): " << tempoTotalStdThread.count() << " ms" << endl;

    salvarImagemPPM("std_thread.ppm", imagemSaida, 1920, 1080, maxIter);

    return 0;
}
