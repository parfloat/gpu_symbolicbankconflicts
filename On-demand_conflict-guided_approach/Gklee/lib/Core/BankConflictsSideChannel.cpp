/*
 * BankConflictsSideChannel.cpp
 *
 * adrianh
 */

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <set>

#include "BankConflictsSideChannel.h"
#include "LineExplorer.h"
#include "CompoundExplorer.h"
#include "TimerHelper.h"

#include "../../include/klee/util/ExprSMTLIBPrinter.h"
#include "../../include/klee/util/ExprSMTLIBLetPrinter.h"
#include <fstream>
#include <string>

// #include "Cliques.h"


// using namespace klee;

unsigned BankConflictsSideChannel::BANK_SIZE = 32;
unsigned BankConflictsSideChannel::BOX_WIDTH = 4;

ModelData BankConflictsSideChannel::readLines;
ModelData BankConflictsSideChannel::writeLines;

klee::ref<Expr> getDifference(MemoryAccess& ma1, MemoryAccess& ma2){
	auto& offset1 = ma1.offset;
	auto& offset2 = ma2.offset;
	auto sub = SubExpr::create(offset1, offset2);

	//	std::cout << "sub expr : ";
	//	sub->print(std::cout);
	//	std::cout << std::endl;

	return sub;
}


klee::ref<Expr> lineEquivalenceCompare(Line& line1, Line& line2){
	if ((line1.threadMemAccVec.size() == 1) && (line2.threadMemAccVec.size() == 1)){
		return ConstantExpr::create(true, Expr::Bool);
	}
	if (line1.threadMemAccVec.size() != line2.threadMemAccVec.size()){
		return ConstantExpr::create(false, Expr::Bool);
	}

	klee::ref<Expr> finalExpr;
	auto initial = getDifference(line1.threadMemAccVec[0], line2.threadMemAccVec[0]);

	//	std::cout << "initial expr : ";
	//	initial->print(std::cout);
	//	std::cout << std::endl;

	for (int i = 1 ; i < line1.threadMemAccVec.size() ; i++){
		auto toCompare = getDifference(line1.threadMemAccVec[i], line2.threadMemAccVec[i]);
		auto equality = EqExpr::create(initial, toCompare);

		if (finalExpr.isNull()){
			finalExpr = equality;
		}else{
			finalExpr = AndExpr::create(finalExpr, equality);
		}

		//		std::cout << "finalExpr expr : ";
		//		finalExpr->print(std::cout);
		//		std::cout << std::endl;
	}

	return finalExpr;
}

