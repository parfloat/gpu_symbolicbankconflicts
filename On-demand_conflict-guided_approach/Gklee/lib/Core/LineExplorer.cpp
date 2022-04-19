/*
 * LineExplorer.cpp
 * rahmed
 */

 // #include <map>
 // #include <vector>
 #include <string>
 #include <set>
 #include <algorithm>

 #define DEBUG_LINE_EXPLORER 1

 #ifdef DEBUG_LINE_EXPLORER
   #include <iostream>
 #endif

 #include "LineExplorer.h"


unsigned LineExplorer::getRange()const{return range;}

unsigned LineExplorer::getMaxCliqueSize()const{return maxCliqueSize;}

LineExplorer::LineExplorer(ExecutionState _state, TimingSolver* _solver, unsigned _lineID, Line _line):
state(_state), solver(_solver), lineID(_lineID), line(_line), range(line.threadMemAccVec.size())
{
  unsigned bytes_per_bank = BankConflictsSideChannel::BOX_WIDTH * BankConflictsSideChannel::BANK_SIZE;
  if(line.shMemBytes % bytes_per_bank)
	 maxCliqueSize= 1 + line.shMemBytes / bytes_per_bank;
  else
   maxCliqueSize= line.shMemBytes / bytes_per_bank;

	if(maxCliqueSize > range) maxCliqueSize = range;
	storage.resize(maxCliqueSize + 1);
  unsatSubCliques.resize(maxCliqueSize + 1);
	// build pairwise constraints pairwieConflicts[ij] and add corresponding "i:j" variables to state.
	//collect pairwise conflicts and constraints
	for(unsigned i=0; i < line.threadMemAccVec.size(); ++i){
		for(unsigned j=i+1; j < line.threadMemAccVec.size(); ++j){
#ifdef DEBUG_LINE_EXPLORER
//			std::cout << std::endl << "making " << i << " and " << j << " conflict" << std::endl;
#endif
			Clique conflictingPair; conflictingPair.insert(i); conflictingPair.insert(j);
			pairwiseConflicts[conflictingPair]= state.constraints.simplifyExpr(makePairConflict(line,i,j));
//			std::string name = std::to_string(lineID) + "|" + std::to_string(i) + ":" + std::to_string(j); //FIXME adrianh : seems to be an issue in the Z3 solver, if it starts with a number
			std::string name = "L" + std::to_string(lineID) + "T" + std::to_string(i) + "S" + std::to_string(j);
			const Array* conflict = new Array(name, 1);
			klee::ref<Expr> readConflict = ReadExpr::createTempRead(conflict, 1);
			conflicts.push_back(conflict);
			klee::ref<Expr> eqConflict = EqExpr::create(readConflict,pairwiseConflicts[conflictingPair]);
#ifdef DEBUG_LINE_EXPLORER
//			std::cout << "eqConflict : " << eqConflict << std::endl;
#endif
		cstrs.push_back(eqConflict);
		}
	}
	// add symbolic objects (e.g. symbolic input arrays)
	for (unsigned SI = 0; SI != state.symbolics.size(); SI++) {
		objects.push_back(state.symbolics[SI].second);
	}
	// add conflict symbols and constrataints
	addEncoding(state,objects);
	// std::cout << "storage size: " << storage.size() << std::endl;
}

