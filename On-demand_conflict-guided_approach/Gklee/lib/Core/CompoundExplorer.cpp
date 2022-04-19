/*
 * CompoundExplorer.cpp
 * rahmed
 */

#include <iomanip>

#include "CompoundExplorer.h"

#include "../../include/klee/util/ExprSMTLIBPrinter.h"
#include "../../include/klee/util/ExprSMTLIBLetPrinter.h"
#include <fstream>
#include <string>
int CompoundExplorer::tries = 0;

#define DEBUG_EXPLORER 1

CompoundExplorer::CompoundExplorer(ExecutionState& _st, LineExplorer* _pH, CompoundExplorer* _pT,TimingSolver* _sol)
:compoundState(_st), pHeadExplorer(_pH), pTailCompoundExplorer(_pT), solver(_sol)
{
	for (unsigned SI = 0; SI != compoundState.symbolics.size(); SI++){
		compoundObjects.push_back(compoundState.symbolics[SI].second);
	}
	if(pTailCompoundExplorer){
		rank = 1 + pTailCompoundExplorer->rank;
	}
	recursivelyAddEncoding(compoundState,compoundObjects);
}

void CompoundExplorer::recursivelyAddEncoding(ExecutionState& _st, std::vector<const Array*>& _obs){
	//  std::cout << "-->> Adding encoding for the head explorer" << std::endl;
	pHeadExplorer->addEncoding(_st,_obs);
	if(pTailCompoundExplorer){
		//	  std::cout << "-->> Adding encoding recursively for the tail compound explorer" << std::endl;
		pTailCompoundExplorer->recursivelyAddEncoding(_st,_obs);
	}
}


bool CompoundExplorer::recursivelyEnforce(ExecutionState& cstate, const std::vector<Clique>& cliques, unsigned pos){
	// assert(cliques.size() > pos);
	assert(rank == pos);
	std::cout << "begin recursivelyEnforce at pos " << pos << std::endl;
	assert(cliques[pos].size() <= pHeadExplorer->maxCliqueSize);

	if(pHeadExplorer->storage[cliques[pos].size()].find(cliques[pos]) != pHeadExplorer->storage[cliques[pos].size()].end()){
		CliqueSet toForbidd = pHeadExplorer->storage[cliques[pos].size()].find(cliques[pos])->second;
		std::cout << "continue recursivelyEnforce at 43 " << std::endl;
		for(Clique clique: toForbidd){
			if(!pHeadExplorer->forbidd(cstate, clique)){
				std::cout << "recursivelyenforce at pos=" << pos << " could not encorce " << clique << std::endl;
				return false;
			}
		}
	}
	std::cout << "continue recursivelyEnforce at 47 " << std::endl;
	if(pos == 0){
		return pHeadExplorer->enforce(cstate,cliques[pos]);
	}
	std::cout << "continue recursivelyEnforce at 51 " << std::endl;
	if(!pHeadExplorer->enforce(cstate,cliques[pos])){
		return false;
	}
	std::cout << "continue recursivelyEnforce at 55 " << std::endl;
	return pTailCompoundExplorer->recursivelyEnforce(cstate,cliques,pos-1);
}

