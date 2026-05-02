#ifndef SSGA_H
#define SSGA_H

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <utility>
#include <vector>
#include <iostream>
#include <limits>
#include <time.h>
#include <climits>

#include "SampleDecoder.h"

std::random_device rd;
std::mt19937 g(rd());

using namespace std;

class SSGA {
public:
	struct Stats {
		unsigned long long childrenGenerated = 0;
		unsigned long long childrenEvaluated = 0;
		unsigned long long childrenRejectedByMean = 0;
		unsigned long long childrenNoWorseCandidates = 0;
		unsigned long long childrenInserted = 0;
		unsigned long long childrenRejectedByInvalidity = 0;
		unsigned long long validos = 0;

		unsigned long long s2_n = 0;
		unsigned long long s2_us = 0;
		unsigned long long s2_b = 0;
		long long s2_g = 0;

		unsigned long long in_n = 0;
		unsigned long long in_us = 0;
		unsigned long long in_b = 0;
		long long in_g = 0;
		
		unsigned long long o2_n = 0;
		unsigned long long o2_us = 0;
		unsigned long long o2_b = 0;
		long long o2_g = 0;
	};

	SSGA(unsigned chromosomeSize,
		unsigned populationSize,
		double mutationProbPerGene,
		double localSearchProb,
		bool enableSwap2,
		bool enableInsertion,
		bool enableOpt2,
		double timeLimitSeconds,
		unsigned seed,
		SampleDecoder& decoder): n(chromosomeSize),
		p(populationSize),
		mutProb(mutationProbPerGene),
		localProb(localSearchProb),
		enSwap2(enableSwap2),
		enInsertion(enableInsertion),
		enOpt2(enableOpt2),
		deadline(std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(timeLimitSeconds))),
		rng(seed),
		dist01(0.0, 1.0),
		decoder(decoder),
		pop(p, std::vector<double>(n, 0.0)),
		popSeq(p),
		fit(p, 0.0),
		bestIdx(0) {
		initialize();
	}

	void initialize() {
		for (unsigned i = 0; i < p; ++i) {
			std::vector<double> chromosome = decoder.encoder();
			std::vector<int> sequence;

			if(decoder.decode(chromosome) == INT_MAX) {
				--i;
				continue;
			} else {
				st.validos++;
			}

			std::vector<std::pair<double, unsigned>> ranking(n);
			for (unsigned g = 0; g < o; ++g) {
				ranking[g] = std::pair<double, unsigned>(chromosome[g], g);
			}
			std::sort(ranking.begin(), ranking.end());

			sequence.reserve(n);
			for (const auto& r : ranking) {
				sequence.push_back(r.second);
			}

			pop[i] = chromosome;
			popSeq[i] = sequence;
		}
		
		evaluatePopulation();
	}

	void iterateOnce() {
		if (timeExceeded()) return;
		unsigned pa = tournamentSelect();
		unsigned pb = tournamentSelect();

		std::vector<double> childA(n);
		std::vector<double> childB(n);
		for (unsigned g = 0; g < n; ++g) {
			if (dist01(rng) < 0.5) {
				childA[g] = pop[pa][g];
				childB[g] = pop[pb][g];
			} else {
				childA[g] = pop[pb][g];
				childB[g] = pop[pa][g];
			}
		}

		tryProduceChild(childA);
		if (timeExceeded()) return;
		tryProduceChild(childB);
	}

	double bestFitness() const { return fit[bestIdx]; }
	const std::vector<double>& bestChromosome() const { return pop[bestIdx]; }
	const Stats& stats() const { return st; }
	double meanFitness() const {
		double s = std::accumulate(fit.begin(), fit.end(), 0.0);
		return (p == 0) ? 0.0 : (s / static_cast<double>(p));
	}

