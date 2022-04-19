/*
 * TimerHelper.h
 *
 *  Created on: Jun 8, 2020
 *      Author: adrianh
 */

#ifndef GKLEE_LIB_CORE_TIMERHELPER_H_
#define GKLEE_LIB_CORE_TIMERHELPER_H_

#include <iostream>
#include <chrono>
#include <ctime>

typedef std::chrono::time_point<std::chrono::steady_clock> timer_point;

class TimerHelper{
public:
	static timer_point startTimer();
	static timer_point stopTimer();
	static double getSeconds(timer_point start, timer_point stop);
	static double getElapsedSeconds();
	static double getElapsedMilliseconds();
	static std::string getCurrentTime();

private:
	static timer_point startTime;
	static timer_point currentTime;
};



#endif /* GKLEE_LIB_CORE_TIMERHELPER_H_ */
