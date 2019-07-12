#define _BSD_SOURCE

#include "learner.h"
#include <unistd.h>
#include <vector>
#include <set>
#include <map>
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <sys/time.h>
#include "common.h"
using namespace std;
#define FEATURE_CAPACITY 1000
#define CONST_FEATURE_CONFIG "CONST"
#define ALLFEATURE_SIZE 10000000
#define max(a, b) (a>b)?a:b
#define filtermsg "fr-fusion1"

extern uint64_t __crc64_table__[8][256];
static uint64_t crc64(const char *buffer) {
    uint64_t crc = 0;

    int len = strlen(buffer);
    while (len >= 8) {
        crc ^= *(uint64_t*)buffer;
        crc = __crc64_table__[7][crc & 0xff] ^
              __crc64_table__[6][(crc >> 8) & 0xff] ^
              __crc64_table__[5][(crc >> 16) & 0xff] ^
              __crc64_table__[4][(crc >> 24) & 0xff] ^
              __crc64_table__[3][(crc >> 32) & 0xff] ^
              __crc64_table__[2][(crc >> 40) & 0xff] ^
              __crc64_table__[1][(crc >> 48) & 0xff] ^
              __crc64_table__[0][crc >> 56];
        buffer += 8;
        len -= 8;
    }
    while (len) {
        crc = __crc64_table__[0][(crc ^ *buffer++) & 0xff] ^ (crc >> 8);
        len--;
    }
    return crc;
}

static vector<uint64_t> extract_feature(string feature_str, float* ig_ctr) {
    //string separator="fr-fusion1:0.1,0.2,0.3,0.4,123,213";
    // fr-fusion1:0.017308,0.000336,0.002577,0.0081735466898,107538485,1
    // fr-fusion1:0.017308,0.000336,0.002577,0.008173,0.008172,5466898,107538485,1
    vector<uint64_t> result;
    auto first = feature_str.find(filtermsg);  //fr-fusion_ftrl
    if (first == string::npos)
	return result;
    string str_s = feature_str.substr(first+11);
    vector<string> str_split = split(str_s,',');
    *ig_ctr = atof(str_split[1].c_str());
    if (str_split[3].size() > 8) {
        int key = std::atoi(str_split[3].substr(8).c_str());
        result.push_back(uint64_t(key));
    }

    time_t t = time(0);    
    char tmp[4];
    strftime(tmp, sizeof(tmp), "%M", localtime(&t));
    int mints = atoi(tmp);
    float rto = (mints<30)?0:1.0;
    if (mints >= 30) {
        rto = ((float)mints - 30.0)/30.0;
    }
    for (int i=4; i<str_split.size(); i++) {    
	int temp = std::atoi(str_split[i].c_str());
        if (temp < 1) continue;

	result.push_back(uint64_t(temp));
    }
    return result;
}


Learner::Learner(int app_num, vector<LWorker*> &lws ) {
    for (int k=0; k<4; k++) cntl[k] = 0;
    lw = lws; //.push_back(lws[i]);
    proinfos = prof_t();
    sample_0.resize(10000);
    sample_1.resize(3000);
    _idx_0 = 0 ; _idx_1 = 0;
    mutex_flag = true;  
    appnum = app_num;
    threadpool_init(&pool, app_num);

    table_code={"14000014","22000001","22000002","25000001","25000002","27000001","27000002","27000003","27000005","27000006","29000001","29000002","29000003","29000005","50000005","60000001","60000003","80000006","80000010","80000011","80000030","80000060","80000070","80000089","50000009","50000010","60000010"};

    printf("learner init success\n");
}