void ModelData::computeEquivalenceClasses(ExecutionState& state, TimingSolver* solver){
	//std::vector<int> equivalenceClasses(data.lines.size());
	std::vector<int> initializeVect(lines.size(), -1);
	//std::vector<int> initializeVect={0,0,2,3,4,5,6,7,8,0,2,3,4,13,14,15,16,0,2,3,4,21,22,23,24}; //,0,2,3,4,29,30,31,32,0,2,3,4,37,38,39,40,0,2,3,4,45,46,47,48};
	equivalenceClasses = initializeVect;

	//initially the lines belong to no equivalence class
	//	for (int i = 0; i < equivalenceClasses.size(); i++){
	//		equivalenceClasses[i] = -1;
	//	}

	for (unsigned i = 0 ; i < equivalenceClasses.size(); i++){
		if (equivalenceClasses[i] == -1){ //it was not equal to any other line before, then it belongs in its own class
			std::cout << "New equivalence class at position " << i << std::endl;
			equivalenceClasses[i] = i;
			for (unsigned j = i + 1; j < equivalenceClasses.size(); j++){
				if (equivalenceClasses[j] == -1){
					bool res;
					/////////////////
					//					std::cout << "\nAsking solver weird questionbs" << std::endl;
					//					ExecutionState ccstate(state);
					//					klee::ref<Expr> formula;
					//					std::cout << "Line 0" << std::endl;
					//					lines[0].threadMemAccVec[0].offset->print(std::cout);
					//					std::cout << "Line 8" << std::endl;
					//					lines[8].threadMemAccVec[0].offset->print(std::cout);
					//					formula = EqExpr::create(lines[0].threadMemAccVec[0].offset, lines[4].threadMemAccVec[0].offset);
					//					bool success11 = solver->mustBeTrue(ccstate, formula, res);
					//					std::cout << "Result is done" << std::endl;
					//					assert(false);

					///////////////////////

					std::cout << "Comparing line " << i << " with line " << j << std::endl;
					klee::ref<Expr> compare = lineEquivalenceCompare(lines[i], lines[j]);
					//					std::cout << "compare expr : ";
					//					compare->print(std::cout);
					//					std::cout << std::endl;
					auto notEqual = NotExpr::create(compare);
					//					std::cout << "notEqual expr : ";
					//					notEqual->print(std::cout);
					//					std::cout << std::endl;


					//					std::cout << "Asking solver if lines are the same " << std::endl;
					//					state.constraints.dump();
					//					std::cout << "Printing constraints: " << state.constraints.size() <<  std::endl;
					//					for (const auto &c : state.constraints){
					//						c->print(std::cout);
					//						std::cout << std::endl;
					//					}
					//					std::cout << "Printing compare expression " << std::endl;
					//					notEqual->print(std::cout);
					//					std::cout << std::endl;
					std::cout << "Asking solver if lines are the same " << std::endl;
					//					std::string fileName = "currentConstraints.smt2";
					//					std::cout << "<><><><><><>Writing smt2 contraints to file : " << fileName << std::endl;
					ExecutionState cstate(state);
					//					cstate.addConstraint(notEqual);
					//					std::ofstream output;
					//					output.open(fileName, std::ofstream::out);
					//					Query query(cstate.constraints, ConstantExpr::alloc(0, Expr::Bool));
					//										ExprSMTLIBPrinter printer;
					//					printer.setOutput(output);
					//					printer.setQuery(query);
					//					printer.setHumanReadable(true);
					//					printer.generateOutput();
					//					output.close();
					//					std::cout << "<><><><><><>End of writing smt2 contraints to file : " << fileName << std::endl;
					bool success = false;
//										std::cout << "\nSimplify expression " << std::endl;
//										notEqual->print(std::cout);
					auto notEqualSimplified = cstate.constraints.simplifyExpr(notEqual);
//										std::cout << "\nSimplified expression " << std::endl;
//										notEqualSimplified->print(std::cout);
					std::cout << "\nChecking if constant " << std::endl;
					if(notEqualSimplified->getKind()==Expr::Constant){
						std::cout << "\nExpression is constant " << std::endl;
						notEqualSimplified->print(std::cout);
						if(cast<ConstantExpr>(notEqualSimplified)->isFalse()){
							success = true;
							res = true; //if notEqual=false then equal=true, so it is true that the lines are identical
						}
					}
					if (success == false){
						//						std::string fileName = "compareConstraints.smt2";
						//						std::cout << "<><><><><><>Writing smt2 contraints to file : " << fileName << std::endl;
						//						std::cout << "\nCreating new state " << std::endl;
						//						ExecutionState cstate(state);
						//						std::cout << "\nAdd notEqual constraint " << std::endl;
						//						cstate.addConstraint(notEqual);
						//						std::ofstream output;
						//						std::cout << "\nOpen file " << std::endl;
						//						output.open(fileName, std::ofstream::out);
						//						std::cout << "\nCreate query " << std::endl;
						//						Query query(cstate.constraints, ConstantExpr::alloc(0, Expr::Bool));
						//						//						Query query(cstate.constraints, notEqual);
						//												ExprSMTLIBPrinter printer;
						//						//						ExprSMTLIBLetPrinter printer;
						//						std::cout << "\nSetting printer properties " << std::endl;
						//						std::cout << "\nSet output " << std::endl;
						//						printer.setOutput(output);
						//						std::cout << "\nSet query " << std::endl;
						//												printer.setQuery(query);
						//						std::cout << "\nSet human readable " << std::endl;
						//						printer.setHumanReadable(true);
						//						std::cout << "\nGenerating output " << std::endl;
						//						printer.generateOutput();
						//						std::cout << "\nClose file " << std::endl;
						//						output.close();
						//						std::cout << "<><><><><><>End of writing smt2 contraints to file : " << fileName << std::endl;
						std::cout << "\nAsking solver " << std::endl;
						success = solver->mustBeFalse(state, notEqual, res);
					}
					std::cout << "\nSuccess = " << success << " | res = " << res << std::endl;

					//					if (notEqual->getKind() == Expr::Constant){
					//						if (cast<ConstantExpr>(notEqual)->isFalse()){
					//							std::cout << "Line " << i << " is equivalent to line " << j << std::endl;
					//							equivalenceClasses[j] = equivalenceClasses[i];
					//						}
					//					}else{
					//						ExecutionState cState(state);
					//						cState.addConstraint(notEqual);
					//
					//						std::vector< std::vector<unsigned char> > values;
					//						bool success = true;
					//						std::vector<const Array*> objects;
					//						for (unsigned SI = 0; SI != cState.symbolics.size(); SI++) {
					//							std::cout << "\n\n -> encountered symbolic object \"" << cState.symbolics[SI].first->name << "\"" << std::endl;
					//							std::cout << "address = " << cState.symbolics[SI].first->address << std::endl;
					//							objects.push_back(cState.symbolics[SI].second);
					//						}
					//						success = solver->getInitialValues(cState, objects, values);
					//
					//						std::cout << "success = " << success << std::endl;
					//						if (success){
					//							std::cout << "\n\n..... printing one solution for observation constraint .....\n" << std::endl;
					//
					//							for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
					//								std::cout << objects[symIndex]->name << " = ";
					//								std::cout << "(size " << objects[symIndex]->size << ", little-endian) ";
					//								for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++)
					//									std::cout << (int)values[symIndex][valueIndex] << " ";
					//								std::cout << std::endl;
					//							}
					//						}else{
					//							std::cout << "Line " << i << " is equivalent to line " << j << std::endl;
					//							equivalenceClasses[j] = equivalenceClasses[i];
					//						}
					//
					//					}
					if (res){
						std::cout << "Line " << i << " is equivalent to line " << j << std::endl;
						equivalenceClasses[j] = equivalenceClasses[i];
					}
				}
			}
		}
	}


	std::cout << "Printing equivalence vector : " << std::endl;
	for (int i = 0 ; i < equivalenceClasses.size(); i++){
		std::cout << equivalenceClasses[i] << " ";
	}
	std::cout << std::endl;

}

