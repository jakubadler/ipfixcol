/*
 * \file FlowWatch.cpp
 * \author Petr Kramolis <kramolis@cesnet.cz>
 * \brief class for flows and SQ numbers check
 *
 * Copyright (C) 2012 CESNET, z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Company nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this
 * product may be distributed under the terms of the GNU General Public
 * License (GPL) version 2 or later, in which case the provisions
 * of the GPL apply INSTEAD OF those given above.
 *
 * This software is provided ``as is, and any express or implied
 * warranties, including, but not limited to, the implied warranties of
 * merchantability and fitness for a particular purpose are disclaimed.
 * In no event shall the company or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 */

#include <iostream>
#include <fstream>

#include "FlowWatch.h"

FlowWatch::FlowWatch() {
	recFlows_ = 0;
	lastFlows_ = 0;
	lastSQ_ = 0;
	firstSQ_ = 0;
	reseted = true;
}

void
FlowWatch::reset(){
	reseted = true;
	recFlows_ = 0;
	lastFlows_ = 0;
	lastSQ_ = 0;
	firstSQ_ = 0;
}

void
FlowWatch::updateSQ(uint SQ){
	if(reseted == true){
		firstSQ_ = lastSQ_ = SQ;
		reseted = false;
	}else{
		if(SQ < firstSQ_){
			//detect SQ reset (modulo 2^32)
			if(firstSQ_ > SQ_TOP_LIMIT && SQ < SQ_BOT_LIMIT){
				//is this first packet with reseted SQ?
				if(lastSQ_ < SQ_BOT_LIMIT){
					if(lastSQ_ < SQ){
						lastSQ_ = SQ;
					}
				}else{
					lastSQ_ = SQ;
				}
			}
			//first packet or out of order packet with lesser SQ
			firstSQ_ = SQ;
		}
		if(SQ > lastSQ_){
			if(lastSQ_ < SQ_BOT_LIMIT && SQ > SQ_TOP_LIMIT){
				//do nothing out of order packet with SQ num before SQ reset
			}else{
				lastSQ_ = SQ;
			}
		}
	}
}

void
FlowWatch::addFlows(uint recFlows){
	lastFlows_ = recFlows;
	recFlows_ += recFlows;
}

uint
FlowWatch::exportedFlows(){
	uint expFlows;
	if(lastSQ_ < firstSQ_){
		expFlows = SQ_MAX - firstSQ_;
		expFlows += lastSQ_;
	}else{
		expFlows = lastSQ_ - firstSQ_;
	}
	return expFlows +lastFlows_;
}


int
FlowWatch::write(std::string dir){
	 std::ofstream flowsFile;

	 flowsFile.open((dir + "flowsStats.txt").c_str());
	 if(flowsFile.is_open()){
		 flowsFile << "Exported flows: "<< exportedFlows() << std::endl;
		 flowsFile << "Received flows: "<< receivedFlows() << std::endl;
		 flowsFile << "Lost flows: "<< exportedFlows() - receivedFlows() << std::endl;
	 	 flowsFile.close();
	 	 return 0;
	 }
	 return -1;
}

uint
FlowWatch::receivedFlows(){
	return recFlows_;
}


FlowWatch::~FlowWatch() {
}
