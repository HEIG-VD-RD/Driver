/**
* @brief : This file is to be shared by kernel driver and user space app
* @file : myTime.h
* @Author : Rafael Dousse
*/
#ifndef MYTIME_H
#define MYTIME_H

/**
 * @brief : Struct that represents a time
 */
struct myTime {
	uint32_t centiseconds;
	uint32_t seconds;
	uint32_t minutes;
};

/**
 * @brief : Struct that represents the time to be read by the user
 * @param currentTime : Current time of the chrono
 * @param lapTime : One lap time
 * @param isEnd : Flag to indicate the end of list
 */
struct userReadTime {
	struct myTime currentTime;
	struct myTime lapTime;
	bool isEnd;
};

#endif