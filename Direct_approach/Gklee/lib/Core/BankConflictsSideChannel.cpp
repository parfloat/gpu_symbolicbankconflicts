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


#include "BankConflictsSideChannel.h"
#include "TimerHelper.h"

unsigned BankConflictsSideChannel::BANK_SIZE = 32;
unsigned BankConflictsSideChannel::BOX_WIDTH = 4;

ModelData BankConflictsSideChannel::readLines;
ModelData BankConflictsSideChannel::writeLines;

using namespace klee;

klee::ref<Expr> BankConflictsSideChannel::isAddrInBoxI(klee::ref<Expr> &addr, unsigned boxID, unsigned boxWidth){
	auto boxWidthE = ConstantExpr::create(boxWidth, addr.get()->getWidth());
	auto divE = UDivExpr::create(addr, boxWidthE);
	auto boxE = ConstantExpr::create(boxID, divE.get()->getWidth());
	auto eqE = EqExpr::create(divE, boxE);

	return eqE;
}

klee::ref<Expr> BankConflictsSideChannel::getPNP(klee::ref<Expr> &precond, const Array* symBox){
	auto readE = ReadExpr::createTempRead(symBox, Expr::Int8);
	auto posResE = EqExpr::create(readE, ConstantExpr::create(1, Expr::Int8));
	auto negResE = EqExpr::create(readE, ConstantExpr::create(0, Expr::Int8));
	auto notPrecond = NotExpr::create(precond);

	auto pE = Expr::createImplies(precond, posResE);
	auto npE = Expr::createImplies(notPrecond, negResE);
	auto pnpE = AndExpr::create(pE, npE);
	return pnpE;
}

void BankConflictsSideChannel::computeOccupancy(Line &line){
	int box = 0;
	for (auto symOcc : line.occupancy){
		klee::ref<Expr> boxOcc;
		for (auto &ma : line.threadMemAccVec){
			klee::ref<Expr> inBoxE = isAddrInBoxI(ma.offset, box, BOX_WIDTH);
			if (boxOcc.isNull()){
				boxOcc = inBoxE;
			}else{
				boxOcc = OrExpr::create(inBoxE, boxOcc);
			}
		}
		//save box occupancy, it is needed for the sum calculation
		line.occPreExpr.push_back(boxOcc);

		//compute condition, precondition
		auto pnpE = getPNP(boxOcc, symOcc);
		line.occConstrs.push_back(pnpE);

		box++;
	}
}

klee::ref<Expr> BankConflictsSideChannel::getSumForBank(const Line &line, unsigned bankID, unsigned bankSize){

	unsigned index = bankID;
	klee::ref<Expr> ret = NULL;
	while (index < line.occupancy.size()){
		const Array* occ = line.occupancy[index];
		auto readE = ReadExpr::createTempRead(occ, Expr::Int8);
		if (ret.isNull()){
			ret = readE;
		}else{
			ret = AddExpr::create(readE, ret);
		}
		index += bankSize;
	}

	return ret;
}

void BankConflictsSideChannel::computeSums(Line &line){
	unsigned banks = line.sum.size();
	for (unsigned bank = 0 ; bank < banks; bank++){
		auto sumBankE = getSumForBank(line, bank, banks);
		line.sumExpr.push_back(sumBankE);

		const Array* sum = line.sum[bank];
		auto readSumE = ReadExpr::createTempRead(sum, Expr::Int8);
		auto sumEqE = EqExpr::create(readSumE, sumBankE);
		line.sumConstrs.push_back(sumEqE);
	}
}

klee::ref<Expr> BankConflictsSideChannel::getLargerSumExpr(Line &line, unsigned sumIndex){

	const Array* sum = line.sum[sumIndex];
	auto sumE = ReadExpr::createTempRead(sum, Expr::Int8);

	klee::ref<Expr> ret = NULL;

	unsigned banks = line.sum.size();
	for (unsigned i = 0 ; i < banks; i++){
		if (i != sumIndex){
			const Array* isum = line.sum[i];
			auto isumE = ReadExpr::createTempRead(isum, Expr::Int8);
			auto ugeE = UgeExpr::create(sumE, isumE);

			if (ret.isNull()){
				ret = ugeE;
			}else{
				ret = AndExpr::create(ugeE, ret);
			}
		}
	}

	return ret;
}

