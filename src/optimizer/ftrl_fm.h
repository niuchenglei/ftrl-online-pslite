#ifndef FTRLFM_H
#define FTRLFM_H

#include <vector>
#include <unordered_map>
#include <ps/ps.h>

using namespace ps;

class FtrlFM {
public:
    FtrlFM() { this->set_flag = false; } //float alpha,float beta,float l1,float l2);
    ~FtrlFM() {}
    std::unordered_map<uint64_t, float>& get_w(){ return W;}
    std::unordered_map<uint64_t, std::unordered_map<unsigned char, float>>& get_v(){ return V;}
    std::unordered_map<uint64_t, uint64_t>& get_acc() { return acc; }

    void update(KVPairs<float>& res,const KVPairs<float>& req);
    void handle_request(const KVMeta& req_meta, const KVPairs<float>& req_data, KVServer<float>* server);
    void set_weight(KVPairs<float>& res, const KVPairs<float>& req); 
    void set_v(KVPairs<float>& res, const KVPairs<float>& req); 
    void set_param(float alpha, float beta, float l1, float l2);
    void set_paramfromkv(KVPairs<float>& res, const KVPairs<float>& req);

    float alpha, beta, lamda1, lamda2, freq, K;
    bool set_flag;
    std::unordered_map<uint64_t, float>W, Z, N;
    std::unordered_map<uint64_t, std::unordered_map<unsigned char, float>> V, ZV, NZ;
    std::unordered_map<uint64_t, uint64_t> acc;
};

#endif
