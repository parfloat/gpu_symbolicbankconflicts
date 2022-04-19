/*
* Cliques.h
*
* rahmed
*
*/
#ifndef GKLEE_LIB_CORE_CLIQUES_H_
#define GKLEE_LIB_CORE_CLIQUES_H_

#include <set>
#include <map>
#include <vector>

#include <bitset>
#include <algorithm>
#include <iterator>
#include <iostream>

//#include "klee/ExecutionState.h"

typedef std::set<unsigned> Clique;
typedef std::set<Clique> CliqueSet;
typedef std::map<unsigned, Clique> UGraph;

// CliqueSet getEmptySingleton();

bool isEmptySingleton(const CliqueSet& cset);

std::ostream& operator<<(std::ostream& out,const Clique& c);

std::ostream& operator<<(std::ostream& out,const CliqueSet& s);

class cliqueFinder{
	UGraph graph;
	unsigned range;
	unsigned size;

public:
	cliqueFinder(const UGraph& _graph, unsigned _range, unsigned _size);

	Clique findClique();

	Clique explorePath(const Clique& prefix, const Clique& suffixes);

	bool isClique(const Clique& candidate);

};

class CliqueGenerator{
	std::vector<bool> current;
	CliqueSet forbiden;
	unsigned range, size;
	bool empty;

	bool isForbidden(const Clique& clq){
		for(const Clique& fbd: forbiden)
		if(std::includes(clq.begin(), clq.end(),fbd.begin(),fbd.end()))
		return true;
		return false;
	}

public:
	CliqueGenerator():empty(true){}

	CliqueGenerator(unsigned _range, unsigned _size, unsigned _max);

	void reset();

	Clique get()const;

	bool isEmpty()const;

	bool next();

	void forbid(const Clique& clique2forbid);
};



#endif //GKLEE_LIB_CORE_CLIQUES_H_