void BankConflictsSideChannel::computeMaximum(Line &line){
	//building the select statement
	klee::ref<Expr> selectE = NULL;
	for (unsigned sumIndex = 0 ; sumIndex < line.sum.size(); sumIndex++){
		const Array* sum = line.sum[sumIndex];
		auto readE = ReadExpr::createTempRead(sum, Expr::Int8);

		if (selectE.isNull()){
			selectE = readE;
		}else{
			auto compareE = getLargerSumExpr(line, sumIndex);
			selectE = SelectExpr::create(compareE, readE, selectE);
		}
	}
	line.maxSelectExpr.push_back(selectE);

	auto readMaxE = ReadExpr::createTempRead(line.max, Expr::Int8);
	auto eqE = EqExpr::create(readMaxE, selectE);
	line.maxConstrs.push_back(eqE);
}

void BankConflictsSideChannel::boundMax(Line &line){
	auto loBoundE = ConstantExpr::create(1, Expr::Int8);
	//	unsigned activeThreads = line.threadMemAccVec.size();
	//	unsigned possibleBankLines = ((line.shMemBytes / BOX_WIDTH)) / BANK_SIZE;
	//	if (possibleBankLines == 0)
	//		possibleBankLines = 1;
	//	unsigned maxBound = (activeThreads < possibleBankLines)?(activeThreads):(possibleBankLines);
	unsigned maxBound = line.getMaxPossibleTransactions(BOX_WIDTH, BANK_SIZE);
	auto hiBoundE = ConstantExpr::create(maxBound, Expr::Int8);

	auto readMaxE = ReadExpr::createTempRead(line.max, Expr::Int8);

	auto loE = UleExpr::create(loBoundE, readMaxE);
	line.maxConstrs.push_back(loE);

	auto hiE = UleExpr::create(readMaxE, hiBoundE);
	line.maxConstrs.push_back(hiE);
}

void BankConflictsSideChannel::computeTotalSum(Line &line){
	klee::ref<Expr> tSumE = NULL;
	for (auto &sum : line.sum){
		auto readE = ReadExpr::createTempRead(sum, Expr::Int8);
		auto zextE = ZExtExpr::create(readE, Expr::Int32);
		if (tSumE.isNull()){
			tSumE = zextE;
		}else{
			tSumE = AddExpr::create(zextE, tSumE);
		}
	}
	line.tSumExpr.push_back(tSumE);

	auto tSum = line.tSum;
	auto readTSumE = ReadExpr::createTempRead(tSum, Expr::Int32);
	auto eqE = EqExpr::create(readTSumE, tSumE);

	line.tSumConstrs.push_back(eqE);

}

void BankConflictsSideChannel::computeTotalMax(ModelData &data){
	klee::ref<Expr> tMaxE = NULL;
	for (auto &line : data.lines){
		auto readE = ReadExpr::createTempRead(line.max, Expr::Int8);
		auto zextE = ZExtExpr::create(readE, Expr::Int32);
		if (tMaxE.isNull()){
			tMaxE = zextE;
		}else{
			tMaxE = AddExpr::create(zextE, tMaxE);
		}
	}
	data.tMaxExpr.push_back(tMaxE);

	auto tMax = data.tMax;
	auto readTMaxE = ReadExpr::createTempRead(tMax, Expr::Int32);
	auto eqE = EqExpr::create(readTMaxE, tMaxE);

	data.tMaxConstrs.push_back(eqE);
}


void BankConflictsSideChannel::boundTotalSum(Line &line){
	auto loBoundE = ConstantExpr::create(1, Expr::Int32);

	unsigned activeThreads = line.threadMemAccVec.size();
	//unsigned possibleBankLines = ((line.shMemBytes / BOX_WIDTH)) / BANK_SIZE;
	//if (possibleBankLines == 0)
	//	possibleBankLines = 1;
	unsigned maxBound = activeThreads;//(activeThreads < possibleBankLines)?(activeThreads):(possibleBankLines);
	auto hiBoundE = ConstantExpr::create(maxBound, Expr::Int32);

	auto readTSumE = ReadExpr::createTempRead(line.tSum, Expr::Int32);

	auto loE = UleExpr::create(loBoundE, readTSumE);
	line.tSumConstrs.push_back(loE);

	auto hiE = UleExpr::create(readTSumE, hiBoundE);
	//	auto hiE = UgeExpr::create(hiBoundE, readTSumE);
	line.tSumConstrs.push_back(hiE);
}
void BankConflictsSideChannel::boundTotalMax(ModelData &data){
	unsigned tLines = data.lines.size();
	unsigned tMaxBound = 0;
	for (auto &line : data.lines){
		//		unsigned activeThreads = line.threadMemAccVec.size();
		//		unsigned possibleBankLines = ((line.shMemBytes / BOX_WIDTH)) / BANK_SIZE;
		//		if (possibleBankLines == 0)
		//			possibleBankLines = 1;
		//		unsigned maxBound = (activeThreads < possibleBankLines)?(activeThreads):(possibleBankLines);
		tMaxBound += line.getMaxPossibleTransactions(BOX_WIDTH, BANK_SIZE);
	}

	auto loBoundE = ConstantExpr::create(tLines, Expr::Int32);
	auto hiBoundE = ConstantExpr::create(tMaxBound, Expr::Int32);

	auto readTMaxE = ReadExpr::createTempRead(data.tMax, Expr::Int32);

	auto loE = UleExpr::create(loBoundE, readTMaxE);
	data.tMaxConstrs.push_back(loE);

	auto hiE = UleExpr::create(readTMaxE, hiBoundE);
	data.tMaxConstrs.push_back(hiE);
}

