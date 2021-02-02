/*
 * ChainSim.h
 *
 *  Created on: May 19, 2019
 *      Author: mirco
 */

#ifndef SIMULATION_CHAINSIM_H_
#define SIMULATION_CHAINSIM_H_
#include <cpsCore/Utilities/Time.hpp>
#include <vector>

struct JitterCounter;

struct ChainSim
{

	std::vector<unsigned> chain;

	std::vector<std::pair<TimePoint, std::vector<uint8_t>>> data; //0 not read, 1 read, 2 reacted

	std::vector<Duration> ages;
	std::vector<Duration> reactions;

	void
	read(unsigned task, const TimePoint& time);

	void
	write(unsigned task, const TimePoint& time);

};



#endif /* SIMULATION_CHAINSIM_H_ */
