#ifndef LEARNER_X_H
#define LEARNER_X_H

#include "core/learner.h"

class igLearner: public Learner{
public:
    igLearner(int app_id, map<string, string>& ini_config):Learner(app_id, ini_config){};
    int CheckLabel(std::tr1::unordered_map<string,string> &condition);
};

class isLearner: public Learner{
public:
    isLearner(int app_id, map<string, string>& ini_config):Learner(app_id, ini_config){};
    int CheckLabel(std::tr1::unordered_map<string,string> &condition);
};

class irLearner: public Learner{
public:
    irLearner(int app_id, map<string, string>& ini_config):Learner(app_id, ini_config){};
    int CheckLabel(std::tr1::unordered_map<string,string> &condition);
};

class ivCPELearner: public Learner{
public:
    ivCPELearner(int app_id, map<string, string>& ini_config):Learner(app_id, ini_config){};
    int CheckImp(std::tr1::unordered_map<string,string> &condition);
    int CheckLabel(std::tr1::unordered_map<string,string> &condition);
};

#endif
