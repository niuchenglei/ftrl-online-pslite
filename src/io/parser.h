#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <mutex> //using std::tr1::unordered_map;
#include <time.h>

#include "utils/logger.h"
#include "utils/loginfo.h"
#include "core/x_learner.h"
#include "utils/string_util.h"
#include "io/sample.h"

class Parser  {
public:
    Parser();
    void Process(const char* _msg, int64_t _len, int param);
    void Start();
    void SetLearner(std::vector<LearnerBase*> &lws);

private:
    float cntl[4];
    RecordLog recordlog;
    int appnum;
    vector<LearnerBase*> lw;

    int _idx_0, _idx_1;  // 当前负样本、正样本索引
    vector<sample> sample_0, sample_1;
    bool volatile mutex_flag;
    vector<vector<int> > Ys_bhv, Ys_imp;

    std::tr1::unordered_map<uint64_t, int> markid_hashmap, markid_hashmap0;
    int pre_st = 0;

    float parse_sample_0(std::vector<uint64_t>& features, std::tr1::unordered_map<string, string>& condition, const char* _msg, int64_t _len);
    float parse_sample_1(std::vector<uint64_t>& features, std::tr1::unordered_map<string, string>& condition, const char* _msg, int64_t _len);
};

#endif