void LineExplorer::addEncoding(ExecutionState& _state, std::vector<const Array*>& _objects)
{
//	std::cout << "--Entering addEncoding : " << cstrs.size() << std::endl;
	for(unsigned ij = 0; ij < cstrs.size(); ++ij){
//		std::cout << "-->> Pushing object conflicts" << std::endl;
		_objects.push_back(conflicts[ij]);
//		std::cout << "-->> Adding to _state the contraint : " << cstrs[ij] << std::endl;
		_state.addConstraint(cstrs[ij]);
	}
}
/* gets a clique as input and returns a set of cliques that need to be forbidden
* to allow (not ensure!) for |clique| transactions.
* - if the returned set is empty, then no cliques need to be forbidden
* - if the returned set contains the empty set, then it is not possible to
*   get |clique| transactions with clique.
* If you do not want the state to be messed up, pass a copy.
*/
bool LineExplorer::makeCliquePossibleLocally(const Clique& clique){
#ifdef DEBUG_LINE_EXPLORER
  // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
  std::cout << "making " << clique << " possible at line " << lineID << std::endl;
#endif
  if(clique.size() > maxCliqueSize || clique.empty()){
    return false;
  }
  //	std::map<Clique,CliqueSet>& mappedSoFar=storage[clique.size()];
  if(storage[clique.size()].find(clique) != storage[clique.size()].end()){//already cached
#ifdef DEBUG_LINE_EXPLORER
    // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    std::cout << "clique cached at line " << lineID << std::endl;
#endif
    return !isEmptySingleton(storage[clique.size()].find(clique)->second);
  }

// check if already encountered a subclique that was impossible on its own
  for(unsigned sz=0; sz <= clique.size(); sz++){
    for(const Clique& usc: unsatSubCliques[sz]){
      bool uscIncluded=true;
      for(unsigned i: usc){
        if(clique.find(i) == clique.end()){
          uscIncluded=false;
          break;
        }
      }
      if(uscIncluded){
        #ifdef DEBUG_LINE_EXPLORER
        // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        std::cout << "smaller clique " << usc << " was ompossible " << std::endl;
        #endif
        return false;
      }
    }
  }

  ExecutionState cState(state); //work on a copy to not mess the state
  if(!enforce(cState,clique)){ //preliminary check: clique impossible
#ifdef DEBUG_LINE_EXPLORER
    // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    std::cout << "could not enforce clique, failure at line " << lineID << std::endl;
#endif
    // encode impossibility: need to forbidd an empty set
    (storage[clique.size()]).insert(std::pair<Clique,CliqueSet>(clique,{{}}));
    unsatSubCliques[clique.size()].insert(clique);
    return false;
  } // Now all solutions will have the clique.
  Clique spoiler;
  CliqueSet cliques2bForbidden;
  do{ //while there are spoilers to forbid
#ifdef DEBUG_LINE_EXPLORER
    // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    std::cout << "quering solver at line " << lineID << std::endl;
#endif
    // get a solution with enforced clique and forbidden cliques2bForbidden.
    std::vector< std::vector<unsigned char> > values;
    bool success = solver->getInitialValues(cState, objects, values);
    solver->setTimeout(0);
    if(!success){
#ifdef DEBUG_LINE_EXPLORER
      // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
      std::cout << "no solutions, failure at line " << lineID << std::endl;
#endif
      // Encode impossible clique and return.
      // storage[clique.size()][clique]=getEmptySingleton();
      (storage[clique.size()]).insert(std::pair<Clique,CliqueSet>(clique,{{}}));
      if(cliques2bForbidden.empty())
        unsatSubCliques[clique.size()].insert(clique);
      return false;
    }
#ifdef DEBUG_LINE_EXPLORER
    // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    std::cout << "found solution, spoilers? at line " << lineID << std::endl;
#endif
    //see if larger cliques are possible
    spoiler = getSpoiler(clique.size()+1, objects, values);
    if(spoiler.empty()){
      // solution has no larger cliques.
#ifdef DEBUG_LINE_EXPLORER
      // std::cout << __FILE__ << ": " <<  __LINE__ << std::endl;
      std::cout << "no spoiler, success at line " << lineID << std::endl;
      // for (int symIndex = objects.size()-1; symIndex >= 0 ; --symIndex) {
      // 	std::cout << objects[symIndex]->name << " = ";
      // 	std::cout << "(size " << objects[symIndex]->size << ", little-endian) ";
      // 	for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); ++valueIndex)
      // 	std::cout << (int)values[symIndex][valueIndex] << " ";
      // 	std::cout << std::endl;
      // }
#endif
      // return fobidden cliques so far.
      // storage[clique.size()][clique] = cliques2bForbidden;
      (storage[clique.size()]).insert(std::pair<Clique,CliqueSet>(clique,cliques2bForbidden));
      return true;
    }
#ifdef DEBUG_LINE_EXPLORER
    // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    std::cout << "found spoiler {" << std::endl;
    std::copy(spoiler.begin(), spoiler.end(), std::ostream_iterator<unsigned>(std::cout, " "));
    std::cout << "} at line " << lineID << std::endl;
#endif
    cliques2bForbidden.insert(spoiler);
  }while(forbidd(cState, spoiler));
#ifdef DEBUG_LINE_EXPLORER
  // std::cout << __FILE__ << ": " << __LINE__ << std::endl;
  std::cout << "could not forbidd spolier. failure at line " << lineID << std::endl;
#endif
  // impossible to forbid the spoiler.
  // Encode impossibility and return.
  // storage[clique.size()][clique]=getEmptySingleton();
  (storage[clique.size()]).insert(std::pair<Clique,CliqueSet>(clique,{{}}));
  return false;
}


bool LineExplorer::enforce(ExecutionState& pCState, const Clique& clique){
	#ifdef DEBUG_LINE_EXPLORER
	 std::cout << "enforcing at line " << lineID << std::endl;
	#endif
	for(unsigned a: clique){
		for(unsigned b: clique){
			if(a<b){
				#ifdef DEBUG_LINE_EXPLORER
				// std::cout << "a=" << a << ", b=" << b << std::endl;
				#endif
				Clique ab; ab.insert(a); ab.insert(b); 
				auto simplifiedExpr = pCState.constraints.simplifyExpr(pairwiseConflicts[ab]); //makes the expression constant if it can
				if(simplifiedExpr->getKind()==Expr::Constant){
					if(cast<ConstantExpr>(simplifiedExpr)->isFalse()){
						return false;
					}
				}
				pCState.addConstraint(simplifiedExpr);
			}
		}
	}
	#ifdef DEBUG_LINE_EXPLORER
	// std::cout << "finished enforcing at line " << lineID << std::endl;
	#endif
	std::cout << "finished enforcing at line " << lineID << std::endl;
	return true;
}

