#include <iostream>
#include <ps/ps.h>
#include "ps_server.h"
#include <fstream>
#include <time.h>
using namespace std;
using namespace ps;

static void* __RSYNC__(void* ptr) {
    PSS* p = (PSS*)ptr;
    p->SyncToRedis();
    return NULL;
}

PSS::PSS(int appid,  map<string, string> &server_conf) {
    this->app_id = appid;
    this->save_path = server_conf["save_path"];
    this->save_frequence = atoi(server_conf["save_frequence"].c_str());
}

void PSS::StartFtrl() {

    pthread_t rsync_thread;
    if (pthread_create(&rsync_thread, NULL, __RSYNC__, this) != 0) {
        printf("can't create thread\n");
        fprintf(stderr, "redis clus");
        return; 
    }

    ftrl = new Ftrl();
    ftrl->set_param(0.02, 1.0, 0.00001, 0.0001);
    auto server = new KVServer<float>(app_id);
    printf("server id %d \n", app_id);
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    std::function<void(const ps::KVMeta&, const ps::KVPairs<float>&, ps::KVServer<float>*)> func = std::bind(&Ftrl::handle_request, ftrl, _1, _2, _3);
    server->set_request_handle(func);
    RegisterExitCallback([server](){ delete server; });
   printf("over\n");
}

void PSS::SyncToRedis() {
    vector<string> buffer; 
    int32_t expire_time = 48*3600;

    while (1) {
        sleep(save_frequence*60);
        std::unordered_map<uint64_t, float>& data1 = ftrl->get_weight(); // ftrl->W;
        std::unordered_map<uint64_t, uint64_t>& acc = ftrl->get_acc();

        std::map<uint64_t, float> data(data1.begin(), data1.end());
        time_t tt=time(NULL);
        tm* t =localtime(&tt);
        string file = save_path+"/model_"+std::to_string(t->tm_hour)+"_"+std::to_string(t->tm_min);
        //string file = "models/m_"+std::to_string(app_id)+"/model_"+std::to_string(t->tm_hour)+"_"+std::to_string(t->tm_min);
        ofstream outfile(file,ios::app);
        
        std::map<uint64_t, float>::iterator iter = data.begin();
        int i=1;
        char buf[64]; 
        for (; iter!= data.end(); iter++) {
            uint64_t keyid = iter->first;
            float wval = data[keyid];
            sprintf(buf, "%.8f", wval);

            outfile<<keyid<<","<< string(buf) <<"\n";

            if (i>10000000)
                break;
            i++;
        }
        outfile.close();
       }
 }