void BankConflictsSideChannel::addExtraConstrs(ModelData &data){
	//TODO add extra constraints if wanted
}

void BankConflictsSideChannel::buildConstraints(ModelData &data){

	for (auto &line : data.lines){
		computeOccupancy(line);
		computeSums(line);
		computeMaximum(line);
		boundMax(line);
		computeTotalSum(line);
		boundTotalSum(line);
	}
	computeTotalMax(data);
	boundTotalMax(data);

	addExtraConstrs(data);//empty for now
}

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
	equivalenceClasses = initializeVect;

	//initially the lines belong to no equivalence class
	//	for (int i = 0; i < equivalenceClasses.size(); i++){
	//		equivalenceClasses[i] = -1;
	//	}

	for (int i = 0 ; i < equivalenceClasses.size(); i++){
		if (equivalenceClasses[i] == -1){ //it was not equal to any other line before, then it belongs in its own class
			std::cout << "New equivalence class at position " << i << std::endl;
			equivalenceClasses[i] = i;
			for (int j = i + 1; j < equivalenceClasses.size(); j++){
				if (equivalenceClasses[j] == -1){
					klee::ref<Expr> compare = lineEquivalenceCompare(lines[i], lines[j]);
					//					std::cout << "compare expr : ";
					//					compare->print(std::cout);
					//					std::cout << std::endl;
					auto notEqual = NotExpr::create(compare);
					//					std::cout << "notEqual expr : ";
					//					notEqual->print(std::cout);
					//					std::cout << std::endl;

					bool res;
					bool success = solver->mustBeFalse(state, notEqual, res);
					std::cout << "Success = " << success << " | res = " << res << std::endl;

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


	std::cout << "Printing equivalence vector : "
			<< lines.size() << " total lines with "
			<< std::endl;
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

	std::cout << uniqueLines.size() << " unique lines " << std::endl;
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
	//	std::string readPath = "READ_LINE_SHARED_" + std::to_string(pathNum) + ".log";
	//	printLineAccesses(readPath, state.addressSpace.lineReadSharedLines);
	//	std::string writePath = "WRITE_LINE_SHARED_" + std::to_string(pathNum) + ".log";
	//	printLineAccesses(writePath, state.addressSpace.lineWriteSharedLines);

	//build the lines, from the read memory accesses
	if (state.addressSpace.lineReadSharedLines.size() > 0){
		buildInternalStructure(readLines, state.addressSpace.lineReadSharedLines, "rd");
		buildConstraints(readLines);
		std::string readCstrsPath = "READ_CONSTRAINTS_" + std::to_string(pathNum) + ".log";
		//printConstraints(readCstrsPath, readLines);
		//solveAllConstraints(state, solver, readLines);
		readLines.computeUniqueLines(state, solver); //equivalence classes - constant difference approach
		//checkEquality(state, solver, readLines); //equivalence classes - full model difference approach

		auto startTotal = TimerHelper::startTimer();
		std::cout << "---------" << "Starting time " << TimerHelper::getCurrentTime() << std::endl;
		auto start = TimerHelper::startTimer();

		//		analyzePerUniqueLine(readLines, state, solver);
		analyzeUniqueLinesTarget(readLines, state, solver, 32);

		auto end = TimerHelper::stopTimer();
		auto stopDifference = TimerHelper::getSeconds(start, end);
		std::cout << "---------" << "Computed in " << stopDifference << " seconds" << std::endl;
		std::cout << "---------" << "End time " << TimerHelper::getCurrentTime() << std::endl;
	}
	//
	//
	//	//build the lines, from the write memory accesses
	//	if (state.addressSpace.lineWriteSharedLines.size() > 0){
	//		buildInternalStructure(writeLines, state.addressSpace.lineWriteSharedLines, "wr");
	//		buildConstraints(writeLines);
	//		std::string writeCstrsPath = "WRITE_CONSTRAINTS_" + std::to_string(pathNum) + ".log";
	//		printConstraints(writeCstrsPath, writeLines);
	//		//solveAllConstraints(state, solver, writeLines);
	//	}

}


void BankConflictsSideChannel::resetModel(){
	readLines.clear();
	writeLines.clear();
}

void BankConflictsSideChannel::buildInternalStructure(ModelData &data, const ShLineAccType &accesses, std::string uniqueID){

	unsigned lineID = 0;
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

					//create the appropriate number of boxes
					unsigned boxes = shMemSizeBytes / BOX_WIDTH;
					line.boxes = boxes;
					for (unsigned box = 0 ; box < boxes; box++){
						std::string occID = "occupancy_line" + std::to_string(lineID) + "_box" + std::to_string(box) + "_" + uniqueID;
						const Array* occ = new Array(occID, 1);
						line.occupancy.push_back(occ);
					}

					//create the appropriate number of sums -> for each bank
					unsigned bankSize = (line.boxes < BANK_SIZE)?(line.boxes):(BANK_SIZE);
					for (unsigned bank = 0 ; bank < bankSize; bank++){
						std::string sumID = "sum_line" + std::to_string(lineID) + "_bank" + std::to_string(bank) + "_" + uniqueID;
						const Array* sum = new Array(sumID, 1);
						line.sum.push_back(sum);
					}

					//create the max for this line
					std::string maxID = "max_line" + std::to_string(lineID) + "_" + uniqueID;
					const Array* max = new Array(maxID, 1);
					line.max = max;

					//create tSum
					std::string tSumID = "tSum_line" + std::to_string(lineID) + "_" + uniqueID;
					const Array* tSum = new Array(tSumID, 4); //4 bytes to have enough range
					line.tSum = tSum;

					//add the new created line to the list
					data.lines.push_back(line);
					lineID++;
				}
			}
		}
	}

	//	//create tSum
	//	std::string tSumID = "tSum_" + uniqueID;
	//	const Array* tSum = new Array(tSumID, 4); //4 bytes to have enough range
	//	data.tSum = tSum;

	//create tMax
	std::string tMaxID = "tMax_" + uniqueID;
	const Array* tMax = new Array(tMaxID, 4); //4 bytes to have enough range
	data.tMax = tMax;
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