private:
	unsigned n;
	unsigned p;
	double mutProb;
	double localProb;
	bool enSwap2;
	bool enInsertion;
	bool enOpt2;
	std::chrono::steady_clock::time_point deadline;

	std::mt19937_64 rng;
	std::uniform_real_distribution<double> dist01;
	SampleDecoder& decoder;

	std::vector<std::vector<double>> pop;
	std::vector<std::vector<int>> popSeq;
	std::vector<double> fit;
	unsigned bestIdx;
	Stats st;

	bool timeExceeded() const {
		return std::chrono::steady_clock::now() >= deadline;
	}

	std::vector<int> sequenceFromChromosome(const std::vector<double>& chromosome) const {
		std::vector<std::pair<double, unsigned>> ranking(chromosome.size());
		for (unsigned i = 0; i < chromosome.size(); ++i) {
			ranking[i] = std::pair<double, unsigned>(chromosome[i], i);
		}
		std::sort(ranking.begin(), ranking.end());

		std::vector<int> seq;
		seq.reserve(chromosome.size());
		for (const auto& r : ranking) {
			seq.push_back(r.second);
		}
		return seq;
	}

	bool sequenceExistsInPopulation(const std::vector<int>& seq) const {
		for (unsigned i = 0; i < p; ++i) {
			if (popSeq[i] == seq) return true;
		}
		return false;
	}

	void tryProduceChild(std::vector<double>& child) {
		if (timeExceeded()) return;
		++st.childrenGenerated;
		mutate(child);
		if(dist01(rng) < localProb) {
			int indexes[3];
			int z = 0;
			if (enSwap2) indexes[z++] = 0;
			if (enInsertion) indexes[z++] = 1;
			if (enOpt2) indexes[z++] = 2;
			if (z > 1) {
				for (int i = z - 1; i > 0; --i) {
					std::uniform_int_distribution<int> pick(0, i);
					int j = pick(rng);
					std::swap(indexes[i], indexes[j]);
				}
			}
			for (int j = 0; j < z; ++j) {
				switch (indexes[j]) {
					case 0: {
						auto t0 = std::chrono::high_resolution_clock::now();
						long d = swap2(child);
						auto t1 = std::chrono::high_resolution_clock::now();
						st.s2_n++;
						st.s2_us += static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
						if (d > 0) st.s2_b++;
						st.s2_g += d;
						break;
					}
					case 1: {
						auto t0 = std::chrono::high_resolution_clock::now();
						long d = insertion(child);
						auto t1 = std::chrono::high_resolution_clock::now();
						st.in_n++;
						st.in_us += static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
						if (d > 0) st.in_b++;
						st.in_g += d;
						break;
					}
					case 2: {
						auto t0 = std::chrono::high_resolution_clock::now();
						long d = opt2(child);
						auto t1 = std::chrono::high_resolution_clock::now();
						st.o2_n++;
						st.o2_us += static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
						if (d > 0) st.o2_b++;
						st.o2_g += d;
						break;
					}
				}
			}
		}
		std::vector<int> childSeq = sequenceFromChromosome(child);
		if (timeExceeded()) return;

		double childFit = decoder.decode(child);
		++st.childrenEvaluated;
		if(childFit == INT_MAX) {
			++st.childrenRejectedByInvalidity;
			return;
		}
		tryInsert(child, childFit, std::move(childSeq));
	}

	void evaluatePopulation() {
		for (unsigned i = 0; i < p; ++i) {
			fit[i] = decoder.decode(pop[i]);
		}
		bestIdx = 0;
		for (unsigned i = 1; i < p; ++i) {
			if (fit[i] < fit[bestIdx]) bestIdx = i;
		}
	}

	unsigned tournamentSelect() {
		std::uniform_int_distribution<unsigned> pick(0, p - 1);
		unsigned a = pick(rng);
		unsigned b = pick(rng);
		return (fit[b] < fit[a]) ? b : a;
	}

	void mutate(std::vector<double>& x) {
		for (unsigned g = 0; g < n; ++g) {
			if (dist01(rng) < mutProb) {
				x[g] = dist01(rng);
			}
		}
	}

	void tryInsert(std::vector<double>& child, double childFit, std::vector<int>&& childSeq) {
		auto worstIt = std::max_element(fit.begin(), fit.end());
		double worstFit = (worstIt == fit.end()) ? childFit : *worstIt;
		if (childFit > worstFit) {
			++st.childrenRejectedByMean;
			return;
		}

		std::vector<unsigned> worst;
		worst.reserve(p);
		for (unsigned i = 0; i < p; ++i) {
			if (fit[i] == worstFit) worst.push_back(i);
		}
		if (worst.empty()) {
			++st.childrenNoWorseCandidates;
		}

		std::uniform_int_distribution<size_t> pick(0, worst.size() - 1);
		unsigned replaceIdx = worst[pick(rng)];

		pop[replaceIdx].swap(child);
		popSeq[replaceIdx] = std::move(childSeq);
		fit[replaceIdx] = childFit;
		if (replaceIdx == bestIdx) {
			bestIdx = static_cast<unsigned>(std::min_element(fit.begin(), fit.end()) - fit.begin());
		} else if (fit[replaceIdx] < fit[bestIdx]) {
			bestIdx = replaceIdx;
		}
		++st.childrenInserted;
	}

	long swap2(vector<double>& processos) {
		vector<double> solucaoAtual = processos;
		long r0 = decoder.decode(solucaoAtual);
		long resultadoAtual = r0;
		long resultadoNovo;

		vector<int>order(n);
		iota(order.begin(), order.end(), 0);
		shuffle(order.begin(), order.end(), rng);

		bool melhorou = true;
		while (melhorou) {
			if (timeExceeded()) break;
			melhorou = false;

			for (unsigned i = 0; i + 1 < n && !melhorou; ++i) {
				for (unsigned j = i + 1; j < n; ++j) {
					if (timeExceeded()) break;
					trocar(solucaoAtual, order[i], order[j]);
					resultadoNovo = decoder.decode(solucaoAtual);
					if (resultadoNovo < resultadoAtual) {
						resultadoAtual = resultadoNovo;
						melhorou = true;
						break;
					}
					trocar(solucaoAtual, order[i], order[j]);
				}
			}
		}

		processos = solucaoAtual;
		return r0 - resultadoAtual;
	}

	void trocar(vector<double>& solucaoNova, int a, int b) {
		if(a != b)
			swap(solucaoNova[a], solucaoNova[b]);
	}

	long opt2(vector<double>& processos) {
		vector<double> solucaoAtual = processos;
		long r0 = decoder.decode(solucaoAtual);
		long resultadoAtual = r0;
		long resultadoNovo;

		vector<int>order(n);
		iota(order.begin(), order.end(), 0);
		shuffle(order.begin(), order.end(), rng);

		bool melhorou = true;
		while (melhorou) {
			if (timeExceeded()) break;
			melhorou = false;

			for (unsigned i = 0; i + 1 < n && !melhorou; ++i) {
				for (unsigned j = i + 1; j < n; ++j) {
					if (timeExceeded()) break;
					trocar2(solucaoAtual, order[i], order[j]);
					resultadoNovo = decoder.decode(solucaoAtual);
					if (resultadoNovo < resultadoAtual) {
						resultadoAtual = resultadoNovo;
						melhorou = true;
						break;
					}
					trocar2(solucaoAtual, order[i], order[j]);
				}
			}
		}

		processos = solucaoAtual;
		return r0 - resultadoAtual;
	}

	void trocar2(vector<double>& solucaoNova, int a, int b) {
		if (a == b) return;
		std::reverse(solucaoNova.begin() + min(a, b), solucaoNova.begin() + max(a, b) + 1);
	}

	long insertion(vector<double>& processos) {
		vector<double> solucaoAtual = processos;
		long r0 = decoder.decode(solucaoAtual);
		long resultadoAtual = r0;
		long resultadoNovo;
		
		vector<int>order(n);
		iota(order.begin(), order.end(), 0);
		shuffle(order.begin(), order.end(), rng);

		bool melhorou = true;

		while (melhorou) {
			if (timeExceeded()) break;

			melhorou = false;

			for (unsigned i = 0; i < n && !melhorou; ++i) {
				for (unsigned j = i + 1; j < n; ++j) {
					if (timeExceeded()) break;
					if (order[i] == order[j]) continue;
					double val = solucaoAtual[order[i]];
					if (order[i] < order[j]) {
						for (int k = order[i]; k < order[j]; ++k) {
							solucaoAtual[k] = solucaoAtual[k + 1];
						}
						solucaoAtual[order[j]] = val;
					} else {
						for (int k = order[i]; k > order[j]; --k) {
							solucaoAtual[k] = solucaoAtual[k - 1];
						}
						solucaoAtual[order[j]] = val;
					}

					resultadoNovo = decoder.decode(solucaoAtual);

					if (resultadoNovo < resultadoAtual) {
						resultadoAtual = resultadoNovo;
						melhorou = true;
						break;
					}

					if (order[i] < order[j]) {
						double v2 = solucaoAtual[order[j]];
						for (int k = order[j]; k > order[i]; --k) {
							solucaoAtual[k] = solucaoAtual[k - 1];
						}
						solucaoAtual[order[i]] = v2;
					} else {
						double v2 = solucaoAtual[order[j]];
						for (int k = order[j]; k < order[i]; ++k) {
							solucaoAtual[k] = solucaoAtual[k + 1];
						}
						solucaoAtual[order[i]] = v2;
					}
				}
			}
		}

		processos = solucaoAtual;
		return r0 - resultadoAtual;
	}
};

#endif
