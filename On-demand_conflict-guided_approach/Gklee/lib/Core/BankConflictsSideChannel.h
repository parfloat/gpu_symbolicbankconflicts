/*
 * BankConflictsSideChannel.h
 *
 * adrianh
 * rahmed
 */
#ifndef GKLEE_LIB_CORE_BANKCONFLICTSSIDECHANNEL_H_
#define GKLEE_LIB_CORE_BANKCONFLICTSSIDECHANNEL_H_

#include <map>

#include "klee/Expr.h"
#include "klee/ExecutionState.h"
#include "TimingSolver.h"



struct Line {
	MemoryAccessVec threadMemAccVec;
	unsigned shMemBytes;

	~Line(){
		threadMemAccVec.clear();

	}

	unsigned int getMaxPossibleTransactions(const unsigned boxWidth, const unsigned bankSize){
		unsigned activeThreads = threadMemAccVec.size();
		unsigned possibleBankLines = ((shMemBytes / boxWidth)) / bankSize;
		if (shMemBytes != possibleBankLines * boxWidth * bankSize)
			possibleBankLines += 1;
		unsigned maxBound = (activeThreads < possibleBankLines)?(activeThreads):(possibleBankLines);
		return maxBound;
	}
};

class UniqLine {
public:
	Line* head;
	std::vector<Line*> same;

	int getFactor(){ return same.size(); }

};

class ModelData{

public:

	void clear(){
		lines.clear();

		equivalenceClasses.clear();
		uniqueLines.clear();
		possibleValues.clear();
	}

	std::vector<Line> lines;

	std::vector<int> equivalenceClasses;
	std::map<int, UniqLine> uniqueLines;
	std::map<int, std::vector<int> > possibleValues;

	void computeUniqueLines(ExecutionState& state, TimingSolver* solver);

private:
	void computeEquivalenceClasses(ExecutionState& state, TimingSolver* solver);
};


class BankConflictsSideChannel{
public:
	static unsigned BANK_SIZE;
	static unsigned BOX_WIDTH;

	static void computeModel(ExecutionState& state, TimingSolver* solver, unsigned pathNum);

	static int getConflicts(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound);

	static int solveAllDyConstraints(ExecutionState& state, TimingSolver* solver,
			ModelData &data, long bound = -1);

	static int getLineValues(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound);
	static int getNPairLineValues(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound, unsigned pairs);

	static int solveUniqueLineDyConstraints(ExecutionState& state, TimingSolver* solver,
			ModelData &data, long bound = -1);
	static int generateConstraintFiles(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound = -1);


private:

	static void resetModel();
	static void buildInternalDyStructure(ModelData &data, const ShLineAccType &accesses, std::string uniqueID);
	static void buildDyConstraints(ModelData &data);

	static void printLineAccesses(std::string path, const ShLineAccType &accesses);

	static ModelData readLines;
	static ModelData writeLines;
};

#endif /* GKLEE_LIB_CORE_BANKCONFLICTSSIDECHANNEL_H_ */