void ModelData::computeUniqueLines(ExecutionState& state, TimingSolver* solver){

	computeEquivalenceClasses(state, solver); //equivalence classes

	std::cout << "Creating unique lines " << std::endl;
	int currentClass = -1;
	int index = 0;
	for (int i = 0 ; i < equivalenceClasses.size(); i++){
		int classID = equivalenceClasses[i];
		std::cout << "Class ID " << classID << std::endl;
		if (uniqueLines.find(classID) == uniqueLines.end()){
			std::cout << "New ID found, adding to map" << std::endl;
			UniqLine uniqLine;
			uniqLine.head = &lines[i];
			uniqLine.same.push_back(uniqLine.head);
			uniqueLines[classID] = uniqLine;
		}else{
			std::cout << "Adding current ID to existing map" << std::endl;
			Line *p = &lines[i];
			uniqueLines[classID].same.push_back(p);
		}
	}

	for (auto &elem : uniqueLines){
		std::cout << "Equivalence class index " << elem.first << std::endl;
		std::cout << "Head " << elem.second.head << std::endl;
		std::cout << elem.second.getFactor() << " lines are equivalent" << std::endl;
		for (auto& line : elem.second.same){
			std::cout << line << std::endl;
		}
	}

}

void BankConflictsSideChannel::computeModel(ExecutionState& state, TimingSolver* solver, unsigned pathNum){
	//reset the previous values, if any
	resetModel();

	//print gathered data from the state
	//std::string readPath = "READ_LINE_SHARED_" + std::to_string(pathNum) + ".log";
	// printLineAccesses(readPath, state.addressSpace.lineReadSharedLines);
	// std::string writePath = "WRITE_LINE_SHARED_" + std::to_string(pathNum) + ".log";
	// printLineAccesses(writePath, state.addressSpace.lineWriteSharedLines);

	// //build the lines, from the read memory accesses
	if (state.addressSpace.lineReadSharedLines.size() > 0){
		buildInternalDyStructure(readLines, state.addressSpace.lineReadSharedLines, "rd");
		buildDyConstraints(readLines);

		readLines.computeUniqueLines(state, solver); //equivalence classes

		solveUniqueLineDyConstraints(state, solver, readLines); //solving possible values line by line
		//generateConstraintFiles(state, solver, readLines);

		// std::string readCstrsPath = "READ_CONSTRAINTS_" + std::to_string(pathNum) + ".log";
		//printConstraints(readCstrsPath, readLines);
		//solveAllDyConstraints(state, solver, readLines); //uncomment when needing to solve the model
	}


	//build the lines, from the write memory accesses
	// if (state.addressSpace.lineWriteSharedLines.size() > 0){
	// 	buildInternalDyStructure(writeLines, state.addressSpace.lineWriteSharedLines, "wr");
	// 	buildDyConstraints(writeLines);
	// 	writeLines.computeUniqueLines(state, solver); //equivalence classes
	// 	solveUniqueLineDyConstraints(state, solver, writeLines); //solving possible values line by line
	// // 	std::string writeCstrsPath = "WRITE_CONSTRAINTS_" + std::to_string(pathNum) + ".log";
	// // 	printConstraints(writeCstrsPath, writeLines);
	// // 	//solveAllConstraints(state, solver, writeLines);
	// }

}



/* Solve all constraints to get models of secret keys. Each solution is a model for the secret key.
 * The observational constraint is added first (e.g. count-based or seq-based) and the solver is
 * called iteratively */
