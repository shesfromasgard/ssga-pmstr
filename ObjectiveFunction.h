#ifndef OBJECTIVEFUNCTION_H
#define OBJECTIVEFUNCTION_H

#include <vector>
#include <map>
#include "Operation.h"

extern int m;
extern int o;
extern int t;
extern int c;

double objectiveFunction(const std::vector<std::vector<Operation>> &maquina, 
    const std::vector<Operation> &vetOperacoes, 
    const std::map<int, std::map<int, int>> &controleOp_base, 
    std::vector<double>& tardiness_maq);

#endif