#ifndef LEARNER_H
#define LEARNER_H
#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <mutex> 
#include <semaphore.h>
#include <time.h>

#include "io/sample.h"
#include "utils/loginfo.h"
#include "utils/string_util.h"

class LearnerBase {
public:
    virtual void GetData(vector<sample>& _sample_0, int len, vector<int>& Y) = 0;
    virtual void Start() = 0;
    virtual bool Wait() = 0;

    virtual int CheckImp(std::tr1::unordered_map<string,string> &condition) = 0;
    virtual int CheckLabel(std::tr1::unordered_map<string,string> &condition) = 0;
};

class Learner: public LearnerBase{
public:
    Learner(int app_id, map<string, string>& ini_config);
    ~Learner();

    void Start();
    bool Wait();
    void GetData(vector<sample>& Xs, int num_sample, vector<int>& Y);

    bool LoadModel(const std::string& model_name);

    virtual int CheckImp(std::tr1::unordered_map<string, string> &condition) { return 0;}
    virtual int CheckLabel(std::tr1::unordered_map<string, string> &condition);

private:
    int appid, stat;   // stat: 0/inited, 1/wait data, 2/wait update, 3/exiting, 4/finished
    void* worker;
    pthread_t update_thread;
    sem_t sem_ready, sem_done;

    std::vector<std::string> table_code;
    RecordLog recordlog;
 
    int num_sample;
    vector<sample> samples;
    std::vector<uint64_t> unique_features;
    std::tr1::unordered_map<uint64_t, float> W;
    std::tr1::unordered_map<uint64_t, float> P;

    void update();
    virtual void predict(vector<sample> &_sample_,int idx, std::tr1::unordered_map<uint64_t,float> &W);
    virtual void caculate_gradient(vector<sample> &_sample_, int idx, std::tr1::unordered_map<uint64_t,float> &P, vector<float>&grad, vector<int>& grad_acc);
};

#endif