// given a clique for each line, add constraints (in the form of forbidden larger cliques)
// untill one answers whether the chosen cliques coexist or not.
bool CompoundExplorer::cliquesArePossibleGlobally(const std::vector<Clique>& cliques){
#ifdef DEBUG_EXPLORER
	std::cout << "begin cliquesArePossibleGlobally at rank " << rank << std::endl;
	std::cout << "with " << cliques.size() << " cliques: " << std::endl;
	for(unsigned i=0; i < cliques.size(); i++)
		std::cout << i << "/" << cliques.size() << " -> "<< cliques[i] << std::endl;
	std::cout << " so far: " << cachedCompoundCliques.size() << " cached vectors globally" << std::endl;
#endif

	assert(!cliques.empty());
	assert((pTailCompoundExplorer == nullptr) == (cliques.size() == 1));
	std::map<std::vector<Clique>,bool>::const_iterator found = cachedCompoundCliques.find(cliques);
	if(found != cachedCompoundCliques.end()){
#ifdef DEBUG_EXPLORER
		std::cout << " sequence of " << cliques.size() << " cliques cached already: " << (found->second? "":"un") << "sat" << std::endl;
#endif
		return found->second;
	}
{
	// if(pTailCompoundExplorer){ //costs too much?
	//   std::vector<Clique> prefix(cliques);
	//   prefix.pop_back();
	//   if(!pTailCompoundExplorer->cliquesArePossibleGlobally(prefix)){
	//     #ifdef DEBUG_EXPLORER
	//     std::cout << " prefix of " << cliques.size() << " cliques is " << (false? "":"un") << "sat" << std::endl;
	//     #endif
	//     return false;
	//   }
	// }
}
	ExecutionState cstate(compoundState);

	if(!recursivelyEnforce(cstate,cliques, rank)){
		cachedCompoundCliques[cliques] = false;
#ifdef DEBUG_EXPLORER
		std::cout << " could not enforce the " << cliques.size() << " cliques " << std::endl;
#endif
		return false;
	}
	std::vector<Clique> spoilers;
	do{
		{
			//trying to dump to smt2 format
			std::string fileName= "./current-constraints-" + std::to_string(tries++) + ".smt2";
			std::cout << "<><><><><><>Writing smt2 contraints to file : " << fileName << std::endl;
			std::ofstream output;
			output.open(fileName, std::ofstream::out);
			Query query(cstate.constraints, ConstantExpr::alloc(0, Expr::Bool));
			ExprSMTLIBPrinter printer;
//			ExprSMTLIBLetPrinter printer;
			printer.setOutput(output);
			printer.setQuery(query);
			printer.setHumanReadable(true);
			printer.generateOutput();
			output.close();
			std::cout << "<><><><><><>End of writing smt2 contraints to file : " << fileName << std::endl;
			//end smt2 dump
		}
		// get a solution with enforced clique and forbidden cliques2bForbidden.
		std::vector< std::vector<unsigned char> > values;
		std::cout << "Asking solver for possible values globally:" << std::endl;
		bool success = solver->getInitialValues(cstate, compoundObjects, values);
		solver->setTimeout(0);
		if(!success){
			#ifdef DEBUG_EXPLORER
			std::cout << "no solutions, failure at rank: " << rank << std::endl;
			#endif
			// remember impossible combination of cliques
			cachedCompoundCliques[cliques] = false;
			#ifdef DEBUG_EXPLORER
			std::cout << " checked the " << cliques.size() << " cliques and turned " << (false? "":"un") << "sat" << std::endl;
			#endif
			return false;
		}
#ifdef DEBUG_EXPLORER
		std::cout << "found global solution at rank " << rank << ", spoilers?" << std::endl;
		for (int symIndex = compoundObjects.size()-1; symIndex >= 0 ; --symIndex) {
			// if(values[symIndex].size()== 1){
			//			if(values[symIndex].size()!= 1 || values[symIndex][0]!=0){
			std::cout << compoundObjects[symIndex]->name << " = ";
			//			std::cout << "(size " << compoundObjects[symIndex]->size << ", little-endian, unsigned char values) ";
			//			for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); ++valueIndex)
			//				std::cout << (unsigned char)values[symIndex][valueIndex] << " ";
			//			std::cout << std::endl;

			std::cout << "(size " << compoundObjects[symIndex]->size << ", little-endian, int values) ";
			for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); ++valueIndex)
				std::cout << (int)values[symIndex][valueIndex] << " ";
			std::cout << std::endl;

			std::cout << "(size " << compoundObjects[symIndex]->size << ", little-endian, long int 4 values) ";
			for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex=valueIndex+4){
				long int integerValue = (int)values[symIndex][valueIndex]
									 + 256 * (int)values[symIndex][valueIndex + 1]
									 + 256 * 256 * (int)values[symIndex][valueIndex + 2]
									 + 256 * 256 * 256 * (int)values[symIndex][valueIndex + 3];
				std::cout << (long int)integerValue << ", ";
			}
			std::cout << std::endl;

			std::cout << "(size " << compoundObjects[symIndex]->size << ", little-endian, int values) ";
						for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); ++valueIndex)
							std::cout << (int)values[symIndex][valueIndex] << " ";
						std::cout << std::endl;

			auto flags = std::cout.flags();
			std::cout << "(size " << compoundObjects[symIndex]->size << ", little-endian, hex values) ";
			for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); ++valueIndex)
				std::cout << "0x" << std::setw(2) << std::setfill('0') << std::hex << (unsigned)values[symIndex][valueIndex] << ", ";
			std::cout << std::endl;
			std::cout.flags(flags);
			//			}
		}
		//	}
