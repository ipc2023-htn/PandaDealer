//
// Created by dh on 02.01.23.
//

#ifndef PANDAPIENGINE_DOFLMFACTORY_H
#define PANDAPIENGINE_DOFLMFACTORY_H

#include "landmarks/lmExtraction/lmGraph.h"
#include "../../Model.h"

class dofLmFactory {

public:
    lmGraph *createLMs(Model *htn);
};


#endif //PANDAPIENGINE_DOFLMFACTORY_H
