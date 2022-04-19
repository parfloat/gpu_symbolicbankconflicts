/*
 * BankConflictsSideChannel.h
 *
 *adrianh
 *
 */
#ifndef GKLEE_LIB_CORE_BANKCONFLICTSSIDECHANNEL_H_
#define GKLEE_LIB_CORE_BANKCONFLICTSSIDECHANNEL_H_

#include "klee/Expr.h"
#include "klee/ExecutionState.h"
#include "TimingSolver.h"

typedef struct Line {
	MemoryAccessVec threadMemAccVec;
	unsigned shMemBytes;
	unsigned boxes;
	std::vector<const Array*> occupancy;
	std::vector<klee::ref<Expr> > occPreExpr;
	std::vector<klee::ref<Expr> > occConstrs;

	std::vector<const Array*> sum;
	std::vector<klee::ref<Expr> > sumExpr;
	std::vector<klee::ref<Expr> > sumConstrs;

	const Array* tSum;
	std::vector<klee::ref<Expr> > tSumExpr;
	std::vector<klee::ref<Expr> > tSumConstrs;

	const Array* max;
	std::vector<klee::ref<Expr> > maxSelectExpr;
	std::vector<klee::ref<Expr> > maxConstrs;

	~Line(){
		threadMemAccVec.clear();

		//		for (int i = 0 ; i < occupancy.size(); i++){
		//			delete occupancy[i];
		//		}
		occupancy.clear();
		occPreExpr.clear();
		occConstrs.clear();

		//		for (int i = 0 ; i < sum.size(); i++){
		//			delete sum[i];
		//		}
		sum.clear();
		sumExpr.clear();
		sumConstrs.clear();

		//		delete tSum;
		tSumExpr.clear();
		tSumConstrs.clear();

		//		delete max;
		maxSelectExpr.clear();
		maxConstrs.clear();
	}

	unsigned int getMaxPossibleTransactions(unsigned boxWidth, unsigned bankSize){
		unsigned activeThreads = threadMemAccVec.size();
		unsigned possibleBankLines = ((shMemBytes / boxWidth)) / bankSize;
		if (possibleBankLines == 0)
			possibleBankLines = 1;
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

//		tSumExpr.clear();
//		tSumConstrs.clear();

		tMaxExpr.clear();
		tMaxConstrs.clear();

		extraConstrs.clear();
	}

	std::vector<Line> lines;

//	const Array* tSum;
//	std::vector<klee::ref<Expr> > tSumExpr;
//	std::vector<klee::ref<Expr> > tSumConstrs;

	const Array* tMax;
	std::vector<klee::ref<Expr> > tMaxExpr;
	std::vector<klee::ref<Expr> > tMaxConstrs;

	std::vector<klee::ref<Expr> > extraConstrs;

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


	static klee::ref<Expr> isAddrInBoxI(klee::ref<Expr> &addr, unsigned boxID, unsigned boxWidth);

	static klee::ref<Expr> getPNP(klee::ref<Expr> &precond, const Array* symBox);

	static void computeOccupancy(Line &line);

	static klee::ref<Expr> getSumForBank(const Line &line, unsigned bankID, unsigned bankSize);

	static void computeSums(Line &line);

	static klee::ref<Expr> getLargerSumExpr(Line &line, unsigned sumIndex);

	static void computeMaximum(Line &line);

	static void computeTotalMax(ModelData &data);

	static void boundMax(Line &line);

	static void computeTotalSum(Line &line); //sum of occ[i][w][t]
	static void boundTotalSum(Line &line);
	static void boundTotalMax(ModelData &data);

	static void addExtraConstrs(ModelData &data);

	static void computeModel(ExecutionState& state, TimingSolver* solver, unsigned pathNum);

	static void naiveCEGAR(ExecutionState& state, ExecutionState& cState,
			ModelData &model,
			std::vector<const Array*> &objects,
			std::vector<std::vector<unsigned char> >& values);

	static int solveAllConstraints(ExecutionState& state, TimingSolver* solver,
			ModelData &data, long bound = -1);

	static void checkEquality(ExecutionState& state, TimingSolver* solver,
			ModelData &data, long bound = -1);

	static void analyzePerUniqueLine(ModelData &data,ExecutionState& state,
			TimingSolver* solver);

	static void analyzeUniqueLinesTarget(ModelData &data,ExecutionState& state,
			TimingSolver* solver, const int obsTarget);

private:

	static void resetModel();
	static void buildInternalStructure(ModelData &data, const ShLineAccType &accesses, std::string uniqueID);
	static void buildConstraints(ModelData &data);

	static void printLineAccesses(std::string path, const ShLineAccType &accesses);
	static void printConstraints(std::string path, ModelData &data);

	static ModelData readLines;
	static ModelData writeLines;
};



#endif /* GKLEE_LIB_CORE_BANKCONFLICTSSIDECHANNEL_H_ */