void BankConflictsSideChannel::printConstraints(std::string path, ModelData &data){
	std::ofstream logFile;
	logFile.open(path.c_str());

	unsigned lineID = 0;
	for (auto &line : data.lines){
		logFile << prPad(20) << "Line " << lineID << prPad(20) << std::endl;

		printExpressionList(logFile, line.occPreExpr, "Occupancy calculation : ");
		printExpressionList(logFile, line.occConstrs, "Occupancy constraints : ");

		printExpressionList(logFile, line.sumExpr, "Sum calculation : ");
		printExpressionList(logFile, line.sumConstrs, "Sum constraints : ");

		printExpressionList(logFile, line.maxSelectExpr, "Max calculation : ");
		printExpressionList(logFile, line.maxConstrs, "Max constraints : ");

		printExpressionList(logFile, line.tSumExpr, "TSum calculation : ");
		printExpressionList(logFile, line.tSumConstrs, "TSum constraints : ");

		lineID++;
	}

	logFile << prPad(20) << std::endl;

	printExpressionList(logFile, data.tMaxExpr, "TMax calculation : ");
	printExpressionList(logFile, data.tMaxConstrs, "TMax constraints : ");



	logFile.close();
}


void addConstraints(ExecutionState &state, std::vector<klee::ref<Expr> > constraints){
	for (auto &c : constraints){
		//		std::cout << prPad(10) <<"Adding constraint: " << prPad(10) << std::endl;
		//		c->print(std::cout);
		//		std::cout << std::endl;
		state.addConstraint(c);
	}
}