#endif
		std::cout << "achoum 130" << std::endl;
		//see if larger cliques are possible
		std::vector<unsigned> spoilersSizes;
		for(unsigned i=0; i < cliques.size(); i++){
			spoilersSizes.push_back(cliques[i].size() + 1);
		}
		spoilers = getSpoilersRecursively(spoilersSizes, rank, compoundObjects, values);
		bool noSpoilers=true;
		for(unsigned i=0; i < spoilers.size(); i++){
#ifdef DEBUG_EXPLORER
			std::cout << "spoiler index " << i << " at rank " << rank << " has size "<< spoilers[i].size() << std::endl;
#endif
			if(!spoilers[i].empty()){
				noSpoilers=false;
			}
		}
		if(noSpoilers){
			// solution has no larger cliques.
			cachedCompoundCliques[cliques] = true;
#ifdef DEBUG_EXPLORER
			std::cout << " no spoilers with the " << cliques.size() << " cliques !"  << " so sat" << std::endl;
#endif
			return true;
		}

#ifdef DEBUG_EXPLORER
		std::cout << "found compund spoilers " << std::endl;
		for(unsigned j=0; j < spoilers.size(); j++){
			std::cout << "spoiler " << j << " at rank " << rank << spoilers[j] <<  std::endl;
		}
#endif
	}while(forbidSpoilersRecursively(cstate,spoilers,rank));
	// impossible to forbid the spoiler.
	// Encode impossibility and return.
	cachedCompoundCliques[cliques] = false;
#ifdef DEBUG_EXPLORER
	std::cout << "could not forbidd spoliers for the " << cliques.size() << " cliques. failure" << std::endl;
#endif
	// getchar();
	return false;
}

