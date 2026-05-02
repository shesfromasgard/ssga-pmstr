#include "ObjectiveFunction.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <cstdlib>
#include "Operation.h"
#include <climits>
#include <limits>

using namespace std;

int maquinaPai(int idJob, const std::vector<std::vector<Operation>> &maquina) {
   for (int i = 0; i < maquina.size(); ++i) {
        auto it = std::find_if(maquina[i].begin(), maquina[i].end(), [idJob](const Operation& op) {
            return op.idJob == idJob;
        });

        if (it != maquina[i].end()) {
            return i;
        }
   }
   return -1;
}

double objectiveFunction(const std::vector<std::vector<Operation>> &maquina, const std::vector<Operation> &vetOperacoes, const std::map<int, std::map<int, int>> &controleOp, std::vector<double> &tardiness_maq)
{
    if (maquina.empty()) {
        return 0.0;
    }

    if (vetOperacoes.size() != o) {
        return INT_MAX;
    }

    // conferindo a regra de precedência na mesma máquina de uma vez
    for (int i = 0; i < m; ++i) {
        std::map<int, int> ultimaOpDoJob;
        for (const auto &op : maquina[i]) {
            if (ultimaOpDoJob.count(op.idJob) && ultimaOpDoJob[op.idJob] > op.idOp) {
                return INT_MAX;
            }
            ultimaOpDoJob[op.idJob] = op.idOp;
        }
    }

    double wd = 1.0;  // Peso do atraso
    double ws = 1.0;  // Peso da troca
    double tau = 1.0; // Tempo de troca (1 hora)

    int setups = 0;
    double tardiness = 0.0;
    vector<double> tempo(m, 0.0);
    tardiness_maq.resize(m, 0.0);

    vector<int> u(m, 0); // quantidade de ferramentas atualmente carregadas
    vector<vector<int>> carregados(m, vector<int>(t, 0));
    vector<vector<vector<int>>> magazine(m, vector<vector<int>>(t, vector<int>())); // magazine de presença de ferramentas
    vector<vector<vector<int>>> prioridades(m, vector<vector<int>>(t, vector<int>()));

    vector<int> setsSize(t);
    int maxJobId = 0;
    int maxOpId = 0;

    for (const auto &op : vetOperacoes) {
        if (op.idJob > maxJobId)
            maxJobId = op.idJob;
        if (op.idOp > maxOpId)
            maxOpId = op.idOp;
    }

    std::vector<std::vector<double>> tempo_final(maxJobId + 1,
                                                 std::vector<double>(maxOpId + 1, std::numeric_limits<double>::max()));

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < maquina[i].size(); j++) {
            setsSize[maquina[i][j].toolSetId] = maquina[i][j].toolSetSize;
        }
    }

    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < t; j++) {
            for (int k = 0; k < maquina[i].size(); k++) {
                magazine[i][j].push_back(0);
                prioridades[i][j].push_back(0);
            }
        }
    }

    // Para cada máquina, para cada ferramenta usada pela operacao em cada estágio
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < maquina[i].size(); ++j) {
            magazine[i][maquina[i][j].toolSetId][j] = 1;
        }
    }

    for (int k = 0; k < m; ++k) {
        for (unsigned i = 0; i < t; ++i) {
            for (unsigned j = 0; j < maquina[k].size(); ++j) {
                if (magazine[k][i][j] == 1) // se o estágio atual precisa daquela ferramenta, prioridade zero
                    prioridades[k][i][j] = 0;
                else {
                    int proxima = 0;
                    bool usa = false;
                    for (unsigned l = j + 1; l < maquina[k].size(); ++l) {
                        ++proxima;
                        if (magazine[k][i][l] == 1) {
                            usa = true;
                            break;
                        }
                    }
                    if (usa)
                        prioridades[k][i][j] = proxima;
                    else
                        prioridades[k][i][j] = -1;
                }
            }
        }
    }

    for (int j = 0; j < m; ++j) {
        for (int i = 0; i < maquina[j].size(); i++) {
            if (carregados[j][maquina[j][i].toolSetId] == 0 && u[j] + maquina[j][i].toolSetSize < c) {
                u[j] += maquina[j][i].toolSetSize;
                carregados[j][maquina[j][i].toolSetId] = 1;
            }
        }
    }

    int max_op = 0;
    for (int i = 0; i < maquina.size(); ++i) {
        if (maquina[i].size() > max_op)
            max_op = maquina[i].size();
    }

    int idxPorMaquina[maquina.size()] = {0};
    vector<int> esperandoPorQuem(maquina.size(), -1);

    bool acabei = false;
    while (!acabei) {
        acabei = true;
        for (int i = 0; i < maquina.size(); ++i) {
            if (idxPorMaquina[i] < maquina[i].size()) {

                acabei = false;
                break;
            }
        }
        if (acabei) {
            break;
        }

        for (int i = 0; i < maquina.size(); i++) {
            int j = idxPorMaquina[i];

            if (j >= maquina[i].size())
                continue;

            int idJob = maquina[i][j].idJob;
            int idOp = maquina[i][j].idOp;
            bool troca = false;

            double tempoPai = 0.0;
            if (idOp > 1) {
                tempoPai = tempo_final[idJob][idOp - 1];

                if (tempoPai >= std::numeric_limits<double>::max()) {
                    esperandoPorQuem[i] = maquinaPai(maquina[i][j].idJob, maquina);
                    vector<int> estado(esperandoPorQuem.size(), 0);
                    
                    for (int z = 0; z < esperandoPorQuem.size(); z++) {
                        if (estado[z] ==0){
                            int atual = z;
                            while (atual != -1 && estado[atual]!=2){
                                if (estado[atual] == 1) {
                                    return INT_MAX;
                                }
                                estado[atual] = 1;
                                atual = esperandoPorQuem[atual];
                            }
                            atual = z;
                            while(atual != -1 && estado[atual]==1) {
                                estado[atual] = 2;
                                atual = esperandoPorQuem[atual];
                            }
                        }    
                    }

                    continue;
                }
            }

            idxPorMaquina[i]++;

            // Cálculo do tempo considerando a espera
            tempo[i] = std::max({tempo[i], tempoPai, (double)maquina[i][j].releaseTime});

            if (carregados[i][maquina[i][j].toolSetId] == 0) {
                u[i] += maquina[i][j].toolSetSize;
                carregados[i][maquina[i][j].toolSetId] = 1; // carrega a ferramenta necessária para a operação atual
                troca = true;

                while (u[i] > c) {
                    int maior = 0;
                    int pMaior = -1;

                    for (unsigned k = 0; k < t; ++k) {
                        if (magazine[i][k][j] != 1) {
                            if ((carregados[i][k] == 1) && (prioridades[i][k][j] == -1)) {
                                pMaior = k;
                                break;
                            }
                            else {
                                if ((prioridades[i][k][j] > maior) && carregados[i][k] == 1) {
                                    maior = prioridades[i][k][j];
                                    pMaior = k;
                                }
                            }
                        }
                    }
                    if (pMaior == -1) {
                        return INT_MAX;
                    }
                    carregados[i][pMaior] = 0;
                    u[i] -= setsSize[pMaior];
                }
            }
            if (troca) {
                ++setups;
                tempo[i] += tau;
            }

            tempo[i] += maquina[i][j].processingTime; // processa a operação (adiciona o processing time)

            tempo_final[idJob][idOp] = tempo[i];

            if (tempo[i] > maquina[i][j].dueDate) {
                double atraso = tempo[i] - maquina[i][j].dueDate;
                tardiness_maq[i] += atraso; // Acumula na máquina específica
                tardiness += atraso;
            }
        }
    }

    double resultado = (wd * tardiness) + (ws * tau * setups);
    return resultado;
}