void addConstraints(ExecutionState &state, ModelData &data){
	unsigned count = 0 ;
	std::cout << prPad(10) << "Adding constraints: " << prPad(10) << std::endl;
	for (const auto &line : data.lines){
		std::cout << prPad(10) << "Adding constraints for line : " << count++ << prPad(10) << std::endl;
		std::cout << prPad(10) << "Adding occupancy constraints: " << prPad(10) << std::endl;
		addConstraints(state, line.occConstrs);
		std::cout << prPad(10) << "Adding sum constraints: " << prPad(10) << std::endl;
		addConstraints(state, line.sumConstrs);
		std::cout << prPad(10) << "Adding max constraints: " << prPad(10) << std::endl;
		addConstraints(state, line.maxConstrs);
		std::cout << prPad(10) << "Adding total sum constraints: " << prPad(10) << std::endl;
		addConstraints(state, line.tSumConstrs);
	}
	std::cout << prPad(10) << "Adding total max constraints: " << prPad(10) << std::endl;
	addConstraints(state, data.tMaxConstrs);

	std::cout << prPad(10) << "Adding extra constraints: " << prPad(10) << std::endl;
	addConstraints(state, data.extraConstrs);
}

void addSymbolic(std::vector<const Array*> &objects, const Array* sym){
	//	std::cout << "\n\n -> encountered symbolic object \"" << sym->name << "\"" << std::endl;
	objects.push_back(sym);
}

void addSymbolics(std::vector<const Array*> &objects, std::vector<const Array*> &symbolics){
	for (const Array* sym : symbolics){
		addSymbolic(objects, sym);
	}
}

void addSymbolics(std::vector<const Array*> &objects, ModelData &data){
	unsigned count = 0 ;
	std::cout << prPad(10) << "Adding symbolics: " << prPad(10) << std::endl;
	for (auto &line : data.lines){
		std::cout << prPad(10) << "Adding symbolics for line : " << count++ << prPad(10) << std::endl;
		std::cout << prPad(10) << "Adding occupancy symbolics: " << prPad(10) << std::endl;
		addSymbolics(objects, line.occupancy);
		std::cout << prPad(10) << "Adding sum symbolics: " << prPad(10) << std::endl;
		addSymbolics(objects, line.sum);
		std::cout << prPad(10) << "Adding max symbolics: " << prPad(10) << std::endl;
		addSymbolic(objects, line.max);
		std::cout << prPad(10) << "Adding tSum symbolics: " << prPad(10) << std::endl;
		addSymbolic(objects, line.tSum);
	}
	std::cout << prPad(10) << "Adding total max symbolics: " << prPad(10) << std::endl;
	addSymbolic(objects, data.tMax);
}

/* Solve all constraints to get models of secret keys. Each solution is a model for the secret key.
 * The observational constraint is added first (e.g. count-based or seq-based) and the solver is
 * called iteratively */
int BankConflictsSideChannel::solveAllConstraints(ExecutionState& state, TimingSolver* solver,
		ModelData &data, long bound) {

	ExecutionState cState(state);
	/* add the observational constraints */
	addConstraints(cState, data);

	int nsolution = 0;

	/* get the models for cache miss constraints */
	{
		/* get values of symbolic objects */
		std::vector< std::vector<unsigned char> > values;
		std::vector<const Array*> objects;
		for (unsigned SI = 0; SI != state.symbolics.size(); SI++) {
			std::cout << "\n\n -> encountered symbolic object \"" << state.symbolics[SI].first->name << "\"" << std::endl;
			std::cout << "address = " << state.symbolics[SI].first->address << std::endl;
			objects.push_back(state.symbolics[SI].second);
		}

		addSymbolics(objects, data);

		bool success = true;
		/********** the KEY CEGAR loop to get different solutions for a given cache miss **********/
		while (success) {
			/* get a model (solution) for the observed cache misses */
			success = solver->getInitialValues(cState, objects, values);
			solver->setTimeout(0);
			/* no solution exists */
			if (!success) {
				std::cout << "\n\n..... observation constraint have no possible solutions .....\n" << std::endl;
			} else /* we have a solution for observed cache misses */ {
				/* dump cache constraints */
				//TODO dump the contraints for cstate if wanted
				std::cout << "\n\n..... printing one solution for observation constraint .....\n" << std::endl;

				for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
					std::cout << objects[symIndex]->name << " = ";
					std::cout << "(size " << objects[symIndex]->size << ", little-endian) ";
					for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++)
						std::cout << (int)values[symIndex][valueIndex] << " ";
					std::cout << std::endl;
				}
				nsolution++;
				std::cout << "..... #keys discovered so far = " << nsolution << " .....\n" << std::endl;
				/* sudiptac: only get as many as "bound" solutions */
				if (bound != -1){
					if (nsolution >= bound)
						break;
				}
				/* implement features to get a different solution each time
				 * "getInitialValues" is called	*/
				naiveCEGAR(state, cState, data, objects, values);
			}
		}
		/********** END of KEY CEGAR loop **********/
	}

	return nsolution;
}