int BankConflictsSideChannel::getConflicts(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound) {
	std::cout << "starting get conflicts" << std::endl;


	std::vector<LineExplorer*> explorers;
	std::vector<CompoundExplorer*> cmpExplorers;



	for (unsigned lineID=0; lineID < data.lines.size(); ++lineID){
		std::cout << "pushing line: " << lineID << std::endl;

		explorers.push_back(new LineExplorer(state, solver, lineID, data.lines[lineID]));
		if(lineID == 0){
			cmpExplorers.push_back(new CompoundExplorer(state, explorers[lineID], nullptr,solver));
		}else{
			cmpExplorers.push_back(new CompoundExplorer(state, explorers[lineID],cmpExplorers[lineID-1],solver));
		}
	}
	do{
		std::cout << "how many transactions? " << std::endl;
		unsigned tr_numb;
		std::cin >> tr_numb;

		if(tr_numb == 0)
			break;

		CompoundExplorer::TargetedCompoundCliques tcc(cmpExplorers[data.lines.size()-1], tr_numb);

		if(!tcc.evaluate()){
			std::cout << "tcc.next? " <<  tcc.next() << "-><-"<< std::endl;
		}

		if(!tcc.isEmpty()){
			std::vector<Clique> cliques = tcc.getCliques();
			for(unsigned i=0; i < cliques.size(); ++i){
				std::cout << "i=" << i << " : ";
				std::copy(cliques[i].begin(), cliques[i].end(), std::ostream_iterator<unsigned>(std::cout, " "));
				std::cout << std::endl;
			}
			getchar();
			//tcc.next();
		}
	}while(true);

	//TODO: debug the crash when deleting pointer in cmpExplorers and explorers
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	//   delete cmpExplorers[line - 1];
	// }
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	// 	delete explorers[line - 1];
	// }

	return 0;
}
////////////////



/* Solve all constraints to get models of secret keys. Each solution is a model for the secret key.
 * The observational constraint is added first (e.g. count-based or seq-based) and the solver is
 * called iteratively */
int BankConflictsSideChannel::solveAllDyConstraints(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound) {
	getConflicts(state,solver,data,bound);

	unsigned nbsolutions=0;

	return nbsolutions;
}



void BankConflictsSideChannel::resetModel(){
	readLines.clear();
	writeLines.clear();
}

void BankConflictsSideChannel::buildInternalDyStructure(ModelData &data, const ShLineAccType &accesses, std::string uniqueID){

	// unsigned lineID = 0;
	for (const auto &kernel : accesses){
		for (const auto &bi : kernel.second){
			for(const auto &shMem : bi.second){
				for(const auto &mav : shMem.second){
					//unsigned lineID = readLines.size();
					Line line;
					//mem accesses in lockstep fashion
					line.threadMemAccVec = mav;

					//get size of shared memory and determine boxes, all the accesses should access the same shared memory
					const auto &mo = mav.at(0).mo;
					unsigned shMemSizeBytes = mo->size;
					line.shMemBytes = shMemSizeBytes;

					std::cout << "shjared memory size: " << shMemSizeBytes << std::endl;



					//add the new created line to the list
					data.lines.push_back(line);
					// lineID++;
				}
			}
		}
	}

	// data.maxSum = data.lines.size() * BANK_SIZE;
	// // initialize table where all expressions giving the partial sums are stored
	// // first, a dummy zero initial raw and one raw for each line
	// std::vector<klee::ref<Expr>> falseColumn(1+data.lines.size(),ConstantExpr::create(false,Expr::Bool));
	// // one column for each partial sum, from 0 to data.maxSum
	// data.table=std::vector<std::vector<klee::ref<Expr>>>(1+data.maxSum,falseColumn);
	// data.table[0][0]=ConstantExpr::create(true,Expr::Bool);

}



void BankConflictsSideChannel::buildDyConstraints(ModelData &data){
	// // incrementally fill the data.table
	// // iteration line fills table[line+1] with contribution of data.lines[line].
	for (unsigned line=0; line < data.lines.size(); ++line){
		std::cout << "line " << line << " has " << data.lines[line].threadMemAccVec.size() << " memory accesses." << std::endl;
		// 	Line currentLine = data.lines[line];
		// 	std::vector<klee::ref<Expr>> transactions = transactionsOf(currentLine);
		// 	for(unsigned sum=0; sum <= data.maxSum; ++sum){
		// 		// currentLine participates with 0
		// 		data.table[sum][line+1] = AndExpr::create(data.table[sum][line],transactions[0]);
		// 		for(unsigned k=1; k < transactions.size(); ++k){
		// 			//currentLine participates with k
		// 			if(k <= sum){
		// 				unsigned prevSum = sum - k;
		// 				klee::ref<Expr> aBranch = AndExpr::create(data.table[prevSum][line],transactions[k]);
		// 				if(k+1 < transactions.size())
		// 					aBranch = AndExpr::create(aBranch,NotExpr::create(transactions[k+1]));
		// 				data.table[sum][line+1] = OrExpr::create(data.table[sum][line+1],aBranch);
		// 			}
		// 		}
		// 	}
	}
	// // now that we filled table, we associate each element of the last row with
	// // the corresponding sums variable
	//
	// for(unsigned sum=0; sum <= data.maxSum; ++sum){
	// 	const Array* sumVar = data.sumsVars[sum];
	// 	klee::ref<Expr> readSumVar= ReadExpr::createTempRead(sumVar, Expr::Bool);
	// 	klee::ref<Expr> sumExpr = data.table[sum].back();
	// 	klee::ref<Expr> sumVarEqExpr = EqExpr::create(readSumVar, sumExpr);
	// 	klee::ref<Expr> sumVarIsTrue = EqExpr::create(readSumVar, ConstantExpr::create(true,Expr::Bool));
	// 	data.sumsConstrs.push_back(AndExpr::create(sumVarEqExpr,sumVarIsTrue));
	// }
	//
	// // for(unsigned i=0; i < data.table.size(); ++i){
	// // 	for(unsigned j=0; j < data.table[i].size(); ++j){
	// // 		std::cout << "table[" << i << "][" << j << "]: " << std::endl;
	// // 		std::cout << data.table[i][j] << std::endl;
	// // }
	//
}



