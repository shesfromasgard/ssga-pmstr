#ifndef OPERATION_H
#define OPERATION_H

class Operation {
public: 
    int id;              //PK
    int idJob;           // Identificador do job ao qual a operação pertence
    int idOp;            // Índice da operação dentro do job
    int toolSetId;       // Identificador do conjunto de ferramentas
    int toolSetSize;     // Tamanho do conjunto de ferramentas
    double processingTime;  // Tempo de processamento
    double dueDate;         // Data de entrega
    double releaseTime;     // Data de liberação
    bool isProcessed;    // Indica se a operação já foi processada

    Operation();
    Operation(int id, int idJob, int idOp, int toolSetId, int toolSetSize, double processingTime, double dueDate, double releaseTime, bool isProcessed = false);
};

#endif // OPERATION_H
