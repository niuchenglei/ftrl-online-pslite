#include "core/learner.h"
#include <unistd.h>
#include <vector>
#include <set>
#include <map>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <sys/time.h>
#include <ps/ps.h>

using namespace std;
using namespace ps;

#define FEATURE_CAPACITY 1000
#define ALLFEATURE_SIZE 10000000
#define max(a, b) (a>b)?a:b

extern "C" int __exit_flag__;
static void* __RSYNC__(void* ptr) {
    Learner* p = (Learner*)ptr;
    p->Start();
    return NULL;
}

static void validate(float* sum, std::vector<sample> &_sample_, int idx_) {   
    sum[0] = 0; sum[1] = 0; sum[2] = 0; sum[3] =0; sum[4] = 0; sum[5] = 0;
    int len = idx_;
    if (len == 0) return;
    for (int i=0; i< idx_; i++) {
        int label = _sample_[i].label;
        sum[0] += - log(label* _sample_[i].ctr + (1-label) * (1-_sample_[i].ctr) + 0.000001);
        sum[1] += - log(label* _sample_[i].pctr + (1-label) * (1-_sample_[i].pctr) + 0.000001);
        sum[2] += _sample_[i].ctr;
        sum[3] += _sample_[i].pctr;
    }
    sum[0] = sum[0]/(len);
    sum[1] = sum[1]/(len);
    sum[2] = sum[2]/(len);
    sum[3] = sum[3]/(len);
    for (int i=0; i< idx_; i++) {
        sum[4] += abs(_sample_[i].ctr - sum[2]);
        sum[5] += abs(_sample_[i].pctr - sum[3]);
    }

}

Learner::~Learner() {
    delete worker;
}


Learner::Learner(int app_id, map<string, string>& ini_config ) {
    this->appid = app_id;

    std::string model_name = ini_config["model"];
    int setparam_flag = atoi(ini_config["setparam_flag"].c_str());
    vector<float> param_vec;
    if (setparam_flag == 1) {
        param_vec.push_back(atof(ini_config["alpha"].c_str()));
        param_vec.push_back(atof(ini_config["beta"].c_str()));
        param_vec.push_back(atof(ini_config["l1"].c_str()));
        param_vec.push_back(atof(ini_config["l2"].c_str()));
    }
    recordlog = RecordLog();
    num_sample = 0;
    samples.resize(10000);
    unique_features.resize(10000000);
    
    worker = new ps::KVWorker<float> (app_id,0);
    ps::KVWorker<float> *kw = (ps::KVWorker<float>*) worker;

    vector<uint64_t> keys(1);
    vector<float> vals(1);
    vector<int> *len = nullptr;
    keys[0] = 1;
    vals[0] = 1.0;
    std::tr1::unordered_map<uint64_t,float>().swap(W);

    kw->Wait(kw->Pull(keys, &vals, len, 1));
    if (fabs(vals[0]-1.0) < 0.0001) {
        if(!LoadModel(model_name))
            INFO("load model failed");
        INFO("load model success");
        vector<uint64_t>().swap(keys);//keys.clear();
        vector<float>().swap(vals); //.clear();
        for (std::tr1::unordered_map<uint64_t,float>::iterator iter=W.begin(); iter != W.end(); iter++) {
            keys.push_back(iter->first);
            vals.push_back(iter->second);
        }   
        kw->Wait(kw->Push(keys,vals,{},1));
    }

    if (setparam_flag == 1) {
        keys.resize(4);
        printf("set param from worker\n");
        vals.assign(param_vec.begin(), param_vec.end());
        kw->Wait(kw->Push(keys,vals,{},2));
     }

   // pthread_t rsync_thread;
    sem_init(&sem_done, 0, 1);
    sem_init(&sem_ready, 0, 0);

    if (pthread_create(&update_thread, NULL, __RSYNC__, this) != 0) {
        printf("can't create thread\n");
        fprintf(stderr, "lworker run thread create");
        return;
     }
    printf("learner init sucess\n");
    INFO ("Learner + "+ std::to_string(this->appid) +"init success");

}
bool Learner::LoadModel(const std::string& model_name) {
    ifstream ifs(model_name.c_str());
    printf("file name is %s\n",model_name.c_str());
    if (!ifs) {
        ERROR("can'f find model file");
        return false;
    }

    string line;
    int cntt = 0;
    vector<string> tmp;
    while (getline(ifs, line)) {
        tmp=split(line,',');
        cntt++;
        if (tmp.size() < 2)
            continue;
        float weight = 0;
        if(cntt == 1) {
            W[0] = 0;
            if (isFloat(tmp[1], &weight))
                W[0] = weight;
            continue;
        }
        uint64_t feature_id = 0;
        if ((tmp.size() == 2) && isUInt64(tmp[0], &feature_id) && isFloat(tmp[1], &weight)) {
            W[feature_id] = weight;
        }
    }
    return true;
}