/* implements a naive CEGAR based approach to get all possible inputs for a given
 * (observed) cache miss */
void BankConflictsSideChannel::naiveCEGAR(ExecutionState& state, ExecutionState& cState,
		ModelData &data,
		std::vector<const Array*> &objects,
		std::vector<std::vector<unsigned char> >& values) {

	//	ref<Expr> counterEx = NULL;
	//
	//	std::cout << "CEGAR process => building the large counter example \n" << std::endl;
	//	for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
	//		UpdateList ul(objects[symIndex], 0);
	//		for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++) {
	//			ref<Expr> read = ReadExpr::create(ul, ConstantExpr::create(valueIndex, Expr::Int32));
	//			ref<Expr> assignedValue = ConstantExpr::alloc(values[symIndex][valueIndex], read->getWidth());
	//			ref<Expr> eqExpr = EqExpr::create(read, assignedValue);
	//			/* add more constraints to the cache state encoding */
	//			(counterEx.isNull()) ? (counterEx = eqExpr) : (counterEx = AndExpr::create(counterEx, eqExpr));
	//		}
	//	}
	//
	//	/* add negation to get a different solution in the next CEGAR iteration */
	//	ref<Expr> notEq = NotExpr::create(counterEx);
	//	cState.addConstraint(notEq);

	//add constraint so that TMax is not found again - the attack model
	std::cout << "CEGAR process => building the tMax counter example \n" << std::endl;
	ref<Expr> tMaxCounterEx = NULL;
	for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
		if (objects[symIndex]->name == data.tMax->name){
			UpdateList ul(objects[symIndex], 0);
			for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++) {
				ref<Expr> read = ReadExpr::create(ul, ConstantExpr::create(valueIndex, Expr::Int32));
				ref<Expr> assignedValue = ConstantExpr::alloc(values[symIndex][valueIndex], read->getWidth());
				ref<Expr> eqExpr = EqExpr::create(read, assignedValue);
				/* add more constraints to the cache state encoding */
				(tMaxCounterEx.isNull()) ? (tMaxCounterEx = eqExpr) : (tMaxCounterEx = AndExpr::create(tMaxCounterEx, eqExpr));
				tMaxCounterEx->print(std::cout);
				std::cout << "\n" << std::endl;
			}
		}
	}
	ref<Expr> tMaxNotEq = NotExpr::create(tMaxCounterEx);
	std::cout << "\n\nTMax counter example : " << std::endl;
	tMaxNotEq->print(std::cout);
	cState.addConstraint(tMaxNotEq);
}


