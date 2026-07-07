#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <map>
#include <string>
#include <sstream>
#include <deque>
#include <random>
#include <algorithm>
#include <iomanip>

#include "Operation.h"
#include "SampleDecoder.h"
#include "SSGA.h"
#include "ObjectiveFunction.h"

using namespace std;
using namespace std::chrono;

ofstream fileSolution;
extern int m, o, t, c;
extern vector<Operation> vetOperacao;

static inline bool progressDebugEnabled()
{
	const char* env = std::getenv("PMSTR_DEBUG_PROGRESS");
	return env != nullptr && env[0] != '\0' && env[0] != '0';
}

// ----------------------- cabecalho das instancias -----------------------
// m = numero de maquinas
// o = numero de operacoes
// t = numero de conjuntos de ferramentas
// c = capacidade do magazine
// ------------------------------------------------------------------------

// ---------------- Parâmetros do iRace ----------------
unsigned POP_SIZE = 15; // p
unsigned MAX_VOID = 95000; // iterações sem melhoria
double MUT_GENE_PROB = 0.03;
double INTENS_PROB = 0.03; // taxa de intensificação (busca local)
bool TWO_SWAP = false; // swap2
bool INSERTION = false; // insertion
bool TWO_OPT = false; // opt2
// -----------------------------------------------------

unsigned generation = 0;

unsigned nVoid = 0;
vector<vector<Operation>> maquina;
// fileSolution já declarado acima

int parseHeaderValue(string line) {
    replace(line.begin(), line.end(), ',', ' ');
    stringstream ss(line);
    ss.imbue(locale("C"));
    string key;
    int value;
    ss >> key >> value;
    return value;
}