int Learner::CheckLabel(std::tr1::unordered_map<string,string> &condition) {
    string code = condition["bhv_code"], filter = condition["filter"];
    string play_duration = condition["play_duration"], isautoplay = condition["isautoplay"], open_app = condition["open_app"];
    if (std::find(table_code.begin(), table_code.end(), code) == table_code.end()) {
        return -1; }
    if (filter != "0" && filter != "10000") {
       return -1;
    }
    uint64_t  play_d = 3500, autoplay = 0, openapp = 3;
    if (play_duration.size() > 0) {
        isUInt64(play_duration, &play_d);
    }
    if (isautoplay.size() > 0)
        isUInt64(isautoplay, &autoplay);
    if (open_app.size() > 0)
        isUInt64(open_app, &openapp);
    if ((play_d >= 3000 || autoplay == 0) &&
        (openapp != 1 && openapp != 2)) {
        return 1;
    } else {
        return -1;
    }
}
void Learner::GetData(vector<sample>& Xs, int num_sample, vector<int>&Y) {
    sem_wait(&sem_done);
    int idx = 0;
    for (int i= 0; i< num_sample; i++) {
        if (Y[i] == -1) continue;
        recordlog.StatSampleNum(Y[i]);
        samples[idx].label = Y[i];
        samples[idx].ctr = Xs[i].ctr;
        samples[idx].length = Xs[i].length;
        memcpy(samples[idx]._mem, Xs[i]._mem, sizeof(uint64_t)*Xs[i].length);
        idx++;
    }
    this->num_sample = idx;
    sem_post(&sem_ready);
    stat = 3;
}
void Learner::Start() {
    while (true) {
        sem_wait(&sem_ready);
        update();
        sem_post(&sem_done);
        stat = 2;

        if (__exit_flag__ == 1) {
            stat = 4;
            pthread_join(update_thread, NULL);
            break;
        }
    }
    stat = 5;
}

bool Learner::Wait(){
    while (stat != 5) {
        sleep(10);
    }
    return true;
}     

void Learner::update() {
    struct timeval btime, etime;
    gettimeofday(&btime,NULL);
    if (num_sample == 0) return;
    unique_features[0] = 0;
    int u_idx = 1; 
    for (int i=0; i< num_sample; i++) {
        memcpy(&unique_features[u_idx], samples[i]._mem, samples[i].length*sizeof(uint64_t)); 
        u_idx += samples[i].length; 
    }
    std::set<uint64_t> sk(unique_features.begin(), unique_features.begin()+u_idx);
    std::vector<uint64_t> _unique;
    _unique.assign(sk.begin(), sk.end());
    int key_size = _unique.size();
    ps::KVWorker<float> *kw = (ps::KVWorker<float>*) worker;
    std::vector<float> w(key_size);
    kw->Wait(kw->Pull(_unique, &w));
    for (int k=0; k<key_size; k++) {
        W[_unique[k]] = w[k];
        P[_unique[k]] = k;
     }
    std::vector<float> grad(key_size);

    std::vector<int> grad_acc(key_size, 0);
    predict(samples, num_sample, W);
    caculate_gradient(samples, num_sample, P, grad, grad_acc);
        
    float lr[6];
    validate(lr, samples, num_sample);
        
    float ll_online = lr[0], ll_pred = lr[1], avr_online = lr[2], avr_pred = lr[3], var_online = lr[4],  var_pred = lr[5];
    //printf("ll_online %f ll_pred %f avr_online %f avr_pred %f\n",ll_online, ll_pred,avr_online,avr_pred);
    recordlog.StatInfo(ll_online, ll_pred, avr_online, avr_pred, var_online, var_pred);
    kw->Wait(kw->Push(_unique, grad));
    gettimeofday(&etime,NULL);
    float timeuse = (etime.tv_sec - btime.tv_sec)*1000 + (etime.tv_usec - btime.tv_usec)/1000.0;
    recordlog.StatQPS(timeuse);
    recordlog.Printbyapp(appid);

    if (W.size() > 10000000) {
        std::tr1::unordered_map<uint64_t, float>().swap(W);
    }
    if (P.size() > 10000000) {
        std::tr1::unordered_map<uint64_t, float>().swap(P);
    }
  
}
    // init
void Learner::predict(vector<sample> &_sample_, int idx_, std::tr1::unordered_map<uint64_t,float> &W) {
   for (int i=0; i< idx_; i++) {
       float sum = 0;
       for (int j=0; j< _sample_[i].length; j++) {
           sum += W[_sample_[i]._mem[j]];
       }
       sum += W[0];
    //   printf("predict value %f\n", sigmoid(sum));
       _sample_[i].pctr = sigmoid(sum);
    }
}

void Learner::caculate_gradient(vector<sample> &_sample_, int idx_, std::tr1::unordered_map<uint64_t,float> &P, vector<float>&grad, vector<int>& grad_acc) {
    for (int i=0; i< idx_; i++) {
        for (int j=0; j<_sample_[i].length; j++) {
            int skey = _sample_[i]._mem[j];
            int pos = P[skey];
            grad[pos] += _sample_[i].pctr - _sample_[i].label;
            grad_acc[pos]++;
        }
        grad[0] += _sample_[i].pctr - _sample_[i].label;
        grad_acc[0]++;
   }
}