bool CompoundExplorer::cliquesDump(const std::vector<Clique>& cliques, int byteIndex, int byteValue, int observationValue){
#ifdef DEBUG_EXPLORER
	std::cout << "begin cliquesDump at rank " << rank << std::endl;
	std::cout << "with " << cliques.size() << " cliques: " << std::endl;
	for(unsigned i=0; i < cliques.size(); i++)
		std::cout << i << "/" << cliques.size() << " -> "<< cliques[i] << std::endl;
	std::cout << " so far: " << cachedCompoundCliques.size() << " cached vectors globally" << std::endl;
#endif

	assert(!cliques.empty());
	assert((pTailCompoundExplorer == nullptr) == (cliques.size() == 1));
	std::map<std::vector<Clique>,bool>::const_iterator found = cachedCompoundCliques.find(cliques);
	if(found != cachedCompoundCliques.end()){
#ifdef DEBUG_EXPLORER
		std::cout << " sequence of " << cliques.size() << " cliques cached already: " << (found->second? "":"un") << "sat" << std::endl;
#endif
		return found->second;
	}

	ExecutionState cstate(compoundState);

	if(!recursivelyEnforce(cstate,cliques, rank)){
		cachedCompoundCliques[cliques] = false;
#ifdef DEBUG_EXPLORER
		std::cout << " could not enforce the " << cliques.size() << " cliques " << std::endl;
#endif
		return false;
	}
	std::vector<Clique> spoilers;
	std::vector< std::vector<unsigned char> > values;

	//trying to dump to smt2 format
	std::string fileName= "./constraints-byteI" + std::to_string(byteIndex) +
			"-byteV" + std::to_string(byteValue) + "-obsV" + std::to_string(observationValue) + ".smt2";
	//			"-"+ std::to_string(tries++)+ ".smt2";

	std::cout << "<><><><><><>Writing smt2 contraints to file : " << fileName << std::endl;
	std::ofstream output;
	output.open(fileName, std::ofstream::out);
	Query query(cstate.constraints, ConstantExpr::alloc(0, Expr::Bool));
	ExprSMTLIBPrinter printer;
	printer.setOutput(output);
	printer.setQuery(query);
	printer.setHumanReadable(true);
	printer.generateOutput();
	output.close();
	std::cout << "<><><><><><>End of writing smt2 contraints to file : " << fileName << std::endl;
	//end smt2 dump

	return true;
}

std::vector<Clique> CompoundExplorer::getSpoilersRecursively(const std::vector<unsigned>& _sizes, unsigned _pos,
		const std::vector< const Array* >& _objects, const std::vector< std::vector<unsigned char> >& _values){
	assert(rank == _pos);
	//#if 0 //DEBUG
	//assert(rank == pHeadExplorer->lineID); //FIXME this should not be the case when analyzing line by line

#ifdef DEBUG

	std::cout << "begin getSpoilersRecursively pos = " << _pos << std::endl;
	for(int i = 0; i < _sizes.size(); i++)
		std::cout << " sizes[" << i << "]=" << _sizes[i];
	std::cout << std::endl;

	// for (int symIndex = _objects.size()-1; symIndex >= 0 ; --symIndex) {
	//   // if(values[symIndex].size()== 1){
	//   if(_values[symIndex].size()!= 1 || _values[symIndex][0]!=0){
	//   std::cout << _objects[symIndex]->name << " = ";
	//   //      std::cout << "(size " << compoundObjects[symIndex]->size << ", little-endian) ";
	//   for (unsigned valueIndex = 0; valueIndex < _values[symIndex].size(); ++valueIndex)
	//   std::cout << (int)_values[symIndex][valueIndex] << " ";
	//   std::cout << std::endl;
	// }
	//  }
#endif

	if(_pos == 0){
		Clique spoiler = pHeadExplorer->getSpoiler(_sizes[_pos], _objects, _values);
		std::vector<Clique> rslt;
		rslt.push_back(spoiler);
		return rslt;
	}
	assert(pTailCompoundExplorer);
	std::vector<Clique> rslt = pTailCompoundExplorer->getSpoilersRecursively(_sizes, _pos-1, _objects, _values);
	Clique spoiler = pHeadExplorer->getSpoiler(_sizes[_pos], _objects, _values);
	rslt.push_back(spoiler);
	return rslt;
}

bool CompoundExplorer::forbidSpoilersRecursively(ExecutionState& cstate, const std::vector<Clique>& spoilers,unsigned pos){
	assert(rank == pos);
	// std::cout << " begin forbidSpoilersRecursively at rank " << rank << " pos " << pos  << " spoilers.size() " << spoilers.size() << std::endl;
	// for(unsigned i=0; i < spoilers.size(); i++){
	// std::cout << "spoiler " << i << "->";
	// std::copy(spoilers[i].begin(), spoilers[i].end(), std::ostream_iterator<unsigned>(std::cout, " "));
	// std::cout << std::endl;
	// }
	if(pos == 0){
		bool result = spoilers[pos].empty() || pHeadExplorer->forbidd(cstate,spoilers[pos]);
		// std::cout << "result forbidSpoilersRecursively at rank " << rank << " pos " << pos  << " is " << result  << std::endl;
		return result;
	}
	if(!spoilers[pos].empty())
		if(!pHeadExplorer->forbidd(cstate,spoilers[pos])){
			// std::cout << "result forbidSpoilersRecursively at rank " << rank << " pos " << pos  << " is " << false  << std::endl;
			return false;
		}
	assert(pTailCompoundExplorer);
	bool result = pTailCompoundExplorer->forbidSpoilersRecursively(cstate,spoilers,pos-1);
	// std::cout << "result forbidSpoilersRecursively at rank " << rank << " pos " << pos  << " is " << result  << std::endl;
	return result;
}



