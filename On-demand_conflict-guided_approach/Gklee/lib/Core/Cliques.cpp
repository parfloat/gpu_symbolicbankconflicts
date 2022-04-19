
#include <set>


#include "Cliques.h"


// CliqueSet getEmptySingleton(){
// 	CliqueSet rslt;
// 	rslt.insert(Clique());
// }

bool isEmptySingleton(const CliqueSet& cset){
	if(cset.size() != 1) return false;
	return cset.find(Clique()) != cset.end();
}


std::ostream& operator<<(std::ostream& out,const Clique& c){
  out << "{";
  std::copy(c.cbegin(), c.cend(), std::ostream_iterator<unsigned>(out, " "));
  out << "}";
	return out;
}


std::ostream& operator<<(std::ostream& out,const CliqueSet& s){
  out << "[" << std::endl;
  for(const Clique& c: s){
    out << c << std::endl;
  }
  out << "]";
	return out;
}



cliqueFinder::cliqueFinder(const UGraph& _graph, unsigned _range, unsigned _size):graph(_graph), range(_range), size(_size){
	// repeatedly exclude nodes with less than (CLIQUE_SIZE - 1) neighboors
	Clique exclude;
	do{
		exclude.clear();
		for(unsigned i=1; i <= range; ++i)
		if(graph[i].size()+1 < size && !graph[i].empty())
		exclude.insert(i);
		for(unsigned i=1; i <= range; ++i){
			Clique diff;
			set_difference(graph[i].begin(), graph[i].end(), exclude.begin(), exclude.end(), std::inserter(diff, diff.end()));
			graph[i]=diff;
		}
		for(unsigned e: exclude)
		graph[e].clear();
	}while(!exclude.empty());
}

Clique cliqueFinder::findClique(){
	Clique rslt;
	for(unsigned i=1; i <= range; ++i){
		Clique prefix;
		prefix.insert(i);
		rslt= explorePath(prefix,graph[i]);
		if(!rslt.empty())
		return rslt;
	}
	rslt.clear();
	return rslt;
}

Clique cliqueFinder::explorePath(const Clique& prefix, const Clique& suffixes){
	// invariant: prefix are pairwise neighbours
	// invariant: each suffix is neighbours to each prefix,
	// cout << endl << "prefix: ";
	// copy(prefix.begin(), prefix.end(), ostream_iterator<unsigned>(cout, " "));
	// cout << endl << "suffixes: ";
	// copy(suffixes.begin(), suffixes.end(), ostream_iterator<unsigned>(cout, " "));
	// cout << endl;

	if(prefix.size() == size)
	return prefix; //success
	Clique unionset;
	Clique xprefix(prefix.begin(), prefix.end());
	set_union(prefix.begin(), prefix.end(), suffixes.begin(), suffixes.end(),inserter(unionset,unionset.end()));
	if(unionset.size() < size){
		unionset.clear();
		return unionset; //failure
	}
	// try extend prefix with first suffix
	unsigned candidate = *suffixes.begin();
	xprefix.insert(candidate);
	Clique xsuffixes;
	//remaining suffixes need to be neighbours with candidate
	set_intersection(++suffixes.begin(), suffixes.end(), graph[candidate].begin(), graph[candidate].end(),inserter(xsuffixes,xsuffixes.end()));
	Clique rslt = explorePath(xprefix,xsuffixes);
	if(rslt.size() == size)
	return rslt;
	//try bypassing first suffix instead
	xsuffixes.clear();
	xsuffixes.insert(++suffixes.begin(), suffixes.end());
	return explorePath(xprefix,xsuffixes);
}

bool cliqueFinder::isClique(const Clique& candidate){
	for(unsigned c: candidate){
		for(unsigned n: candidate)
		if(c != n && graph[c].find(n)==graph[c].end())
		return false;
	}
	return true;
}


CliqueGenerator::CliqueGenerator(unsigned _range, unsigned _size, unsigned _maxCliqueSize):
	range(_range), size(_size), empty(size > range || size > _maxCliqueSize){
		//max conflicts represents the max number of conflicting addresses belonging to pairwise different banks.
	current = std::vector<bool>(range);
	reset();
}

void CliqueGenerator::reset(){
	for(unsigned i=0; i < range; ++i)
	current[i]=(i < size ? 1 : 0);
	if(isForbidden(get()))
	next();
}

Clique CliqueGenerator::get()const{
	//assert(!empty);
	Clique rslt;
	for(unsigned i=0; i < range; ++i)
	if(current[i])
	rslt.insert(i);
	return rslt;
}

bool CliqueGenerator::isEmpty()const{
	return empty;
}

bool CliqueGenerator::next(){
	if(empty)
	return false;
	Clique rslt;
	// do{
	unsigned cnt=0;
	for(unsigned i=0; i+1 < range; ++i){
		if(current[i]){
			if(current[i+1]){
				++cnt;
			}else{
				current[i].flip();
				current[i+1].flip();
				for(unsigned j=0; j < i; ++j)
				current[j] = (j < cnt? 1 : 0);
				rslt = get();
				return true;
			}
		}
	}
	// }while(isForbidden(rslt) && !rslt.empty());

	for(unsigned i=range-size; i < size; ++i)
	current[i] = 0;
	empty = true;
	return false;
}

void CliqueGenerator::forbid(const Clique& clique2forbid){
	CliqueSet entailed;
	for(const Clique& fbd: forbiden){
		if(std::includes(clique2forbid.begin(), clique2forbid.end(), fbd.begin(), fbd.end()))
			return;
		if(std::includes(fbd.begin(), fbd.end(), clique2forbid.begin(), clique2forbid.end()))
			entailed.insert(fbd);
	}
	for(Clique fbd: entailed)
		forbiden.erase(fbd);
	forbiden.insert(clique2forbid);
}

// CompoundCliqueGenerator::CompoundCliqueGenerator(const std::vector<CliqueGenerator>& _components):components(_components){
// 	for(const CliqueGenerator& cgen: components)
// 	if(cgen.isEmpty())
// 	empty=true;
// }
//
// void CompoundCliqueGenerator::reset(){
// 	for(CliqueGenerator& cgen: components)
// 	cgen.reset();
// }
//
// bool CompoundCliqueGenerator::isEmpty()const{return empty;}
//
// bool CompoundCliqueGenerator::next(){
// 	if(empty) return false;
// 	for(unsigned i=0; i < components.size(); ++i){
// 		if(components[i].next()){
// 			for(unsigned j=0; j < i; ++j)
// 			components[j].reset();
// 			return true;
// 		}
// 	}
// 	return false;
// }
