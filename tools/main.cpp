#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <ps/ps.h>
#include "logger.h"
#include "learner.h"
#include "message.h"
#include "pss.h"
#include "LWorker.h"
using namespace std;

static int __worker_idx = -1;
extern "C" int __get_worker_idx() {
    return __worker_idx;
}

int CALLBACK_SHELL(const char* msg, int64_t len, int param, void* ptr) {
    Learner *p = static_cast<Learner*>(ptr);
    p->Merge(msg, len, param);
    return true; 
} 
typedef struct {
    MessageKafka *msg_kafka;
    Learner* learner;
} param_t;

void* pthread(void* ptr) {
    pid_t pid;
    pthread_t tid;
    
    pid = getpid();
    tid = pthread_self();

    param_t* _param = (param_t*)ptr;
    //while(1) {
    printf("pid %lu tid %lu (0x%lx) (0x%lx)\n", (unsigned long)pid, (unsigned long)tid, (unsigned long)tid), _param;
    //}*/
    _param->msg_kafka->Process(CALLBACK_SHELL, _param->learner);

}

bool UpdateAux(void* ptr) {
    AutoUpdate *p = static_cast<AutoUpdate*>(ptr);
    p->Load();
    return true;
}
 
int main(int argc, char **argv) {
    char *worker_idx_c = "0";
    char *part_num_c = "3";
    char *init_model_c = "";
    char *init_model_c2 = "";
    char *redis_config_c = "";
    char *param_flag = "0";
    char *init_alpha = "";
    char *init_beta = "";
    char *init_l1 = "";
    char *init_l2 = "";
    int opt; 
    while ((opt = getopt(argc, argv, "n:i:m:e:r:f:a:b:l:h:eX:As:DO")) != -1){
        switch (opt) {
            case 'i':
                worker_idx_c = optarg;
                break;
            case 'n':
                part_num_c = optarg;
                break;
            case 'm':
                init_model_c = optarg;
                break;
            case 'e':
                init_model_c2 = optarg;
              //  break;
            case 'r':
                redis_config_c = optarg;
            case 'f':
                param_flag = optarg;
            case 'a':
                init_alpha = optarg;
            case 'b':
                init_beta = optarg;
            case 'l':
                init_l1 = optarg;
            case 'h':
                init_l2 = optarg;
            default:
                break;
        }
    }

    int worker_idx = atoi(worker_idx_c);
    __worker_idx = worker_idx;

    Logger &__logger__ = Logger::getInstance();
    int app_num = 2; 
    ps::StartAsync(0);
    if (ps::IsServer()) {
        for (int i=0; i<app_num; i++) {
            PSS *ps_ins = new PSS(i);
            ps_ins->StartFtrl();
        }
    }
 
    if (ps::IsWorker()) {
        vector<float>param_vec = {0.02, 1.0, 0.00001, 0.0001};
        if (atoi(param_flag) == 1) {
            param_vec[0] = atof(init_alpha);
            param_vec[1] = atof(init_beta);
            param_vec[2] = atof(init_l1);
            param_vec[3] = atof(init_l2);
        }
        vector<LWorker*> lws;
        for (int i=0; i < app_num; i++) {
            string model_file = string(init_model_c);
            LWorker *lw = new LWorker(i, model_file, atoi(param_flag), param_vec);
            lws.push_back(lw);
        }
        
        printf(" Lworker init success\n");
 
        pthread_t pbhv, pimp;
        Learner learner(app_num, lws);   //"/data0/home/chenglei3/ps-online/tools/yi_ctr_igl.model");

        {
        // 启动线程读取关联日志
        MessageKafka *mk_bhv=new MessageKafka();
        string brokers_bhv = "yz1429.kafka.data.sina.com.cn:9110,yz1425.kafka.data.sina.com.cn:9110,yz1435.kafka.data.sina.com.cn:9110,yz1438.kafka.data.sina.com.cn:9110,yz1434.kafka.data.sina.com.cn:9110", group_bhv="ads_algo", topic_bhv="wb_ad_sfst_logjoin_result";
        vector <int> bhv_part;
        bhv_part.push_back(worker_idx);
        INFO("try read partition: "+std::to_string(worker_idx));
        mk_bhv->Init(brokers_bhv.c_str(), group_bhv.c_str(), topic_bhv.c_str(),bhv_part, true, worker_idx);
        param_t *_param=new param_t() ; _param->msg_kafka = mk_bhv;  _param->learner = &learner; 
        if (pthread_create(&pbhv, NULL, pthread, _param) != 0) {
            //root.warn("can't create thread\n");
            exit(1);
         }
        }
 
        {
        // 启动线程读取曝光日志
        MessageKafka *mk_imp=new MessageKafka();
        string brokers_imp = "10.39.40.101:9110,10.39.40.102:9110,10.39.40.103:9110,10.39.40.104:9110", group_imp = "ads_algo", topic_imp = "wb_ad_sfstbimptf";
        std::vector<int> parts;
        for (int k=0; k<4; k++) {
            parts.push_back(worker_idx*4+k);
            //parts.push_back(39+k);
            INFO("try read partition: "+std::to_string(worker_idx*4+k));
        }
        mk_imp->Init(brokers_imp.c_str(), group_imp.c_str(), topic_imp.c_str(), parts,false, worker_idx);
        param_t *_param = new param_t(); _param->msg_kafka =mk_imp; _param->learner = &learner;
        if (pthread_create(&pimp, NULL, pthread, _param) != 0) {
            exit(1);
        }
        
        }

        pthread_join(pimp, NULL);
        pthread_join(pbhv, NULL);
        ps::Finalize(0, false);
        INFO(" worker stop");
        return 0;
    }
    while(1) {
        sleep(10);
    }

    return 0;
}