// uses arguments about sizes of cntributions of head and tails given maxnumber of arbuments to recursively result in
// a contribution per level (larger heads, smaller tails).
//enumberation then tries to increase tail contributions untill there is no room for head contributions.
//returns true if settles on a contribution par level that would result in target, otherwise false.
//No solver queries are involved.

CompoundExplorer::TargetedCompoundCliques::TargetedCompoundCliques(CompoundExplorer* _pCmpdExpl, unsigned _target)
:pCmpdExpl(_pCmpdExpl), target(_target){
	if(pCmpdExpl->rank == 0){
		headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
		empty = headClq.isEmpty();
		return;
	}

	tailContr = pCmpdExpl->rank;
	if(target - tailContr > pCmpdExpl->pHeadExplorer->getMaxCliqueSize()){
		tailContr = target - pCmpdExpl->pHeadExplorer->getMaxCliqueSize();
	}

	while(tailContr < target){
		headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
		pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer, tailContr);
		if(!headClq.isEmpty() && !pTailClqs->isEmpty())
			return;
		tailContr++;
	};

	empty= true;
}

bool CompoundExplorer::TargetedCompoundCliques::evaluate(){
	// if(empty || headClq.isEmpty())
	//   return false;
	// if(!pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
	//     headClq.forbid(headClq.get());
	//     return false;
	// }evaluate(

	std::cout << "253 evaluate(): " << headClq.get() << std::endl;
	///////// while locally impossible advance cliques ///////
	if(pTailClqs){
		do{
			while(!headClq.isEmpty() && !pTailClqs->isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
				headClq.forbid(headClq.get());
				headClq.next();
			};
			if(headClq.isEmpty() || pTailClqs->isEmpty()){
				if(target <= 1 + tailContr){
					empty=true;
					return false;
				}
				tailContr++;
				headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
				delete pTailClqs;
				pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer , tailContr);
			}else{
				//found a locally possible headClq!
				break;
			}
		}while(true);
	}else{
		while(!headClq.isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
			headClq.forbid(headClq.get());
			headClq.next();
		};
		if(headClq.isEmpty()){
			empty=true;
			return false;
		}
	}
	/////////////headClq possible locally//////////////////

	std::cout << "286 evaluate() check globally: " << headClq.get() << std::endl;
	return pCmpdExpl->cliquesArePossibleGlobally(getCliques());
}

bool CompoundExplorer::TargetedCompoundCliques::tryDump(int byteIndex, int byteValue, int observationValue){
	//	std::cout << "325 tryDump(): " << headClq.get() << std::endl;
	///////// while locally impossible advance cliques ///////
	if(pTailClqs){
		do{
			while(!headClq.isEmpty() && !pTailClqs->isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
				headClq.forbid(headClq.get());
				headClq.next();
			};
			if(headClq.isEmpty() || pTailClqs->isEmpty()){
				if(target <= 1 + tailContr){
					empty=true;
					return false;
				}
				tailContr++;
				headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
				delete pTailClqs;
				pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer , tailContr);
			}else{
				break;
			}
		}while(true);
	}else{
		while(!headClq.isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
			headClq.forbid(headClq.get());
			headClq.next();
		};
		if(headClq.isEmpty()){
			empty=true;
			return false;
		}
	}
	/////////////headClq possible locally//////////////////

	std::cout << "358 tryDump() check globally: " << headClq.get() << std::endl;
	return pCmpdExpl->cliquesDump(getCliques(), byteIndex, byteValue, observationValue);//cliquesArePossibleGlobally(getCliques());
}