void BankConflictsSideChannel::checkEquality(ExecutionState& state, TimingSolver* solver,
		ModelData &data, long bound){

	std::cout << "In checkEquality " << std::endl;

	for (unsigned i = 0; i < data.lines.size() - 1; i++){
		for (unsigned j = i + 1; j < data.lines.size(); j++){
			ExecutionState cState(state);
			addConstraints(cState, data.lines[i].occConstrs);
			addConstraints(cState, data.lines[i].sumConstrs);
			addConstraints(cState, data.lines[i].maxConstrs);
			addConstraints(cState, data.lines[i].tSumConstrs);

			addConstraints(cState, data.lines[j].occConstrs);
			addConstraints(cState, data.lines[j].sumConstrs);
			addConstraints(cState, data.lines[j].maxConstrs);
			addConstraints(cState, data.lines[j].tSumConstrs);


			std::vector< std::vector<unsigned char> > values;
			std::vector<const Array*> objects;
			for (unsigned SI = 0; SI != state.symbolics.size(); SI++) {
				//std::cout << "\n\n -> encountered symbolic object \"" << state.symbolics[SI].first->name << "\"" << std::endl;
				//std::cout << "address = " << state.symbolics[SI].first->address << std::endl;
				objects.push_back(state.symbolics[SI].second);
			}

			addSymbolics(objects, data.lines[i].occupancy);
			addSymbolics(objects, data.lines[i].sum);
			addSymbolic(objects, data.lines[i].max);
			addSymbolic(objects, data.lines[i].tSum);

			addSymbolics(objects, data.lines[j].occupancy);
			addSymbolics(objects, data.lines[j].sum);
			addSymbolic(objects, data.lines[j].max);
			addSymbolic(objects, data.lines[j].tSum);

			auto readE1 = ReadExpr::createTempRead(data.lines[i].max, Expr::Int8);
			auto readE2 = ReadExpr::createTempRead(data.lines[j].max, Expr::Int8);
			auto neqExpr = NeExpr::create(readE1, readE2);
			cState.addConstraint(neqExpr);

			bool result;
			result = solver->getInitialValues(cState, objects, values);
			solver->setTimeout(0);
			if (result == false){
				std::cout << "Line " << i << " and " << " line " << j << " are identical" << std::endl;
			}else{
				std::cout << "Line " << i << " and " << " line " << j << " are NOT identical" << std::endl;
				std::cout << "\n\n..... printing one solution for observation constraint .....\n" << std::endl;

				for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
					std::cout << objects[symIndex]->name << " = ";
					std::cout << "(size " << objects[symIndex]->size << ", little-endian) ";
					for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++)
						std::cout << (int)values[symIndex][valueIndex] << " ";
					std::cout << std::endl;
				}
			}

		}
	}
}