std::string prPad(unsigned tabs){
	std::string pad = "----";
	std::string result = "";
	for (unsigned i = 0 ; i < tabs; i++){
		result += pad;
	}
	return result;
}

void BankConflictsSideChannel::printLineAccesses(std::string path, const ShLineAccType &accesses){
	std::ofstream logFile;
	logFile.open(path.c_str());

	unsigned p = 2;

	logFile << "Executed kernels : " << accesses.size() <<
			std::endl;
	for (const auto &kernel : accesses){
		logFile << "Kernel : " << kernel.first <<
				" executed " << kernel.second.size() << " barrier interval(s)" <<
				std::endl;

		for (const auto &bi : kernel.second){
			logFile << prPad(p) <<
					"At barrier interval : " << bi.first <<
					" executed  " << bi.second.size() << " shared memories" <<
					std::endl;

			for(const auto &shMem : bi.second){
				logFile << prPad(p + 1) <<
						"Shared Memory : " << shMem.first <<
						" accessed by " << shMem.second.size() << " lockstep accesses" <<
						std::endl;

				for(const auto &mav : shMem.second){
					logFile << prPad(p + 2) <<
							"Line : " << &mav <<
							" executed by " << mav.size() << " threads" <<
							std::endl;

					for(const auto &ma : mav){
						logFile << prPad(p + 3) <<
								"Thread : " << ma.tid <<
								" block : " << ma.bid << " | " <<
								" seq num : " << ma.instSeqNum << " | " <<
								" access dump : " <<
								std::endl;
						ma.dump(logFile);
					}
				}
			}
		}
	}

	logFile.close();
}

void printExpressionList(std::ofstream &dest, const std::vector<klee::ref<Expr>> &list, std::string message){
	dest << prPad(10) << message << " " << list.size() << prPad(10) << std::endl;

	for (const auto &c : list){
		c->print(dest);
		dest << std::endl;
	}
}