CompoundExplorer::TargetedCompoundCliques::~TargetedCompoundCliques(){
	if(pTailClqs)
		delete pTailClqs;
}

// bool CompoundExplorer::TargetedCompoundCliques::isSomeEmpty()const{
//   if(headClq.isEmpty())
//     return true;
//   if(pTailClqs)
//     return pTailClqs->isSomeEmpty();
//   return false;
// }

// reset:


std::vector<Clique> CompoundExplorer::TargetedCompoundCliques::getCliques()const{
	std::vector<Clique> rslt;
	if(pTailClqs) rslt = pTailClqs->getCliques();
	Clique clique = headClq.get(); assert(!headClq.isEmpty());
	rslt.push_back(clique);
	assert(rslt.size() == 1 + pCmpdExpl->rank);
	return rslt;
}

// std::vector<Clique> CompoundExplorer::TargetedCompoundCliques::getCliques(){
//   std::vector<Clique> rslt;
//   if(pTailCompoundExplorer){
//       rslt = pTailCompoundExplorer->getCliques();
//   }
//   assert(!headClq.isEmpty());
//   rslt.push_back(headClq.get());
//
//   return rslt;
// }


bool CompoundExplorer::TargetedCompoundCliques::isEmpty()const{
	// if(empty || headClq.isEmpty() || (pTailClqs && pTailClqs->isEmpty()){
	//   empty=true;
	// }
	return empty;
}

//uncheckedGeneratedNext : returns true if found a next combination of conflicts (not necessarily sat).
// returns false if no more combinations exist.
bool CompoundExplorer::TargetedCompoundCliques::uncheckedGeneratedNext(){

	std::cout << "next at rank " << pCmpdExpl->rank << std::endl;
	if(empty)	{
		return false;
	}


	///////// while locally impossible advance cliques ///////
	if(pTailClqs){
		do{
			std::cout << "367: " << headClq.get() << std::endl;
			while(!headClq.isEmpty() && !pTailClqs->isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
				headClq.forbid(headClq.get());
				headClq.next();
			};
			if(headClq.isEmpty() || pTailClqs->isEmpty()){
				if(target <= 1 + tailContr){
					empty=true;
					return false;
				}
				tailContr++;
				headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
				delete pTailClqs;
				pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer , tailContr);
			}else{
				break;
			}
		}while(true);
	}else{
		while(!headClq.isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
			headClq.forbid(headClq.get());
			headClq.next();
		};
		if(headClq.isEmpty()){
			empty=true;
			return false;
		}
	}
	/////////////headClq possible locally//////////////////
	assert(!headClq.isEmpty());
	//11 + 1
	if(pTailClqs){
		bool moveOn;
		do{
			moveOn=false;
			//      if(pTailClqs->evaluate()){
			while(headClq.next()){
				if(!pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
					headClq.forbid(headClq.get());
					continue;
				}
				//          if(pCmpdExpl->cliquesArePossibleGlobally(getCliques())){
				std::cout << "409: " << headClq.get() << std::endl;
				return true;
				//          }
			};
			//      }
			headClq.reset();
			if(!pTailClqs->uncheckedGeneratedNext() || headClq.isEmpty()){
				tailContr++;
				if(target <= tailContr){
					empty = true;
					return false;
				}
				headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
				delete pTailClqs;
				pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer , tailContr);
				if(pTailClqs->isEmpty() || headClq.isEmpty())
					moveOn= true;
			}
		}while(moveOn); // || !pCmpdExpl->cliquesArePossibleGlobally(getCliques())
		std::cout << "428: " << headClq.get() << std::endl;
		return true;
	}else{
		std::cout << "****" << std::endl;
		while(headClq.next()){
			// std::cout << "checking ";
			// Clique clique = headClq.get();
			// std::copy(clique.begin(), clique.end(), std::ostream_iterator<unsigned>(std::cout, " "));
			if(!pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
				headClq.forbid(headClq.get());
				continue;
			}
			//      if(pCmpdExpl->cliquesArePossibleGlobally(getCliques())){
			assert(!headClq.isEmpty());
			std::cout << "442: " << headClq.get() << std::endl;
			return true;
			//      }
		};
		empty = true;
		return false;
	}
}

