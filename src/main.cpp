#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <ps/ps.h>
#include "utils/logger.h"
#include "io/sample_kafka.h"
#include <fstream>
#include "core/ps_server.h"
#include "core/x_learner.h"
#include "utils/INIReader.h"
using namespace std;

static int __worker_idx = -1;
extern "C" int __get_worker_idx() {
    return __worker_idx;
}

extern "C" int __exit_flag__ = 0;
void signal_handler_func(int signal) {
    __exit_flag__ = 1;
}
 
int main(int argc, char **argv) {
    //signal(2, signal_handler_func);
    //signal(9, signal_handler_func);

    char *worker_idx_c = "0";
    char *app_num_c = "1";
    char *init_model_c = "";
    char *redis_config_c = "";
    int opt; 
    while ((opt = getopt(argc, argv, "i:n:m:e:r:f:a:b:l:h:eX:As:DO")) != -1){
        switch (opt) {
            case 'i':
                worker_idx_c = optarg;
                break;
            case 'n':
                app_num_c = optarg;
                break;
            case 'm':
                init_model_c = optarg;
            default:
                break;
        }
    }

    int worker_idx = atoi(worker_idx_c);
    __worker_idx = worker_idx;

    Logger &__logger__ = Logger::getInstance();
    int app_num = atoi(app_num_c); 
    INFO("app_num" +std::to_string(app_num));
    ps::StartAsync(0);

    if (ps::IsServer()) {
        string mModelFile = string(init_model_c);
        ifstream ifs(mModelFile.c_str());
        if (!ifs) {
            return false;
        }
        INIReader *config_file = new INIReader(mModelFile);
        if (config_file == NULL) {
            return false;
        }
        for (int i=0; i<app_num; i++) {
            map<string, string> server_conf;
            server_conf["save_path"] = config_file->Get("server_"+std::to_string(i), "save_path", "");
            server_conf["save_frequence"] = config_file->Get("server_"+std::to_string(i), "save_frequence", "");
            PSS *ps_ins = new PSS(i, server_conf);
            ps_ins->StartFtrl();
        }
    }
     
    if (ps::IsWorker()) {
        string mModelFile = string(init_model_c);
        ifstream ifs(mModelFile.c_str());
        if (!ifs) {
            return false;
        }

        INIReader *config_file = new INIReader(mModelFile);
        if (config_file == NULL) {
           return false;
        }
        map<string, string> ini_config;
        ini_config["imp_topic"] = config_file->Get("sample_message", "imp_topic", "");
        ini_config["imp_broker"] = config_file->Get("sample_message", "imp_broker", "");
        ini_config["group"] = config_file->Get("sample_message", "group", "");
        ini_config["bhv_topic"] = config_file->Get("sample_message", "bhv_topic", "");
        ini_config["bhv_broker"] = config_file->Get("sample_message", "bhv_broker", "");
        if (worker_idx >= 0)
           ini_config["worker_idx"] = std::to_string(worker_idx);
        vector<LearnerBase*> learners;
        int N = app_num;
        for (int k=0; k<N; k++) {
           map<string, string> param_conf;
           param_conf["alpha"] = config_file->Get("learner_"+std::to_string(k), "alpha", "");
           param_conf["beta"] = config_file->Get("learner_"+std::to_string(k), "beta", "");
           param_conf["l1"] = config_file->Get("learner_"+std::to_string(k), "l1", "");
           param_conf["l2"] = config_file->Get("learner_"+std::to_string(k), "l2", "");
           param_conf["model"] = config_file->Get("learner_"+std::to_string(k), "init_model", "");
           param_conf["setparam_flag"] = config_file->Get("learner_"+std::to_string(k), "setparam_flag", "");
           string name = config_file->Get("learner_"+std::to_string(k), "name", "");
           LearnerBase* _ll;
           if (name == "ig") {
               LearnerBase* _ll = new igLearner(k, param_conf);
               learners.push_back(_ll);
               INFO("begin ig model");
           }
           else if (name == "is"){
               LearnerBase* _ll = new isLearner(k, param_conf);
               learners.push_back(_ll);
               INFO("begin is model");
           }
           else if (name == "ir"){
               LearnerBase* _ll = new irLearner(k, param_conf);
               learners.push_back(_ll);
               INFO("begin ir model");
           }
           else if (name == "ivCPE"){
               LearnerBase* _ll = new ivCPELearner(k, param_conf);
               learners.push_back(_ll);
               INFO("begin ivCPE model");
           }
        }   

        SampleKafka sample_kafka(learners, ini_config);
        sample_kafka.Start();
        sample_kafka.Wait();
        for (int k=0; k<N; k++)
             learners[k]->Wait();
     }
    while(true) {};
    return 0;
 }