int BankConflictsSideChannel::getLineValues(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound) {
	std::cout << "starting get line values" << std::endl;

	unsigned lineID = 0;
	for (const auto &entry : data.uniqueLines){
		lineID = entry.first;
		std::cout << "pushing unique line: " << lineID << std::endl;

		LineExplorer* lineExplorer = new LineExplorer(state, solver, lineID, data.lines[lineID]);
		CompoundExplorer* cmpExplorer = new CompoundExplorer(state, lineExplorer, nullptr,solver);

		unsigned maxBound = data.lines[lineID].getMaxPossibleTransactions(BOX_WIDTH, BANK_SIZE);
		for (unsigned value = 1; value <= maxBound ; value++){
			std::cout << "trying value " << value << std::endl;
			CompoundExplorer::TargetedCompoundCliques tcc(cmpExplorer, value);

			if(!tcc.evaluate()){
				std::cout << "not with default cliques," << " some other cliques with same value? " <<  (tcc.next()? "yes!":"no!") << std::endl;
			}

			if(!tcc.isEmpty()){
				std::cout << "a clique that gives values=" << value << " is " << std::endl;
				std::vector<Clique> cliques = tcc.getCliques();
				for(unsigned i=0; i < cliques.size(); ++i){
					std::cout << "clique i: " << cliques[i];
					std::cout << std::endl;
				}

				data.possibleValues[lineID].push_back(value);
			}
		}
		//TODO should delete these two ... but it crashes
		delete cmpExplorer;
		delete lineExplorer;
	}


	std::cout << "Printing possible values per line : " << std::endl;
	for (const auto& uniqLines : data.possibleValues ){
		std::cout << "Line " << uniqLines.first << std::endl;

		for (const auto& value : uniqLines.second){
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}






	//TODO: debug the crash when deleting pointer in cmpExplorers and explorers
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	//   delete cmpExplorers[line - 1];
	// }
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	// 	delete explorers[line - 1];
	// }

	return 0;
}

void analyzePairs(ExecutionState& state, TimingSolver* solver, ModelData &data, const long bound, const std::vector<int> &lineIDVector,
		const unsigned BOX_WIDTH, const unsigned BANK_SIZE ){

	//	//eliminate a symkey value we know already works
	//	std::cout << "Add constraint disjunction for symkey we know already gives the observation:" << std::endl;
	//	//TODO read from file the array
	//	std::ifstream file;
	//	std::vector<int> values;
	//	file.open("./constraints.txt");
	//	if (file.is_open()){
	//		int size;
	//		file >> size;
	//		std::cout << "Vector size for file constraints : " << size << std::endl;
	//
	//		for (int i = 0; i < size; i++){
	//			int value;
	//			file >> value;
	//			values.push_back(value);
	//		}
	//		std::ios_base::fmtflags f( std::cout.flags() );
	//		for (auto val : values){
	//			std::cout << std::hex << val << " ";
	//		}
	//		std::cout << std::endl;
	//		std::cout.flags( f );
	//		file.close();
	//
	//		//add to the state constraints
	//
	//		ref<Expr> formula = NULL;
	//
	//		for (unsigned SI = 0; SI < state.symbolics.size(); SI++) {
	//			std::cout << "arrayname at position " <<  SI << " : " << state.symbolics[SI].second->name << std::endl;
	//			if (state.symbolics[SI].second->name == "symkey"){
	//				for (int i = 0 ; i < size; i++){
	//					UpdateList ul(state.symbolics[SI].second, 0);
	//					ref<Expr> read = ReadExpr::create(ul, ConstantExpr::create(i, Expr::Int32));
	//					ref<Expr> valueEx = ConstantExpr::alloc(values[i], read->getWidth());
	//					if (formula.isNull())
	//						formula = NeExpr::create(read, valueEx);
	//					else
	//						formula = OrExpr::create(formula, NeExpr::create(read, valueEx));
	//				}
	//			}
	//		}
	//		assert(formula.isNull()==0 && "constraining other values for the key went wrong");
	//		std::cout << "Adding formula to constraint list" << std::endl;
	//		formula->print(std::cout);
	//		state.constraints.addConstraint(formula);
	//		std::cout << "\nAdded formula to constraint list" << std::endl;
	//
	//	}else{
	//		std::cout << "No \"constraints.txt\" file found --> skipping this step" << std::endl;
	//	}
	//	//end of eliminating

	std::map<int, LineExplorer*> explorers;
	std::vector<CompoundExplorer*> cmpExplorers;
	unsigned index = 0;
	unsigned maxPairsBound = 0;

	for (const auto &lineID : lineIDVector){
		std::cout << "pushing unique line: " << lineID << std::endl;
		//		std::cout << ":>> size " << data.lines[lineID].threadMemAccVec.size() << std::endl;
		//		for(unsigned i=0; i<data.lines[lineID].threadMemAccVec.size(); i++)
		//			std::cout << ":>> "<< i << ": " << data.lines[lineID].threadMemAccVec.at(i).offset << std::endl;
		explorers[lineID] = new LineExplorer(state, solver, lineID, data.lines[lineID]);
		if(index == 0){
			maxPairsBound = 0;
			std::cout << "Creating head compound explorer for line : " << lineID << std::endl;
			cmpExplorers.push_back(new CompoundExplorer(state, explorers[lineID], nullptr,solver));
		}else{
			std::cout << "Creating compound explorer for line : " << lineID << std::endl;
			cmpExplorers.push_back(new CompoundExplorer(state, explorers[lineID], cmpExplorers[cmpExplorers.size() - 1],solver));
		}
		maxPairsBound += data.lines[lineID].getMaxPossibleTransactions(BOX_WIDTH, BANK_SIZE);
		index++;
	}

	std::cout << ">>>>>>>>>>>Analyzing new pair of size : " << lineIDVector.size() << " with first lineID of pair : " << lineIDVector[0] << std::endl;
	std::cout << "Explorers : " << explorers.size() << " with max value : " << maxPairsBound << std::endl;
	//	for (unsigned value = explorers.size(); value <= maxPairsBound; value++)
	//	for (unsigned value = maxPairsBound; value >= explorers.size(); value--)
	{
		unsigned value = maxPairsBound;//maxPairsBound;//36;//maxPairsBound - 3;//36;
		CompoundExplorer::TargetedCompoundCliques tcc(cmpExplorers[index - 1], value);
		std::cout << "//////////Trying value " << value << std::endl;

		bool foundPossibleSolution = true;
		while((foundPossibleSolution) && (!tcc.evaluate())){ //changed to while to make sure lazyNext() is checked to be correct
			//std::cout << "tcc.next? " <<  tcc.next() << "-><-"<< std::endl;
			foundPossibleSolution = tcc.lazyNext();
			std::cout << "tcc.next? " <<  foundPossibleSolution << "-><-"<< std::endl;
		}

		if(!tcc.isEmpty()){
			std::vector<Clique> cliques = tcc.getCliques();
			std::cout << "Printing found feasible solution\n";
			for(unsigned i=0; i < cliques.size(); ++i){
				std::cout << "clique i= " << i << ": " << cliques[i] << std::endl;
				//std::copy(cliques[i].begin(), cliques[i].end(), std::ostream_iterator<unsigned>(std::cout, " "));
				//			std::cout << std::endl;
			}

			data.possibleValues[lineIDVector[0]].push_back(value);
		}
	}

	//FIXME: debug the crash when deleting pointer in cmpExplorers and explorers
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	//   delete cmpExplorers[line - 1];
	// }
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	// 	delete explorers[line - 1];
	// }

}

int BankConflictsSideChannel::getNPairLineValues(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound, unsigned pairs) {
	std::cout << "starting getNPairLineValues" << std::endl;

	std::map<int, LineExplorer*> explorers;
	std::vector<CompoundExplorer*> cmpExplorers;
	unsigned index = 0;
	unsigned lineID = 0;
	unsigned entries = 0;
	bool newBatch = true;
	unsigned pos;

	//	ExecutionState state(istate);

	std::map<int, std::vector<int>> lineIDMap;
	//std::vector<int> lineIDVector;
	for (const auto &entry : data.uniqueLines){
		index++;
		entries++;
		lineID = entry.first;
		//lineIDVector.push_back(lineID);
		if (newBatch){
			pos = lineID;
			newBatch = false;
		}
		lineIDMap[pos].push_back(lineID);

		if ((index == pairs) || (entries == data.uniqueLines.size())){
			//analyzePairs(state, solver, data, bound, lineIDVector, BOX_WIDTH, BANK_SIZE);
			analyzePairs(state, solver, data, bound, lineIDMap[pos], BOX_WIDTH, BANK_SIZE);
			index = 0;
			//lineIDVector.clear();
			newBatch = true;
		}
	}

	int hasPossibleValues = 0; //no values are possible
	std::cout << "Printing possible values per line : " << std::endl;
	if (data.possibleValues.size() == 0){
		std::cout << "No possible solution for any batch\n";
		return hasPossibleValues;
	}
	hasPossibleValues = 1; //all values are possible
	for (const auto& uniqLines : data.possibleValues ){
		//std::cout << "Starting line " << uniqLines.first << std::endl;
		std::cout << "(";
		for (const auto& lineID : lineIDMap[uniqLines.first]){
			std::cout << "Line " << lineID << ",";
		}
		std::cout << ")\n";
		if (uniqLines.second.size() == 0){
			hasPossibleValues = 2; //some values are possible
		}
		for (const auto& value : uniqLines.second){
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}
	data.possibleValues.clear(); //FIXME: add a more reliable way to save for all possible bytes, now we just clear it so we can try different bytes for same lines

	return hasPossibleValues;
}

/* constrain one byte with a specific value */
ref<Expr> constrainOneByte(ExecutionState& state, int nbyte, int value) {
	ref<Expr> ret = NULL;

	for (unsigned SI = 0; SI < state.symbolics.size(); SI++) {
		std::cout << "arrayname at position " <<  SI << " : " << state.symbolics[SI].second->name << std::endl;
		if (state.symbolics[SI].second->name == "symkey"){
			UpdateList ul(state.symbolics[SI].second, 0);
			ref<Expr> read = ReadExpr::create(ul, ConstantExpr::create(nbyte, Expr::Int32));
			ref<Expr> valueEx = ConstantExpr::alloc(value, read->getWidth());
			if (ret.isNull())
				ret = EqExpr::create(read, valueEx);
			else
				ret = AndExpr::create(ret, EqExpr::create(read, valueEx));
		}
	}

	assert(ret.isNull()==0 && "constraining byte of a key went wrong");

	return ret;
}

int BankConflictsSideChannel::solveUniqueLineDyConstraints(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound) {
	//getConflicts(state,solver,data,bound);
	//getLineValues(state,solver,data,bound);
	unsigned pairs = data.uniqueLines.size();//12;

	std::map<unsigned int, std::map<unsigned int, unsigned int> > isPossibleMap;

	auto startTotal = TimerHelper::startTimer();
	//	for (unsigned position = 0 ; position < 10 ; position++ ){
	//		for (unsigned value = 0 ; value < 256; value ++){
	//			std::cout << prPad(20) << "Setting byte " << position << " to value " << value << std::endl;
	//			ExecutionState istate(state);
	//			auto constraint = constrainOneByte(istate, position, value);
	//			istate.addConstraint(constraint);
	std::cout << prPad(10) << "Starting time " << TimerHelper::getCurrentTime() << std::endl;
	auto start = TimerHelper::startTimer();

	auto hasPossibleValues = getNPairLineValues(state, solver, data, bound, pairs);

	//			if (hasPossibleValues == 0){
	//				std::cout << "No possible solution for byte " << position << " for value " << value << std::endl;
	//			}

	//			isPossibleMap[position][value] = hasPossibleValues;

	auto end = TimerHelper::stopTimer();
	auto stopDifference = TimerHelper::getSeconds(start, end);
	std::cout << prPad(10) << "Computed in " << stopDifference << " seconds" << std::endl;
	std::cout << prPad(10) << "End time " << TimerHelper::getCurrentTime() << std::endl;
	//		}
	//	}

	//	for (const auto &positions : isPossibleMap){
	//		const auto position = positions.first;
	//		std::cout << prPad(20) << "Byte " << position << " set to : " << std::endl;;
	//		for (const auto &values : positions.second){
	//			const auto value = values.first;
	//			const auto isPossible = values.second;
	//			std::cout << value << " " << (isPossible==0?"(NOT possible)":(isPossible==1?"(something possible)":"(possible)")) << " ";
	//		}
	//		std::cout << "\n\n";
	//	}

	auto stopTotal = TimerHelper::startTimer();
	auto stopDifferenceTotal = TimerHelper::getSeconds(startTotal, stopTotal);
	std::cout << prPad(10) << "Total time needed for solving : " << stopDifferenceTotal << " seconds" << std::endl;


	unsigned nbsolutions=0;

	return nbsolutions;
}

void dumpPairs(ExecutionState& state, TimingSolver* solver, ModelData &data, const long bound, const std::vector<int> &lineIDVector,
		const unsigned BOX_WIDTH, const unsigned BANK_SIZE, int byteIndex, int byteValue, int observationValue){

	std::map<int, LineExplorer*> explorers;
	std::vector<CompoundExplorer*> cmpExplorers;
	unsigned index = 0;
	unsigned maxPairsBound = 0;

	for (const auto &lineID : lineIDVector){
		std::cout << "pushing unique line: " << lineID << std::endl;
		//std::cout << ":>> " << data.lines[lineID].threadMemAccVec.at(0).offset << std::endl;
		explorers[lineID] = new LineExplorer(state, solver, lineID, data.lines[lineID]);
		if(index == 0){
			maxPairsBound = 0;
			std::cout << "Creating head compound explorer for line : " << lineID << std::endl;
			cmpExplorers.push_back(new CompoundExplorer(state, explorers[lineID], nullptr,solver));
		}else{
			std::cout << "Creating compound explorer for line : " << lineID << std::endl;
			cmpExplorers.push_back(new CompoundExplorer(state, explorers[lineID], cmpExplorers[cmpExplorers.size() - 1],solver));
		}
		maxPairsBound += data.lines[lineID].getMaxPossibleTransactions(BOX_WIDTH, BANK_SIZE);
		index++;
	}

	std::cout << ">>>>>>>>>>>Dump pair of size : " << lineIDVector.size() << " with first lineID of pair : " << lineIDVector[0] << std::endl;
	std::cout << "Explorers : " << explorers.size() << " with max value : " << maxPairsBound << std::endl;
	//	for (unsigned value = explorers.size(); value <= maxPairsBound; value++)
	//for (unsigned value = maxPairsBound; value >= explorers.size(); value--)
	{
		unsigned value = maxPairsBound;//observationValue; //maxPairsBound;//36;
		CompoundExplorer::TargetedCompoundCliques tcc(cmpExplorers[index - 1], value);
		std::cout << "//////////Trying value " << value << std::endl;

		bool foundPossibleSolution = true;
		while((foundPossibleSolution) && (!tcc.tryDump(byteIndex, byteValue, observationValue))){ //changed to while to make sure lazyNext() is checked to be correct
			//std::cout << "tcc.next? " <<  tcc.next() << "-><-"<< std::endl;
			foundPossibleSolution = tcc.lazyNext();
			std::cout << "tcc.next? " <<  foundPossibleSolution << "-><-"<< std::endl;
		}
	}

	//FIXME: debug the crash when deleting pointer in cmpExplorers and explorers
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	//   delete cmpExplorers[line - 1];
	// }
	// for(unsigned line = data.lines.size(); line > 0 ; --line ){
	// 	delete explorers[line - 1];
	// }

}


int BankConflictsSideChannel::generateConstraintFiles(ExecutionState& state, TimingSolver* solver, ModelData &data, long bound) {

	std::vector<int> lineIDVector;
	unsigned lineID = 0;
	for (const auto &entry : data.uniqueLines){
		lineID = entry.first;
		lineIDVector.push_back(lineID);
	}

	int observation = 36;
	auto startTotal = TimerHelper::startTimer();
	for (unsigned position = 13 ; position < 16 ; position++ ){
		for (unsigned value = 0 ; value < 256; value++){
			std::cout << prPad(20) << "Setting byte " << position << " to value " << value << std::endl;
			ExecutionState istate(state);
			auto constraint = constrainOneByte(istate, position, value);
			istate.addConstraint(constraint);
			std::cout << prPad(10) << "Starting time " << TimerHelper::getCurrentTime() << std::endl;
			auto start = TimerHelper::startTimer();

			dumpPairs(istate, solver, data, bound, lineIDVector, BOX_WIDTH, BANK_SIZE, position, value, observation);

			auto end = TimerHelper::stopTimer();
			auto stopDifference = TimerHelper::getSeconds(start, end);
			std::cout << prPad(10) << "Computed in " << stopDifference << " seconds" << std::endl;
			std::cout << prPad(10) << "End time " << TimerHelper::getCurrentTime() << std::endl;
		}
	}

	auto stopTotal = TimerHelper::startTimer();
	auto stopDifferenceTotal = TimerHelper::getSeconds(startTotal, stopTotal);
	std::cout << prPad(10) << "Total time needed for solving : " << stopDifferenceTotal << " seconds" << std::endl;


	unsigned nbsolutions=0;

	return nbsolutions;
}
