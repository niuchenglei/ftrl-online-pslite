#include <iostream>
#include "rdkafka.h"
#include "message_kafka.h"
#include <string>
#include <sys/time.h> 
#include <fstream>
#include "parser.h"
using namespace std;
static int param_id = 0;
extern "C" int __exit_flag__;     

int MessageKafka::Init(const char *brokers, const char *group, const char *topic,std::vector<int>& parts, bool sasl, int worker_idx){
    rd_kafka_conf_t *conf, *conf2;
    rd_kafka_topic_conf_t *topic_conf, *topic_conf2;
    rd_kafka_resp_err_t err;
    char tmp[16];
    char errstr[512];
    rd_kafka_t *rk;

    for (int i=0; i<parts.size(); i++) {
        partitions.push_back(parts[i]);
    }
    /* Kafka configuration */
    conf = rd_kafka_conf_new();
    param = 0;
    rd_kafka_conf_set(conf, "group.id", group, errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "api.version.request", "true", errstr, sizeof(errstr));
    if(sasl) {
        param = 1;
        rd_kafka_conf_set(conf, "security.protocol", "sasl_plaintext", errstr, sizeof(errstr));
        rd_kafka_conf_set(conf, "sasl.mechanisms", "PLAIN", errstr, sizeof(errstr));
        rd_kafka_conf_set(conf, "sasl.username", "ads", errstr, sizeof(errstr));
        rd_kafka_conf_set(conf, "sasl.password", "73f4b1a6dd828199bd9f1964dc1b669d", errstr, sizeof(errstr));
        //rd_kafka_conf_set(conf, "queued.min.messages", "20", NULL, 0);
     }
    param_id = param;
    rd_kafka_conf_set(conf, "session.timeout.ms", "60000", NULL, 0);
    //rd_kafka_conf_set(conf, "auto.commit.interval.ms", "3000", NULL,0); 
    rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!rk)
        ERROR("Failed to create rdkafka instance ");
 
    topic_conf = rd_kafka_topic_conf_new();
    rd_kafka_topic_conf_set(topic_conf,"auto.offset.reset","latest", errstr, sizeof(errstr));
        
    if (rd_kafka_brokers_add(rk, brokers) == 0){
        INFO( "%% No valid brokers specified");
        return -1;
       }
    rkts[0] = rd_kafka_topic_new(rk,topic, topic_conf);

    rkq = rd_kafka_queue_new(rk); 

    this->worker_idx = worker_idx;
}

void MessageKafka::Process(CALLBACK_FUNC callback, void* ptr) {
    int len = partitions.size();
    time_t start,end;
    start = time(NULL);
    end = time(NULL);

    for (int i=0; i<partitions.size(); i++) {
        int re = rd_kafka_consume_start_queue(rkts[0], partitions[i], RD_KAFKA_OFFSET_END, rkq); 
        if (re == -1)
            DEBUG("Failed to start queue consuming");
    }

    int count = 0;
    int timeval = 100;
    int msgsize = 2000;
    if ( param == 1) {
         timeval = 100; msgsize = 2000; 
    }

    while (true) { 
        start = time(NULL);
        end = time(NULL);
        count = 0;
        std::vector<long> lastset(len, 0);
        std::vector<long> newset(len, LONG_MAX);
        if (__exit_flag__ == 1) break;
        while ( difftime(end,start) < 60*60) {
            if (__exit_flag__ == 1) break;
            ssize_t r;
            rd_kafka_message_t *rkmessage[msgsize];
            r = rd_kafka_consume_batch_queue(rkq, timeval, rkmessage, msgsize);
            //printf("consumer %d\n", r);

            if (r == 0) { continue;}
            if (r == -1) { 
                DEBUG("Failed to consume messages"); continue;
            }
            else {
                for (int k=0;k<r;k++) {
                 //   gettimeofday(&t1, NULL);
                    for (int j=0; j<partitions.size(); j++) {
                        if (rkmessage[k]->partition == partitions[j]  && rkmessage[k]->offset < newset[j]) {
                            newset[j] = rkmessage[k]->offset;  break; }
                        else if (rkmessage[k]->partition == partitions[j] &&
                              rkmessage[k]->offset > lastset[j]) {
                            lastset[j]=rkmessage[k]->offset; break; }
                     }
                    callback((const char*)rkmessage[k]->payload, (int)rkmessage[k]->len, param, ptr);
                    rd_kafka_message_destroy(rkmessage[k]);
                 }

            }
            end=time(NULL);
        }

        for (int j=0; j<partitions.size(); j++) {
            rd_kafka_consume_stop(rkts[0], partitions[j]);
            int re = rd_kafka_consume_start_queue(rkts[0], partitions[j], RD_KAFKA_OFFSET_END, rkq);
            if (re == -1)
                DEBUG("Failed to startt consume messages");
        }
        ssize_t r=0;
        std::vector <long> trueset(len, LONG_MAX);
        rd_kafka_message_t *rkmessage[200];
        while (r<1) {
            r = rd_kafka_consume_batch_queue(rkq, 100, rkmessage, 200); 
        }

        string topicstr; 
        for (int k=0;k<r;k++) {
            for (int j=0; j<partitions.size(); j++) {
                if (rkmessage[k]->partition == partitions[j] && rkmessage[k]->offset < trueset[j]){ 
                    trueset[j] = rkmessage[k]->offset; topicstr = rd_kafka_topic_name(rkmessage[k]->rkt);
                     break; 
                 }
            }// 
            callback((const char*)rkmessage[k]->payload, (int)rkmessage[k]->len, param, ptr);
            rd_kafka_message_destroy(rkmessage[k]);
        }//  if (r>0)
        long sum = 0;
        time_t tt=time(NULL);
        tm* tr =localtime(&tt);
        string record_time = std::to_string(tr->tm_hour)+"_"+std::to_string(tr->tm_min);
        string filename = "kafka_offset_"+std::to_string(worker_idx) + "_" + std::to_string(param)+".txt";
        ofstream out (filename,ios::app);
        
        for (int i=0;i<len;i++) {
            if (trueset[i] == LONG_MAX) continue;
            float res = (lastset[i]-newset[i])*1.0/(trueset[i]-newset[i]);
            sum += lastset[i]-newset[i];
            out<<record_time<<" "<<topicstr<<" partiton "<<partitions[i]<<"last:"<< lastset[i] << "true:" << trueset[i] << "res:" << res<<"\n";

        }
        float percount=sum/(count+0.00001);
        out<<"sum:"<<sum<<"avg:"<<percount<<"\n";
        out.close();
      }
      rd_kafka_topic_destroy(rkts[0]);

  }


void MessageKafka::Stop(){
    run = 0;
}
 
