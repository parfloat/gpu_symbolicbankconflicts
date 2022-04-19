/*
* CompoundExplorer.h
*
* rahmed
*/
#ifndef GKLEE_LIB_CORE_COMPOUND_EXPLORER_H_
#define GKLEE_LIB_CORE_COMPOUND_EXPLORER_H_

#include "klee/Expr.h"
#include "klee/ExecutionState.h"
#include "TimingSolver.h"

#include "BankConflictsSideChannel.h"
#include "LineExplorer.h"

#include "Cliques.h"


class CompoundExplorer{
static int tries;

  ExecutionState compoundState;
  TimingSolver* solver;
  std::vector<const Array*> compoundObjects;

  LineExplorer* pHeadExplorer = nullptr;
  CompoundExplorer* pTailCompoundExplorer = nullptr;
  unsigned rank = 0;

  std::map<std::vector<Clique>,bool> cachedCompoundCliques;

public:
  CompoundExplorer(ExecutionState&, LineExplorer*, CompoundExplorer*, TimingSolver*);

  // given a clique for each line, add constraints (in the form of forbidden larger cliques)
  // untill one answers whether the chosen cliques coexist or not.
  bool cliquesArePossibleGlobally(const std::vector<Clique>& cliques);

  bool cliquesDump(const std::vector<Clique>& cliques, int byteIndex, int byteValue, int observationValue);


private:
  void recursivelyAddEncoding(ExecutionState&, std::vector<const Array*>&);

  bool recursivelyEnforce(ExecutionState&, const std::vector<Clique>&, unsigned);

  std::vector<CliqueSet> getSpoilers(unsigned size, std::vector< std::vector<unsigned char> >& values);

  bool enforceSpoilers(const std::vector<CliqueSet>&);


  // // returns true whether chosen cliques, one for each line,
  // // can coexist.
  // bool feasible();

  std::vector<Clique> getSpoilersRecursively(const std::vector<unsigned>& _sizes, unsigned _pos, const std::vector<const Array*>& _objects,
    const std::vector< std::vector<unsigned char> >& _values);

  bool forbidSpoilersRecursively(ExecutionState& cstate, const std::vector<Clique>&, unsigned);

public:


  class TargetedCompoundCliques{
  	CompoundExplorer* pCmpdExpl = nullptr;
  	unsigned target=0;
  	unsigned tailContr=0;
  	CliqueGenerator headClq;
  	TargetedCompoundCliques* pTailClqs = nullptr;
  	bool empty=false;
  public:
  	TargetedCompoundCliques(CompoundExplorer* _pCmpdExpl, unsigned _target);

  	~TargetedCompoundCliques();

    bool evaluate();

  	// bool reset();

  	std::vector<Clique> getCliques()const;

  	bool isEmpty()const;

    // bool isSomeEmpty() const;

  	bool lazyNext();

  	bool next();

  	bool tryDump(int byteIndex, int byteValue, int observationValue);

  private:
  	bool uncheckedGeneratedNext();
  };
};



#endif