// returns false iff detects unsat
bool LineExplorer::forbidd(ExecutionState& pCState, const Clique& clique){
  if(clique.size() <= 1)
    return false;
	klee::ref<Expr> cliqueEncoding = ConstantExpr::create(true,Expr::Bool);
	for(unsigned a: clique){
		for(unsigned b: clique){
			if(a<b){
				Clique ab; ab.insert(a); ab.insert(b);
			     cliqueEncoding = AndExpr::create(pairwiseConflicts[ab], cliqueEncoding);
          //cliqueEncoding = pairwiseConflicts[ab];
          //break; //
			}
		}
    break;
	}
	klee::ref<Expr> encodingForbiddenClique = pCState.constraints.simplifyExpr(NotExpr::create(cliqueEncoding));
	if(encodingForbiddenClique->getKind()==Expr::Constant){
		if(cast<ConstantExpr>(encodingForbiddenClique)->isFalse()){
			return false;
		}
	}
	pCState.addConstraint(encodingForbiddenClique);
	return true;
}

Clique LineExplorer::getSpoiler(unsigned _spoilerSize, const std::vector<const Array*>& _objects, const std::vector< std::vector<unsigned char> >& _values){
	UGraph conflictGraph;
  // #ifdef DEBUG_LINE_EXPLORER
	// std::cout << "begin getSpoiler at Line = " << lineID << " of size at least " << _spoilerSize << std::endl;
  // #endif
  for (unsigned symIndex = 0; symIndex < _objects.size(); ++symIndex) {
		for (unsigned valueIndex = 0; valueIndex < _values[symIndex].size(); ++valueIndex)
		if(_values[symIndex].size()== 1){
			std::string name = _objects[symIndex]->name;
			//int pos1 = name.find("|");
			int pos1 = name.find("T");
			//int pos2 = name.find(":");
			int pos2 = name.find("S");
			if(pos1 != std::string::npos){
				//unsigned ln = std::stoi(name.substr(0,pos1));
				unsigned ln = std::stoi(name.substr(1,pos1)); //adrianh added a non numeric first character "l" so that the smt2 solver doesn't have issues
  			if(_values[symIndex][0]==1){
					// std::cout << _objects[symIndex]->name << " = ";
					// std::cout << (int)_values[symIndex][valueIndex] << " ";
					// std::cout << std::endl;
          if(ln == lineID){
  				unsigned i = std::stoi(name.substr(pos1+1,pos2));
					unsigned j = std::stoi(name.substr(pos2+1));
					conflictGraph[i].insert(j);
					conflictGraph[j].insert(i);
				}
			}
    }
		}
	}
	cliqueFinder cf(conflictGraph, line.threadMemAccVec.size(), _spoilerSize);
	Clique spoiler = cf.findClique();
	// #ifdef DEBUG_LINE_EXPLORER
	// std::cout << "Line " << lineID << " spoiler {";
	// std::copy(spoiler.begin(), spoiler.end(), std::ostream_iterator<unsigned>(std::cout, " "));
	// std::cout << "}" << std::endl;
	// #endif
	return spoiler;
}

/* Encodes (line(i)/box_width)!=(line(j)/box_width) but (line(i)/box_width)%bank_size == (line(j)/box_width)%bank_size*/
klee::ref<Expr> LineExplorer::makePairConflict(const Line& line, unsigned i, unsigned j){
	const MemoryAccess mai = line.threadMemAccVec.at(i);
	const MemoryAccess maj = line.threadMemAccVec.at(j);
	klee::ref<Expr> boxWidthE = ConstantExpr::create(BankConflictsSideChannel::BOX_WIDTH, mai.offset.get()->getWidth());
	klee::ref<Expr> bankSizeE = ConstantExpr::create(BankConflictsSideChannel::BANK_SIZE, mai.offset.get()->getWidth());
	klee::ref<Expr> boxIE = UDivExpr::create(mai.offset,boxWidthE);
	klee::ref<Expr> boxJE = UDivExpr::create(maj.offset,boxWidthE);
	klee::ref<Expr> bankIE = URemExpr::create(boxIE,bankSizeE);
	klee::ref<Expr> bankJE = URemExpr::create(boxJE,bankSizeE);
	klee::ref<Expr> sameBank = EqExpr::create(bankIE, bankJE);
	klee::ref<Expr> diffBoxes = NotExpr::create(EqExpr::create(boxIE, boxJE));
	// std::cout << "same bank: " << sameBank << std::endl;
	// std::cout << "diffBoxes: " << diffBoxes << std::endl;
	return AndExpr::create(sameBank, diffBoxes);
}
