#ifndef LEARNER_H
#define LEARNER_H
#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <mutex> //using std::tr1::unordered_map;
#include "logger.h"
#include <time.h>
#include "EventHandler.h"
#include "loginfo.h"
#include "LWorker.h"
#include "common.h"
#include "threadpool.h"

class Learner  {
public:
    Learner(int app_num, vector<LWorker*> &lws );
    void Merge(const char* _msg, int64_t _len, int param);
private:
    float cntl[4];
    prof_t proinfos;
    int  appnum;
    vector<LWorker*> lw;
    vector<sample> sample_0, sample_1;
    bool volatile mutex_flag;
    int _idx_0, _idx_1;
    std::vector<std::string> table_code;
    threadpool_t pool; //log4cpp::Category sub1;
    float Parse_sample_0(std::vector<uint64_t>& features, const char* _msg, int64_t _len);
    float Parse_sample_1(std::vector<uint64_t>& features, const char* _msg, int64_t _len);

    std::tr1::unordered_map<uint64_t, int> markid_hashmap, markid_hashmap0;
    int pre_st = 0;
};

#endif
