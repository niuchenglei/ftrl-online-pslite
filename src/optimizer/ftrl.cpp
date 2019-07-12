#include "ftrl.h"
#include <math.h>
#include <fstream>

using namespace std;
using namespace ps;

// 0.02, 1.0, 0.0001, 0.0002
void Ftrl::set_param (float alpha,float beta,float l1,float l2) { //float alpha,beta,l1,l2) {
    this->alpha = alpha;
    this->beta = beta;
    this->lamda1 = l1;
    this->lamda2 = l2;
}

void Ftrl::update(KVPairs<float> &res, const KVPairs<float> &req) {
    time_t t = time(0);
    char tmp[64];

    //printf(" this alpha %f beta %f l1 %f l2 %f\n",this->alpha, this->beta, this->lamda1, this->lamda2);
    strftime(tmp, sizeof(tmp), "%H:$M", localtime(&t));  // %Y/%m/%d %X %A 本年第%j天 %z
    string mints = string(tmp);
    bool reset = false;
    if (mints == "00:00" || mints == "0:0")
        reset = true;

    for (size_t i=0; i<res.keys.size(); ++i) {
        uint64_t key = res.keys[i];
        //if (key<10) {W[key] }
        acc[key]++;
        if (acc[key] > 1000000 && reset) 
           acc[key] = 1000000;

        float g = req.vals[i];
        float Ni = N[key] + g*g;
        Z[key] += g - (std::sqrt(Ni)-std::sqrt(N[key]))/alpha*W[key];
        N[key] = Ni;
        if (fabs(Z[key]) <= lamda1) {
            W[key] = 0;
        }
        else {
            float sign = (Z[key] > 0.0)?1:-1;
            float tmpr = Z[key] - sign*lamda1;
            float tmpl = -((beta+std::sqrt(N[key]))/alpha+lamda2);
            W[key] = tmpr/tmpl;
        }
       //if (key <10 ) printf("serve W[%d] %f\n", key, W[key]);
    }
}

void Ftrl::set_paramfromkv(KVPairs<float> &res, const KVPairs<float>&req) {
     if (res.keys.size()!=4 || req.vals.size()!=4)
         printf("set param error\n"); // (size_t i=0; i<res.keys.size(); ++i) {
      
     this->alpha = req.vals[0];
     this->beta = req.vals[1];
     this->lamda1 = req.vals[2];
     this->lamda2 = req.vals[3];
     printf("alpha:%f, beta:%f, l1:%f, l2:%f\n", this->alpha, this->beta, this->lamda1, this->lamda2);
}
        
void Ftrl::set_weight(KVPairs<float> &res, const KVPairs<float>&req) {
   // printf("set weight\n");
    for (size_t i=0; i<res.keys.size(); ++i) {
        uint64_t key = res.keys[i];
        float v = req.vals[i];
        if (key == 0)
            printf("W[0]:%f\n",v);
        W[key] = v;
        acc[key] = 1000000;
    }
    set_flag=true;
}

void Ftrl::handle_request(const KVMeta& req_meta, const KVPairs<float>& req_data, KVServer<float>* server) {
    size_t n = req_data.keys.size();
    KVPairs<float> res;
    res.keys = req_data.keys;
    if (req_meta.push) { // push
      //  w_len = req_data.vals.size()/n;
        CHECK_EQ(req_data.vals.size(), n);
    } else {            // pull
        //res.keys = req_data.keys;
        res.vals.resize(n);
        //res.lens.resize(res.keys.size());
    }
    
    if (req_meta.push) {
        //printf("cmd is %d\n",req_meta.cmd);
        if (req_meta.cmd == 1) {
            set_weight(res, req_data);
        }
        else if (req_meta.cmd == 2) {
            set_paramfromkv(res, req_data);
        }
        else
	    update(res, req_data);
    } else {
        if(req_meta.cmd == 1) {
        //    printf("pull \n cmd ==1 \n");
            if (set_flag)
                res.vals[0] = 0.0;
            else 
                res.vals[0] = 1.0;
        }
        else {
            for (size_t i=0; i<n; ++i) {
                uint64_t key = res.keys[i];
                res.vals[i] = W[key];
            } 
        }
    }  
    server->Response(req_meta, res);
}