float Learner::Parse_sample_0(std::vector<uint64_t>& features, const char* _msg, int64_t _len) {
    string msg = string(_msg);

    size_t overmsg = msg.find(filtermsg);
    if (overmsg == string::npos) {
        return -1;
    }
    int _c1 = count(msg, '\n');
   // int _c2 = count(msg, '\t');
    if (_c1 < 1 ) {
        return -1;
    }

    vector<string> array = split(msg, '\n');
    string features_str = "";
    if (array.size() < 1) {
        return -1;
    }

    string imp = array[0];  // 曝光日志
    int _c3 = count(imp, '');
    if (_c3 < 5) {
        return -1;
    }
     
    string state = last_state(imp);
    int state_i = atoi(state.c_str());
    if (state_i != 0) {
        return -1;
    }

    vector<string> sec = split(imp, '');
    vector<string> common = split(sec[0], '\t');
    if (common.size() < 2 ) {
        return -1;
    }//
    string unixtime = common[1].substr(0,10);
    int mt = std::atoi(unixtime.c_str());
    time_t t;
    t = time(NULL);
    int cur = (int) t;
    //printf("delay_0: %d\n", cur - mt);
    proinfos.test0(mt, cur);

    vector<string> psid = split(sec[1], '\t');
   
    string markid = psid[4];
    uint64_t hash_key = crc64(markid.c_str());
    markid_hashmap0[hash_key]++;
    if (markid_hashmap0[hash_key] > 1) {
        printf("bhv repeat(%s):%d\n", markid.c_str(), markid_hashmap0[hash_key]);
        return -1;
    }
    if (markid_hashmap0.size() > 50000000) {
        time_t t = time(0);
        char tmp[64];
        strftime(tmp, sizeof(tmp), "$M", localtime(&t));  // %Y/%m/%d %X %A 本年第%j天 %z
        string mints = string(tmp);
        if (mints == "00" || mints == "0")
            std::tr1::unordered_map<uint64_t, int>().swap(markid_hashmap0);
    }
    vector<string> ad = split(sec[3], '\t');
    vector<string> fraud = split(sec[4], '\t');
    if (fraud.size() < 2 ) {
        return -1;
    }
    if( ad.size() < 6)  {
        return -1;}
    string rank = ad [5];

    //string cust_uid = ad[8], adid = ad[0],  bid_type = ad[9], rank = ad[5], feedid = ad[6], promotion = ad[11];

    vector<string> rank_match = split(rank, ';');
    if (rank_match.size() < 17) {
        return -1;        
    }
    
    float ig_ctr = -1;
    features = extract_feature(rank_match[16], &ig_ctr);
    
    return ig_ctr;
}

float Learner::Parse_sample_1(std::vector<uint64_t>& features, const char* _msg, int64_t _len) {
    string msg = string(_msg);
    size_t overmsg = msg.find(filtermsg);
    
    if (overmsg == string::npos) {
        return -1;
    }
    int _c1 = count(msg, '\n'), _c2 = count(msg, '\t');
    if (_c1 < 3 ) {  //_c2 < 43
        return -1;
    }

    vector<string> array = split(msg, '\n');

    vector<string> bhvs = split(array[2], '');
    if (bhvs.size() <4 ) {
        return -1; 
    }
    vector<string> common = split(bhvs[0], '\t');
    if (common.size() < 8) { 
        return -1;}
    string code = common[7];
    if (std::find(table_code.begin(), table_code.end(), code) == table_code.end()) {
        return -1; }

    vector<string> addt = split(bhvs[3], '\t');
    if (addt.size() == 0) {  
       return -1;}
    string filter = addt[0];
    if (filter != "0" && filter != "10000") {
       return -1;
    }

    vector<string> extend = split(bhvs[2], '\t');
    if (extend.size() < 2) { return -1;}

    vector<string> imp = split(array[1], '');
    vector<string> mk = split(imp[1], '\t');
    string markid = mk[4];
    vector<string> ad = split(imp[3], '\t');
    if (ad.size() < 6 ) {
        return -1;
    }
    string rank = ad[5];
    vector<string> rank_match = split(rank, ';');
    if (rank_match.size() < 17) {
        return -1;
    }

    string unixtime = common[2].substr(0,10);
    int now = std::atoi(unixtime.c_str());
    time_t t = time(NULL);
    int cur = (int) t;
    proinfos.test1(now,cur);

    string extend1 = extend[0], extend2 = extend[1];

    markid += "-" + code + "-" + filter;
    uint64_t hash_key = crc64(markid.c_str());
    markid_hashmap[hash_key]++;
    if (markid_hashmap[hash_key] > 1) { 
        printf("bhv repeat(%s):%d\n", markid.c_str(), markid_hashmap[hash_key]);
        return -1;
    }
    if (markid_hashmap.size() > 50000000) {
        time_t t = time(0);
        char tmp[64];
        strftime(tmp, sizeof(tmp), "%H:$M", localtime(&t));  // %Y/%m/%d %X %A 本年第%j天 %z
        string mints = string(tmp);
        if (mints == "00:00" || mints == "0:0")
            std::tr1::unordered_map<uint64_t, int>().swap(markid_hashmap);
    }
   

    string play_duration, isautoplay, open_app;
    play_duration = find_kv(extend2, "play_duration");
    isautoplay = find_kv(extend2, "isautoplay");
    open_app = find_kv(extend2, "open_app");
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
    } else {
        return -1;
    }

    float ig_ctr = -1;
    features = extract_feature(rank_match[16], &ig_ctr);
    return ig_ctr;
}

