#include "sample_kafka.h"
#include "utils/logger.h"
using namespace std;
extern "C" int __exit_flag__ ;

int CALLBACK_SHELL(const char* msg, int64_t len, int param, void* ptr) {
    Parser *p = static_cast<Parser*>(ptr);
    p->Process(msg, len, param);
    
    return 0;
}
typedef struct {
    MessageKafka *msg_kafka;
    Parser* parser;
} param_t;

void* pthread(void* ptr) {
    pid_t pid;
    pthread_t tid;
    pid = getpid();
    tid = pthread_self();
    param_t* _param = (param_t*)ptr;
    _param->msg_kafka->Process(CALLBACK_SHELL, _param->parser);
}
SampleKafka::~SampleKafka() {
    delete message_bhv;
    delete message_imp;
    delete parser;
}

SampleKafka::SampleKafka(std::vector<LearnerBase*>& learners, map<string,string> &ini_config) {
    parser = new Parser();
    parser->SetLearner(learners);

    workid = atoi(ini_config["worker_idx"].c_str());
    stat = 0;
    {
        message_bhv = new MessageKafka();
        string brokers_bhv = ini_config["bhv_broker"], group_bhv = ini_config["group"], topic_bhv =  ini_config["bhv_topic"];
        vector <int> bhv_part;
        bhv_part.push_back(workid);    
        INFO("try to read partion" +std::to_string(workid));
        message_bhv->Init(brokers_bhv.c_str(), group_bhv.c_str(), topic_bhv.c_str(),bhv_part, true, workid);
    }
    {
        message_imp = new MessageKafka();
        string brokers_imp  = ini_config["imp_broker"], group_imp = ini_config["group"], topic_imp = ini_config["imp_topic"];//"10.39.40.101:9110,10.39.40.102:9110,10.39.40.103:9110,10.39.40.104:9110", group_imp = "ads_algo", topic_imp = "wb_ad_sfstbimptf";
        std::vector<int> parts;
        for (int k=0; k<4; k++) {
            parts.push_back(workid*4+k);    //message_imp.setParser(parser);
            INFO("try to read partion" +std::to_string(workid*4+k));
        }
        message_imp->Init(brokers_imp.c_str(), group_imp.c_str(), topic_imp.c_str(), parts, false, workid);
    }
    stat = 1;
}
void SampleKafka::Start() {
    pthread_t pbhv, pimp;
    stat = 2;
    {
    param_t *_param=new param_t() ; _param->msg_kafka = message_bhv;  _param->parser = parser;
     if (pthread_create(&pbhv, NULL, pthread, _param) != 0) {
        exit(1);
     }
    }

    {
        // 启动线程读取曝光日志
    param_t *_param = new param_t(); _param->msg_kafka =message_imp; _param->parser = parser;
    if (pthread_create(&pimp, NULL, pthread, _param) != 0) {
        exit(1);
    }

    }
    stat = 3;
    pthread_join(pimp, NULL);
    pthread_join(pbhv, NULL);
    stat = 4;
    /*while (true) {
        if (__exit_flag__ == 1) {
            pthread_join(pimp, NULL);
            pthread_join(pbhv, NULL);
            stat = 3;
            break;
        }
    }*/
    //stat = 4;
}
bool SampleKafka::Wait() {
    while (stat != 4 ) {
        sleep(10);
   }
   return true;
}
 