int main(std::string filename) {
	if (filename.empty()) {
		std::cerr << "Arquivo de instancia vazio." << std::endl;
		return 1;
	}

	ios_base::sync_with_stdio(false);

	ifstream fin(filename);
	if (!fin.is_open()) {
		std::cerr << "Nao foi possivel abrir arquivo de instancia: " << filename << std::endl;
		return 1;
	}
	fin.imbue(locale("C"));

    string line;

	// Saída automática baseada no nome da instância
	string outFile = filename + ".out";
	fileSolution.open(outFile);
	if (!fileSolution.is_open()) {
		std::cerr << "Nao foi possivel abrir arquivo de saida: " << outFile << std::endl;
		return 1;
	}

	// Evita que doubles "grandes" sejam impressos sem parte fracionária por causa
	// do default do iostream (6 dígitos significativos).
	fileSolution.setf(std::ios::fixed);
	fileSolution << std::setprecision(2);

    if (getline(fin, line))
        o = parseHeaderValue(line);
    if (getline(fin, line))
        m = parseHeaderValue(line);
    if (getline(fin, line))
        t = parseHeaderValue(line);
    if (getline(fin, line))
        c = parseHeaderValue(line);
	maquina.assign(m, vector<Operation>());
	vetOperacao.clear();

    getline(fin, line);
    getline(fin, line);

    int i = 0;
    int operacao_global = 0;
    map<int, map<int, int>> controleOp;

    while (getline(fin, line) && !line.empty()) {
        replace(line.begin(), line.end(), ',', ' ');
        stringstream ss(line);
        ss.imbue(locale("C"));

        int idJob, idOp, toolSetId, toolSetSize;
        double releaseTime, processingTime, dueDate;

        ss >> idJob >> idOp >> releaseTime >> processingTime >> dueDate >> toolSetId >> toolSetSize;

        if (ss) {
            Operation op(i, idJob, idOp, toolSetId - 1, toolSetSize, processingTime, dueDate, releaseTime);
            vetOperacao.push_back(op);
            controleOp[idJob][idOp] = 0;
            i++;
        }
    }

    // Sem args/flags: usa os parâmetros globais default (ou setados no código).

    map<int, double> tempo_final;
    vector<double> tardiness_maq;
	double best = 9e18;
	double initialBest = -1;
	
	auto populacao = o * POP_SIZE;
	unsigned chromosome_size = o;

	high_resolution_clock::time_point t1 = high_resolution_clock::now();

	unsigned RNG_SEED = static_cast<unsigned>(
		chrono::high_resolution_clock::now().time_since_epoch().count()
	);
	
	SampleDecoder decoder;
	cout << "Iniciando o SSGA..." << std::endl;
	SSGA ssga(chromosome_size, populacao, MUT_GENE_PROB, INTENS_PROB, TWO_SWAP, INSERTION, TWO_OPT, 7200.0, RNG_SEED, decoder);
	cout.setf(std::ios::fixed);
	cout << std::setprecision(6);
	cout << "[RUN] outFile='" << outFile << "' seed=" << RNG_SEED << " o=" << o << " m=" << m << " t=" << t << " c=" << c << std::endl;
	if (progressDebugEnabled()) {
		cout << "[RUN] debug: printing both currentBest(ssga) and globalBest(best)" << std::endl;
	}

	long teste_geracao = 0;
	high_resolution_clock::time_point t2;
	duration<double> time_span;
	initialBest = ssga.bestFitness();
	best = initialBest;

	generation = 0;

	do {
		++generation;
		double before = ssga.bestFitness();
		ssga.iterateOnce();
		double after = ssga.bestFitness();

		if (after < best) {
			nVoid = 0;
			teste_geracao = generation;
			best = after;
		} else
			++nVoid;

		t2 = high_resolution_clock::now();
		time_span = duration_cast<duration<double>>(t2 - t1);
			cout << "Iteração: " << generation
				 << " | currentBest(ssga): " << after
				 << " | globalBest(best): " << best
				 << " | nVoid: " << nVoid << endl;
	} while (nVoid < MAX_VOID && time_span.count() < 7200.0);

	string stopReason = "UNKNOWN";
	if (nVoid >= MAX_VOID) stopReason = "MAX_VOID";
	else if (time_span.count() >= 7200.0) stopReason = "TIME_LIMIT";

	t2 = high_resolution_clock::now();
	time_span = duration_cast<duration<double>>(t2 - t1);
 
	vector<double> ch = ssga.bestChromosome();


	vector<vector<pair<double,unsigned>>> ranking (m, vector<pair<double,unsigned>>());

	for(unsigned i = 0; i < ch.size(); ++i) {
		int mid = (int)floor(ch[i]);
		if (mid < 0 || mid >= m) return INT_MAX;
		ranking[mid].push_back(pair<double,unsigned>(ch[i], i));
	}
	
	for(int i = 0; i < m; ++i)
		sort(ranking[i].begin(), ranking[i].end());

	for(int i = 0; i < m; ++i) {
		for(auto& slaPorra : ranking[i]) {
			maquina[i].push_back(vetOperacao[slaPorra.second]);
		}
	}

	for (int i = 0; i < m; ++i) {
		int maxJob = 0;

		for(Operation op : maquina[i]){
			if(op.idJob > maxJob)
				maxJob = op.idJob;
		}

		vector<vector<pair<int, int>>> controle(maxJob + 1, vector<pair<int, int>>());

		for(int j = 0; j < maquina.size(); ++j) {
			controle[maquina[i][j].idJob].push_back(pair<int, int>(maquina[i][j].idOp, j));
		}
		
		for(auto& slaPorra : controle)
			sort(slaPorra.begin(), slaPorra.end());

		for(int j = 0; j < maxJob; ++j) {
			vector<int> lugar;
			vector<Operation> banco;
			for(auto& slaPorra : controle[j]) {
				lugar.push_back(slaPorra.second);
				banco.push_back(maquina[i][slaPorra.second]);
			}
			sort(lugar.begin(), lugar.end());
			for(int k = 0; k < lugar.size(); ++k) {
				maquina[i][lugar[k]] = banco[k];
			}
		}
	}

	cout << endl;

	double makespan = 00;

	fileSolution << initialBest << " " << best << " " << time_span.count() << " " << generation << endl;

	fileSolution << "\n==== RESULTADO DA EXECUÇÃO ====\n";
	fileSolution << "Makespan inicial: " << initialBest << "\n";
	fileSolution << "Melhor makespan: " << best << "\n";
	fileSolution << "Tempo de execução (s): " << time_span.count() << "\n";
	fileSolution << "Total de iterações: " << generation << "\n";
	fileSolution << "Razão de parada: " << stopReason << "\n";
	fileSolution << "Melhor iteração: " << teste_geracao << "\n";
	fileSolution << "Iterações sem melhoria: " << nVoid << "\n";
	fileSolution << "\n==== Estatísticas SSGA ====\n";
	fileSolution << "Filhos gerados: " << ssga.stats().childrenGenerated << "\n";
	fileSolution << "Rejeitados por pior absoluto (childFit > worst): " << ssga.stats().childrenRejectedByMean << "\n";
	fileSolution << "Rejeitados por serem inválidos: " << ssga.stats().childrenRejectedByInvalidity << "\n";
	fileSolution << "Sem candidatos piores para substituir (defensivo): " << ssga.stats().childrenNoWorseCandidates << "\n";
	fileSolution << "Inseridos na população: " << ssga.stats().childrenInserted << "\n";

	// Busca local
	fileSolution << "\n==== Busca Local (médias) ====\n";
	{
		auto s = ssga.stats();
		double s2_avg_us = (s.s2_n == 0) ? 0.0 : (static_cast<double>(s.s2_us) / static_cast<double>(s.s2_n));
		double in_avg_us = (s.in_n == 0) ? 0.0 : (static_cast<double>(s.in_us) / static_cast<double>(s.in_n));
		double o2_avg_us = (s.o2_n == 0) ? 0.0 : (static_cast<double>(s.o2_us) / static_cast<double>(s.o2_n));
		fileSolution << "swap2: n=" << s.s2_n << " avg_us=" << s2_avg_us << " melhoras=" << s.s2_b << " ganho_total=" << s.s2_g << "\n";
		fileSolution << "insertion: n=" << s.in_n << " avg_us=" << in_avg_us << " melhoras=" << s.in_b << " ganho_total=" << s.in_g << "\n";
		fileSolution << "opt2: n=" << s.o2_n << " avg_us=" << o2_avg_us << " melhoras=" << s.o2_b << " ganho_total=" << s.o2_g << "\n";
		long long best_g = max(max(s.s2_g, s.in_g), s.o2_g);
		int ties = 0;
		if (s.s2_g == best_g) ++ties;
		if (s.in_g == best_g) ++ties;
		if (s.o2_g == best_g) ++ties;
		const char* win = "EMPATE";
		if (ties == 1) {
			if (s.s2_g == best_g) win = "swap2";
			else if (s.in_g == best_g) win = "insertion";
			else win = "opt2";
		}
		fileSolution << "Melhor (por ganho_total): " << win << "\n";
	}

    fileSolution << "\n==== Parametros do Algoritmo ====\n";
	  fileSolution << "Algoritmo: SSGA (steady-state)\n";
	  fileSolution << "Tamanho da população: " << populacao << "\n";
	  fileSolution << "Prob. mutação por posição (swap): " << MUT_GENE_PROB << "\n";
	  fileSolution << "Máximo sem melhoria (MAX_VOID): " << MAX_VOID << "\n";
	  fileSolution << "Razão de parada: " << stopReason << "\n";

  	fileSolution << "\n==== Parâmetros da instância ====\n";
	fileSolution << "Maquinas: " << m << "\n";
  	fileSolution << "Operacoes: " << o << "\n";
  	fileSolution << "Ferramentas: " << t << "\n";
  	fileSolution << "Capacidade Magazine: " << c << "\n";

	fileSolution << "\n==== Solução ====\n";
    fileSolution << "Sequência de tarefas: ";
	for(auto tmp : maquina) {
		for(auto tmp1 : tmp) {
			fileSolution << tmp1.idJob << "|" << tmp1.idOp << " ";
		}
		fileSolution << "proximo\n";
	}
	fileSolution << "\n";

	fileSolution.close();
	return 0;
}
