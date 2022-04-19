/*
 * TimerHelper.cpp
 *
 *  Created on: Jun 8, 2020
 *      Author: adrianh
 */


#include "TimerHelper.h"
#include <array>

timer_point TimerHelper::startTime = std::chrono::steady_clock::now();;
timer_point TimerHelper::currentTime;

timer_point TimerHelper::startTimer(){
	startTime = std::chrono::steady_clock::now();//std::chrono::system_clock::now();
	return startTime;
}

timer_point TimerHelper::stopTimer(){
	currentTime = std::chrono::steady_clock::now();//std::chrono::system_clock::now();
	return currentTime;
}

double TimerHelper::getSeconds(timer_point start, timer_point stop){
	std::chrono::duration<double> elapsed_seconds = stop - start;
	double seconds = elapsed_seconds.count();
	return (seconds);
}

double TimerHelper::getElapsedSeconds(){
	stopTimer();
	std::chrono::duration<double> elapsed_seconds = currentTime - startTime;
	double seconds = elapsed_seconds.count();
	//	long int seconds = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();
	return (seconds);
}

double TimerHelper::getElapsedMilliseconds(){
	//	std::chrono::duration<double> elapsed_seconds = currentTime - startTime;
	//	double seconds = elapsed_seconds.count();
	double seconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count();
	return (seconds);
}

std::string TimerHelper::getCurrentTime(){
	std::array<char, 64> buffer;
	buffer.fill(0);
	time_t rawtime;
	time(&rawtime);
	const auto timeinfo = localtime(&rawtime);
	strftime(buffer.data(), sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	std::string timeStr(buffer.data());

	return timeStr;
}





