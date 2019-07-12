#ifndef FTRL_H
#define FTRL_H

#include <vector>
#include <unordered_map>
#include <ps/ps.h>

using namespace ps;

class Ftrl {
public:
    Ftrl() {this->set_flag=false;}; //float alpha,float beta,float l1,float l2);
    ~Ftrl() {}
    std::unordered_map<uint64_t,float>& get_weight(){ return W;}
    std::unordered_map<uint64_t, uint64_t>& get_acc() { return acc; }

    void update(KVPairs<float> &res,const KVPairs<float> &req);
    void handle_request(const KVMeta& req_meta, const KVPairs<float>& req_data, KVServer<float>* server);
    void set_weight(KVPairs<float> &res, const KVPairs<float>&req) ; 
    void set_param(float alpha,float beta,float l1,float l2);
    void set_paramfromkv(KVPairs<float> &res, const KVPairs<float>&req);

    float alpha, beta, lamda1, lamda2, freq;
    bool set_flag;
    std::unordered_map<uint64_t, float>W, Z, N;
    std::unordered_map<uint64_t, uint64_t> acc;
};

#endif