//lazy next: evaluates if the uncheckedGeneratedNext() clique is possible
bool CompoundExplorer::TargetedCompoundCliques::lazyNext(){

	//	while(uncheckedGeneratedNext()){
	//		//if(evaluate()){
	//			if(pCmpdExpl->cliquesArePossibleGlobally(getCliques())){
	//				return true;
	//			}
	//		//}
	//	}
	if(uncheckedGeneratedNext()){
		return true;
	}
	return false;
}

//eager next: returns next FEASIBLE conflict combination
bool CompoundExplorer::TargetedCompoundCliques::next(){

	std::cout << "next at rank " << pCmpdExpl->rank << std::endl;
	if(empty)	{
		return false;
	}


	///////// while locally impossible advance cliques ///////
	if(pTailClqs){
		do{
			std::cout << "354: " << headClq.get() << std::endl;
			while(!headClq.isEmpty() && !pTailClqs->isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
				headClq.forbid(headClq.get());
				headClq.next();
			};
			if(headClq.isEmpty() || pTailClqs->isEmpty()){
				if(target <= 1 + tailContr){
					empty=true;
					return false;
				}
				tailContr++;
				headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
				delete pTailClqs;
				pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer , tailContr);
			}else{
				break;
			}
		}while(true);
	}else{
		while(!headClq.isEmpty() && !pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
			headClq.forbid(headClq.get());
			headClq.next();
		};
		if(headClq.isEmpty()){
			empty=true;
			return false;
		}
	}
	/////////////headClq possible locally//////////////////

	//11 + 1
	if(pTailClqs){
		bool moveOn;
		do{
			moveOn=false;
			if(pTailClqs->evaluate()){
				while(headClq.next()){
					if(!pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
						headClq.forbid(headClq.get());
						continue;
					}
					if(pCmpdExpl->cliquesArePossibleGlobally(getCliques())){
						return true;
					}
				};
			}
			headClq.reset();
			if(!pTailClqs->next() || headClq.isEmpty()){
				tailContr++;
				if(target <= tailContr){
					empty = true;
					return false;
				}
				headClq = CliqueGenerator(pCmpdExpl->pHeadExplorer->getRange(), target - tailContr, pCmpdExpl->pHeadExplorer->getMaxCliqueSize());
				delete pTailClqs;
				pTailClqs = new TargetedCompoundCliques(pCmpdExpl->pTailCompoundExplorer , tailContr);
				if(pTailClqs->isEmpty() || headClq.isEmpty())
					moveOn= true;
			}
		}while(moveOn || !pCmpdExpl->cliquesArePossibleGlobally(getCliques()));
		return true;
	}else{
		std::cout << "****" << std::endl;
		while(headClq.next()){
			// std::cout << "checking ";
			// Clique clique = headClq.get();
			// std::copy(clique.begin(), clique.end(), std::ostream_iterator<unsigned>(std::cout, " "));
			if(!pCmpdExpl->pHeadExplorer->makeCliquePossibleLocally(headClq.get())){
				headClq.forbid(headClq.get());
				continue;
			}
			if(pCmpdExpl->cliquesArePossibleGlobally(getCliques())){
				return true;
			}
		};
		empty = true;
		return false;
	}
}