void BankConflictsSideChannel::analyzePerUniqueLine(ModelData &data,ExecutionState& state,
		TimingSolver* solver){


	std::cout << "\n\nStart analyzing each unique line" << std::endl;
	unsigned lineID = 0;
	for (const auto &entry : data.uniqueLines){
		lineID = entry.first;
		std::cout << "Analyzing unique line: " << lineID << std::endl;
		std::cout << "Shared memory size : " << data.lines[lineID].shMemBytes << " "
				<< " boxes : " << data.lines[lineID].boxes
				<< std::endl;

		auto maximum = data.lines[lineID].getMaxPossibleTransactions(BOX_WIDTH, BANK_SIZE);
		for (unsigned value = maximum; value <= maximum ; value++){
			ExecutionState cState(state); //FIXME move it before, and just update with the current equality
			addConstraints(cState, data.lines[lineID].occConstrs);
			addConstraints(cState, data.lines[lineID].sumConstrs);
			addConstraints(cState, data.lines[lineID].maxConstrs);
			addConstraints(cState, data.lines[lineID].tSumConstrs);


			std::vector< std::vector<unsigned char> > values;
			std::vector<const Array*> objects;
			for (unsigned SI = 0; SI != state.symbolics.size(); SI++) {
				//std::cout << "\n\n -> encountered symbolic object \"" << state.symbolics[SI].first->name << "\"" << std::endl;
				//std::cout << "address = " << state.symbolics[SI].first->address << std::endl;
				objects.push_back(state.symbolics[SI].second);
			}

			addSymbolics(objects, data.lines[lineID].occupancy);
			addSymbolics(objects, data.lines[lineID].sum);
			addSymbolic(objects, data.lines[lineID].max);
			addSymbolic(objects, data.lines[lineID].tSum);
			std::cout << "trying value " << value << std::endl;

			auto target = ConstantExpr::create(value, Expr::Int8);
			auto max =  ReadExpr::createTempRead(data.lines[lineID].max, Expr::Int8);
			auto equal = EqExpr::create(max, target);
			auto simpEqual = cState.constraints.simplifyExpr(equal);
			std::cout << "Equality " << equal << std::endl;

			if ((simpEqual->getKind() == Expr::Constant) && (cast<ConstantExpr>(simpEqual)->isFalse())){
				std::cout << "Line " << lineID << " CANNOT have value " << value << std::endl;
			}else{
				std::cout << "Line " << lineID << " adding equal expression :  " << equal << std::endl;
				cState.addConstraint(equal); //we can only add if we are sure "equal" is not false
				std::cout << "Line " << lineID << " trying with solver for value :  " << value << std::endl;
				bool result;
				result = solver->getInitialValues(cState, objects, values);
				solver->setTimeout(0);
				if (result == true){
					std::cout << "Line " << lineID << " can have value " << value << std::endl;
					//				std::cout << "\n\n..... printing one solution for observation constraint .....\n" << std::endl;
					//
					//				for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
					//					std::cout << objects[symIndex]->name << " = ";
					//					std::cout << "(size " << objects[symIndex]->size << ", little-endian) ";
					//					for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++)
					//						std::cout << (int)values[symIndex][valueIndex] << " ";
					//					std::cout << std::endl;
					//				}

					data.possibleValues[lineID].push_back(value);
				}else{
					std::cout << "Line " << lineID << " CANNOT have value " << value << std::endl;
				}



			}
		}
	}


	std::cout << "Printing possible values per line : " << std::endl;
	for (const auto& uniqLines : data.possibleValues ){
		std::cout << "Line " << uniqLines.first << std::endl;

		for (const auto& value : uniqLines.second){
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}

}

void BankConflictsSideChannel::analyzeUniqueLinesTarget(ModelData &data,ExecutionState& state,
		TimingSolver* solver, const int obsTarget){


	std::cout << "\n\nStart analyzing unique lines with target = " << obsTarget << std::endl;
	ExecutionState cState(state); //everything gets stored here
	std::vector< std::vector<unsigned char> > values;
	std::vector<const Array*> objects;
	for (unsigned SI = 0; SI != state.symbolics.size(); SI++) {
		//std::cout << "\n\n -> encountered symbolic object \"" << state.symbolics[SI].first->name << "\"" << std::endl;
		//std::cout << "address = " << state.symbolics[SI].first->address << std::endl;
		objects.push_back(state.symbolics[SI].second);
	}

	unsigned lineID = 0;
	for (const auto &entry : data.uniqueLines){
		lineID = entry.first;
		std::cout << "Adding unique line: " << lineID << std::endl;
		std::cout << "Shared memory size : " << data.lines[lineID].shMemBytes << " "
				<< " boxes : " << data.lines[lineID].boxes
				<< std::endl;

		addConstraints(cState, data.lines[lineID].occConstrs);
		addConstraints(cState, data.lines[lineID].sumConstrs);
		addConstraints(cState, data.lines[lineID].maxConstrs);
		addConstraints(cState, data.lines[lineID].tSumConstrs);

		addSymbolics(objects, data.lines[lineID].occupancy);
		addSymbolics(objects, data.lines[lineID].sum);
		addSymbolic(objects, data.lines[lineID].max);
		addSymbolic(objects, data.lines[lineID].tSum);
	}

	addConstraints(cState, data.tMaxConstrs);

	addSymbolic(objects, data.tMax);

	auto target = ConstantExpr::create(obsTarget, Expr::Int8);
	auto max =  ReadExpr::createTempRead(data.tMax, Expr::Int8);
	auto equal = EqExpr::create(max, target);
	auto simpEqual = cState.constraints.simplifyExpr(equal);
	std::cout << "Equality " << equal << std::endl;

	if ((simpEqual->getKind() == Expr::Constant) && (cast<ConstantExpr>(simpEqual)->isFalse())){
		std::cout << "Line " << lineID << " CANNOT have value " << obsTarget << std::endl;
	}else{
		std::cout << "Adding equal expression :  " << equal << std::endl;
		cState.addConstraint(equal); //we can only add if we are sure "equal" is not false
		std::cout << "Trying with solver for value :  " << obsTarget << std::endl;
		bool result;
		result = solver->getInitialValues(cState, objects, values);
		solver->setTimeout(0);
		if (result == true){
			std::cout << "We can have value " << obsTarget << std::endl;
			//				std::cout << "\n\n..... printing one solution for observation constraint .....\n" << std::endl;
			//
			//				for (unsigned symIndex = 0; symIndex < objects.size(); symIndex++) {
			//					std::cout << objects[symIndex]->name << " = ";
			//					std::cout << "(size " << objects[symIndex]->size << ", little-endian) ";
			//					for (unsigned valueIndex = 0; valueIndex < values[symIndex].size(); valueIndex++)
			//						std::cout << (int)values[symIndex][valueIndex] << " ";
			//					std::cout << std::endl;
			//				}

			data.possibleValues[lineID].push_back(obsTarget);
		}else{
			std::cout << "Line " << lineID << " CANNOT have value " << obsTarget << std::endl;
		}
	}



	std::cout << "Printing possible values per line : " << std::endl;
	for (const auto& uniqLines : data.possibleValues ){
		std::cout << "Line " << uniqLines.first << std::endl;

		for (const auto& value : uniqLines.second){
			std::cout << value << " ";
		}
		std::cout << std::endl;
	}

}