void Learner::Merge(const char* _msg, int64_t _len, int param) {
    if (param == 0) {
        vector<uint64_t> features;
        float ig_ctr = Parse_sample_0(features, _msg, _len);
        if (ig_ctr == -1) {  return;}
        ig_ctr = max(ig_ctr, 0.00001);
        if (features.empty()) {
             return;}
        while (!mutex_flag) {
            usleep(100000); // sleep 100ms
        }
        if (_idx_0 >= 10000) {
            printf("more than 10000\n"); return;}
        sample_0[_idx_0].label = 0;
        sample_0[_idx_0].length = features.size();
        sample_0[_idx_0].ctr = ig_ctr;

        memcpy(sample_0[_idx_0]._mem, &features[0], sample_0[_idx_0].length*sizeof(uint64_t));
        while (!mutex_flag) {
            usleep(100000);
        }
        _idx_0++;

        cntl[0]++;
        proinfos.statis0();
        proinfos.proflearner();
    }
    else {
        vector<uint64_t> features;
        float ig_ctr = Parse_sample_1(features, _msg, _len);
        if (ig_ctr == -1) {  return;}

        ig_ctr = max(ig_ctr, 0.00001);
        if (features.empty()) {
            return;}
        if (_idx_1 >= 2000) {
            printf("more than 2000\n"); return; }

        sample_1[_idx_1].label = 1;
        sample_1[_idx_1].length = features.size();
        sample_1[_idx_1].ctr = ig_ctr;
       
        memcpy(sample_1[_idx_1]._mem, &features[0], sample_1[_idx_1].length*sizeof(uint64_t));
        proinfos.statis1(); cntl[1]++;
    }
    
    // 由互动线程触发PS-FTRL操作
    //proinfos.prof();
    if ((cntl[1] + cntl[0] >= 100) && param == 1) {
        cntl[1] = 0; cntl[0] = 0;
        printf(" param \n");   
        mutex_flag = false;
        
        for (int i=0, j=_idx_0; i<_idx_1; i++,j++) {
            sample_0[j].label = sample_1[i].label;
            sample_0[j].ctr = sample_1[i].ctr;
            sample_0[j].length = sample_1[i].length;
            memcpy(sample_0[j]._mem, sample_1[i]._mem, sizeof(uint64_t)*sample_1[i].length);
        }
        void * sam = (void *) &sample_0;
        int len =  _idx_0+_idx_1;
        for (int i=0; i<appnum; i++ ) {
            threadpool_add_task(&pool, lw[i]->Run, lw[i], sam, len);
        }
        while (true) {
            bool flag = true;
            for (int i=0; i<appnum; i++) {

                flag = flag && lw[i]->mutex_flag;
               // printf("flag[%d] %d\n",i, lw[i]->mutex_flag);
            }
            if (flag) break;
        }
        printf (" lw mutex flag has complete\n");
        _idx_0 = 0; _idx_1 = 0;
        mutex_flag = true;

       printf("one batch over\n");
    }
    //printf(" one batch over\n");
}
