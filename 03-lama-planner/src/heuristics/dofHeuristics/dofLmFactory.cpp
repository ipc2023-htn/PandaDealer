//
// Created by dh on 02.01.23.
//

#include "dofLmFactory.h"
#include "hhDOfree.h"

lmGraph *dofLmFactory::createLMs(Model *htn) {
    searchNode *tnI = htn->prepareTNi(htn);
    htn->updateReachability(tnI);
    tnI->heuristicValue = new int[1];
    hhDOfree *hDof = new hhDOfree(htn,tnI,0,IloNumVar::Int,IloNumVar::Bool,progression::cSatisficing,progression::cTdgAllowUC,progression::cPgTimeRelaxed,progression::cAndOrLmsFull,progression::cLmcLmsFull,progression::cNetChangeFull,progression::cAddExternalLmsNo);
    hDof->setHeuristicValue(tnI, nullptr, -1);

    vector<int> taskCandidates;
    unordered_map<int, int> taskCount; // how often are the candidates included
    int candidates = 0;
    for (auto & containedTask : hDof->containedTasks) {
        taskCandidates.push_back(containedTask.first);
        taskCount.insert(containedTask);
        candidates += containedTask.second;
    }
    vector<int> methodCandidates;
    unordered_map<int, int> methodCount;
    for (auto & containedMethod : hDof->containedMethods) {
        methodCandidates.push_back(containedMethod.first);
        methodCount.insert(containedMethod);
        candidates += containedMethod.second;
    }

    cout << "- landmark candidates: " << candidates << endl;
    int calls = 1;
    unordered_map<int, int> taskLMs;
    int numTaskLMs = 0;
    unordered_map<int, int> methodLMs;
    int numMethodLMs = 0;
    for (int i = 0; i < taskCandidates.size(); i++) {
        int task = taskCandidates[i];
        if (task == -1){
            continue;
        }
        for (int limit = taskCount[task] - 1; limit >= 0; limit--) {
            hDof->doNotUseTasks.clear();
            hDof->doNotUseTasks.insert(make_pair(task, limit));
            hDof->setHeuristicValue(tnI, nullptr, -1);
            calls++;
            if (tnI->heuristicValue[0] == UNREACHABLE) {
                taskLMs.insert(make_pair(task, limit + 1));
                numTaskLMs += (limit + 1);
                if (limit > 0) {
                    cout << "- LM " << htn->taskNames[task] << " needed " << (limit + 1) << " times." << endl;
                }
            } else {
                for (int j = i + 1; j < taskCandidates.size(); j++) {
                    int otherTask = taskCandidates[j];
                    if (otherTask == -1) {
                        continue;
                    } else if (hDof->containedTasks.find(otherTask) == hDof->containedTasks.end()) {
                        taskCandidates[j] = -1;
                    } else {
                        int count = taskCount[otherTask];
                        taskCount[otherTask] = min(count, hDof->containedTasks[otherTask]);
                        if (taskCount[otherTask] == 0) {
                            taskCandidates[j] = -1;
                        }
                    }
                }
                for (int j = 0; j < methodCandidates.size(); j++) {
                    int method = methodCandidates[j];
                    if (method == -1) {
                        continue;
                    } else if (hDof->containedMethods.find(method) == hDof->containedMethods.end()) {
                        methodCandidates[j] = -1;
                    } else {
                        int count = methodCount[method];
                        methodCount[method] = min(count, hDof->containedMethods[method]);
                        if (methodCount[method] == 0) {
                            methodCandidates[j] = -1;
                        }
                    }
                }
            }
        }
    }
    hDof->doNotUseTasks.clear();

    for (int i = 0; i < methodCandidates.size(); i++) {
        int method = methodCandidates[i];
        if (method == -1){
            continue;
        }
        for (int limit = methodCount[method] - 1; limit >= 0; limit--) {
            hDof->doNotUseMethods.clear();
            hDof->doNotUseMethods.insert(make_pair(method, 0));
            hDof->setHeuristicValue(tnI, nullptr, -1);
            calls++;
            if (tnI->heuristicValue[0] == UNREACHABLE) {
                methodLMs.insert(make_pair(method, limit + 1));
                numMethodLMs += (limit + 1);
                if (limit > 0) {
                    cout << "- LM " << htn->methodNames[method] << " needed " << (limit + 1) << " times." << endl;
                }
            } else {
                for (int j = i + 1; j < methodCandidates.size(); j++) {
                    int otherMethod = methodCandidates[j];
                    if (otherMethod == -1) {
                        continue;
                    } else if (hDof->containedMethods.find(otherMethod) == hDof->containedMethods.end()) {
                        methodCandidates[j] = -1;
                    } else {
                        int count = methodCount[method];
                        methodCount[method] = min(count, hDof->containedMethods[method]);
                        if (methodCount[method] == 0) {
                            methodCandidates[j] = -1;
                        }
                    }
                }
            }
        }
    }
    delete tnI;
    lmGraph *g = new lmGraph();
    g->numLMs = numTaskLMs + numMethodLMs;
    g->lms = new lmNode[g->numLMs];

    vector<pair<int,int>> orderings;
    int i = 0;
    for (auto lm : taskLMs) {
        for (int count = 0; count < lm.second; count++) {
            g->lms[i].connection = disjunctive;
            tLmAtom *atom = new tLmAtom();
            if (lm.first < htn->numActions) {
                atom->type = action;
            } else {
                atom->type = task;
            }
            atom->lm = lm.first;
            atom->isNegated = false;
            g->lms[i].lm.push_back(atom);
            if (count > 0) {
                orderings.push_back(make_pair(i - 1, i));
            }
            i++;
        }
    }
    for (auto lm : methodLMs) {
        for (int count = 0; count < lm.second; count++) {
            g->lms[i].connection = disjunctive;
            tLmAtom *atom = new tLmAtom();
            atom->type = METHOD;
            atom->lm = lm.first;
            atom->isNegated = false;
            g->lms[i].lm.push_back(atom);
            if (count > 0) {
                orderings.push_back(make_pair(i - 1, i));
            }
            i++;
        }
    }
    g->initOrderings();
    for (pair<int, int> ordering : orderings) {
        g->addOrdering(ordering.first, ordering.second, ORD_NATURAL);
    }

    cout << "- needed " << calls << " calls to DOF heuristic. [dofCalls=" << calls << "]" << endl;
    return g;
}

