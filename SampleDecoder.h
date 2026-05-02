#ifndef SAMPLEDECODER_H
#define SAMPLEDECODER_H

#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <climits>
#include <random>
#include "ObjectiveFunction.h"

#define maxsize 4501

using namespace std;

extern int m, o, t, c;
extern vector<Operation> vetOperacao;

class SampleDecoder {
public:
	SampleDecoder()  { }
	~SampleDecoder() { }

	double decode(vector<double>& chromosome) const{
		vector<vector<Operation>> maquina(m, vector<Operation>());
		vector<vector<pair<double,unsigned>>> ranking (m, vector<pair<double,unsigned>>());

		for(unsigned i = 0; i < chromosome.size(); ++i) {
			int mid = (int)floor(chromosome[i]);
			if (mid < 0 || mid >= m) 
				return INT_MAX;
			ranking[mid].push_back(pair<double,unsigned>(chromosome[i], i));
		}
		
		for(int i = 0; i < m; ++i)
			sort(ranking[i].begin(), ranking[i].end());

		for(int i = 0; i < m; ++i) {
			for(auto& slaPorra : ranking[i]) {
				maquina[i].push_back(vetOperacao[slaPorra.second]);
			}
		}

		for (int i = 0; i < m; ++i) {
			restricaoPrecedencia(maquina[i], chromosome, ranking, i);
		}

		map<int, double> tempo_final;
		vector<double> tardiness_maq;
		map<int, map<int, int>> controleOp;

		return objectiveFunction(maquina, vetOperacao, controleOp, tardiness_maq);
	}

	vector<double> encoder() {

		std::random_device rd;
    	std::mt19937 gen(rd());
    	std::uniform_real_distribution<double> dis(0.0, 1.0);
		
		vector<Operation> operacoes = randomizarOp();
		vector<vector<Operation>> maquinas = atribuirMaquinas(operacoes);
		vector<double> chromossome(o, 0.0);

		for(int i = 0; i < m; ++i) {
			vector<double> tmp(maquinas[i].size());
			for(int j = 0; j < maquinas[i].size(); ++j)
				tmp[j] = dis(gen);

			sort(tmp.begin(), tmp.end());

			for(int j = 0; j < maquinas[i].size(); ++j) {
				for(int k = 0; k < vetOperacao.size(); ++k) {
					if((maquinas[i][j].idJob == vetOperacao[k].idJob) && (maquinas[i][j].idOp == vetOperacao[k].idOp)) {
						chromossome[k] = tmp[j] + i;
						break;
					}
				}
			}

		}

		return chromossome;

	}
	
	private:
	static void restricaoPrecedencia(vector<Operation>& ops, vector<double> &chromossome, vector<vector<pair<double,unsigned>>> ranking, int cara) {
		int maxJob = 0;
		
		for(Operation op : ops) {
			if(op.idJob > maxJob)
			maxJob = op.idJob;
		}
		
		vector<vector<pair<int, int>>> controle(maxJob + 1, vector<pair<int, int>>());
		
		for(int i = 0; i < ops.size(); ++i) {
			controle[ops[i].idJob].push_back(pair<int, int>(ops[i].idOp, i));
		}
		
		for(auto& slaPorra : controle)
			sort(slaPorra.begin(), slaPorra.end());
		
		for(int i = 0; i <= maxJob; ++i) {
			vector<int> lugar;
			vector<Operation> banco;
			for(auto& slaPorra : controle[i]) {
				lugar.push_back(slaPorra.second);
				banco.push_back(ops[slaPorra.second]);
			}
			if (lugar.size() <= 1)
				continue;
			sort(lugar.begin(), lugar.end());
			for(int j = 0; j < lugar.size(); ++j) {
				ops[lugar[j]] = banco[j];
			}
			vector<int> indexes(banco.size());
			vector<double> valor(banco.size());
			for(int j = 0; j < (int)banco.size(); ++j) {
				indexes[j] = -1;
				for(int k = 0; k < o; ++k) {
					if((vetOperacao[k].idJob == banco[j].idJob) && (vetOperacao[k].idOp == banco[j].idOp)) {
						indexes[j] = k;
						break;
					}
				}
				if (indexes[j] < 0)
					return;
				valor[j] = chromossome[indexes[j]];
			}
			sort(valor.begin(), valor.end());
			for(int j = 0; j < (int)banco.size(); ++j) {
				chromossome[indexes[j]] = valor[j];
			}
		}

	}
	std::vector<Operation> randomizarOp() {
		std::map<int, std::deque<Operation>> tarefas;
		for (const auto &op : vetOperacao) {
			tarefas[op.idJob].push_back(op);
		}
	
		std::vector<Operation> operacoesRandomizadas;
		std::vector<int> tarefasDisponiveis;
	
		for (auto const &[id, fila] : tarefas) {
			tarefasDisponiveis.push_back(id);
		}
	
		std::random_device rd;
		std::mt19937 g(rd());
	
		while (!tarefasDisponiveis.empty()) {
	
			std::uniform_int_distribution<> dis(0, tarefasDisponiveis.size() - 1);
			int idSorteado = dis(g);
			int idJobSorteado = tarefasDisponiveis[idSorteado];
	
			auto &fila = tarefas[idJobSorteado];
			operacoesRandomizadas.push_back(fila.front());
			fila.pop_front();
	
			if (fila.empty())
			{
				tarefasDisponiveis.erase(tarefasDisponiveis.begin() + idSorteado);
			}
		}
	
		return operacoesRandomizadas;
	}
	
	vector<vector<Operation>> atribuirMaquinas(std::vector<Operation> operacoes) {
		vector<vector<Operation>> maquinas(m, vector<Operation>());
	
		std::vector<double> tempoMaq(m, 0.0);
		std::map<int, double> tempoJob;
	
		for (const auto &op : operacoes)
		{
			int melhorMaq = 0;
			double menorTermino = -1.0;
	
			for (int j = 0; j < m; j++)
			{
				double prontoJob = (op.idOp > 1) ? tempoJob[op.idJob] : 0.0;
	
				double inicioPossivel = std::max({tempoMaq[j], prontoJob, (double)op.releaseTime});
				double terminoPrevisto = inicioPossivel + op.processingTime;
	
				if (menorTermino < 0 || terminoPrevisto < menorTermino)
				{
					menorTermino = terminoPrevisto;
					melhorMaq = j;
				}
			}
	
			maquinas[melhorMaq].push_back(op);
	
			double prontoJob = (op.idOp > 1) ? tempoJob[op.idJob] : 0.0;
			double inicioReal = std::max({tempoMaq[melhorMaq], prontoJob, (double)op.releaseTime});
	
			tempoMaq[melhorMaq] = inicioReal + op.processingTime;
			tempoJob[op.idJob] = tempoMaq[melhorMaq];
		}
		return maquinas;
	}
	vector<unsigned> tProcessamento;
};

#endif
