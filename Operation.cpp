#include <vector>

#include "Operation.h"

int m = 0;
int o = 0;
int t = 0;
int c = 0;

std::vector<Operation> vetOperacao;

Operation::Operation() : id(0), idJob(0), idOp(0), toolSetId(0), toolSetSize(0), processingTime(0), dueDate(0), releaseTime(0), isProcessed(0) {}

Operation::Operation(int id, int idJob, int idOp, int toolSetId, int toolSetSize, double processingTime, double dueDate, double releaseTime, bool isProcessed)
	: id(id), idJob(idJob), idOp(idOp), toolSetId(toolSetId), toolSetSize(toolSetSize), processingTime(processingTime), dueDate(dueDate), releaseTime(releaseTime), isProcessed(isProcessed) {}
