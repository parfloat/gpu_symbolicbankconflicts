/*
* LineExplorer.h
*
* rahmed
*/
#ifndef GKLEE_LIB_CORE_LINEEXPLORER_H_
#define GKLEE_LIB_CORE_LINEEXPLORER_H_


#include <map>
#include <vector>

#include "klee/Expr.h"
#include "klee/ExecutionState.h"
#include "TimingSolver.h"

#include "BankConflictsSideChannel.h"
#include "Cliques.h"

class LineExplorer{

public:
  ExecutionState state;
  TimingSolver* solver;
  unsigned lineID;
  Line line;

  unsigned range;
  unsigned maxCliqueSize;
  // cache, for each clique size s, a mapping from  a clique c of size s to sets
  // of cliques that need to be forbidden to allow the clique of size s to hold
  // without any clique of size s+1 holding.
  std::vector<std::map<Clique, CliqueSet>> storage;
  std::vector<CliqueSet> unsatSubCliques;
  // cache formulae for pairwise conflicts
  std::map<Clique,klee::ref<Expr>> pairwiseConflicts;

  std::vector<const Array*> objects;
  //		std::vector< std::vector<unsigned char> > values;
  std::vector<const Array*> conflicts;
  std::vector<klee::ref<Expr> > cstrs;

public:

  unsigned getRange()const;

  unsigned getMaxCliqueSize()const;

  LineExplorer(ExecutionState, TimingSolver*, unsigned, Line);

  ~LineExplorer(){
    // storage.clear();
    // pairwiseConflicts.clear();
    // objects.clear();
    // conflicts.clear();
    // cstrs.clear();
  }

  void addEncoding(ExecutionState&, std::vector<const Array*>&);

  /* gets a clique as input and returns a set of cliques that need to be forbidden
  * to allow (not ensure!) for |clique| transactions.
  * - if the returned set is empty, then no cliques need to be forbidden
  * - if the returned set contains the empty set, then it is not possible to
  *   get |clique| transactions with clique.
  * If you do not want the state to be messed up, pass a copy.
  */
  bool makeCliquePossibleLocally(const Clique&);

  bool enforce(ExecutionState&, const Clique&);

  bool forbidd(ExecutionState&, const Clique&);

  Clique getSpoiler(unsigned _spoilerSize, const std::vector<const Array*>& _objects, const std::vector< std::vector<unsigned char> >& _values);


private:

  /* Encodes (line(i)/box_width)!=(line(j)/box_width) but (line(i)/box_width)%bank_size == (line(j)/box_width)%bank_size*/
  klee::ref<Expr> makePairConflict(const Line&, unsigned, unsigned);
};


#endif
