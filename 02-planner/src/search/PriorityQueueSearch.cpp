/*
 * PriorityQueueSearch.cpp
 *
 *  Created on: 10.09.2017
 *      Author: Daniel Höller
 */

#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <cassert>
#include <chrono>
#include <fstream>

#include "PriorityQueueSearch.h"
#include "../ProgressionNetwork.h"
#include "../Model.h"
#include "../intDataStructures/FlexIntStack.h"


#ifdef PREFMOD
// preferred modifications
#include "AlternatingFringe.h"
#endif

#include <queue>
#include <map>
#include <algorithm>
#include <bitset>

namespace progression {

PriorityQueueSearch::PriorityQueueSearch() {
	// TODO Auto-generated constructor stub
}

PriorityQueueSearch::~PriorityQueueSearch() {
	// TODO Auto-generated destructor stub
}

searchNode* PriorityQueueSearch::handleNewSolution(searchNode* newSol, searchNode* oldSol, long time, bool optSol) {
	searchNode* res;
	foundSols++;
    bool betterSol = false;
	if(oldSol == nullptr) {
        cout << "- Status: Solved" << endl;
		res = newSol;
		firstSolTime = time;
		bestSolTime = time;
		if (optSol) {
			cout << "SOLUTION: (" << time << "ms) Found first solution with action costs of " << newSol->actionCosts << "." << endl;
		}
        betterSol = true;
	} else if(newSol->actionCosts < oldSol->actionCosts) {
		// shall optimize until time limit, this is a better one
		bestSolTime = time;
		res = newSol;
		solImproved++;
		cout << "SOLUTION: (" << time << "ms) Found new solution with action costs of " << newSol->actionCosts << "." << endl;
        betterSol = true;
	} else {
		// cout << "Found new solution with action costs of " << newSol->actionCosts << "." << endl;
		res = oldSol;
	}
    if (optSol && betterSol) {
        auto[sol, sLength] = extractSolutionFromSearchNode(htnmodel, res);
        cout << "- Found solution of length " << sLength << endl;
        cout << "- Total costs of actions: " << res->actionCosts << endl;
        ofstream newsol;
        newsol.open ("solution.raw", std::ofstream::out | std::ofstream::trunc);
        newsol << sol;
        newsol.close();
        // cout << sol << endl;

        //string cmd = "~/CLionProjects/PandaDealerFork/00-parser/pandaPIparser -c solution.raw " + this->solFileName;
        string cmd = "/planner/pandaPIparser -c solution.raw " + this->solFileName;
        cout << "executing: " << cmd << endl;
        int ret = system(cmd.c_str());
    }
	return res;
}

/// closing namespace
}

