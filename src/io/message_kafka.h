#ifndef MESSAGE_KAFKA_H
#define MESSAGE_KAFKA_H
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <error.h>
#include <getopt.h>
#include <string>
#include <boost/unordered_map.hpp>
#include <rdkafka.h>

typedef int (*CALLBACK_FUNC)(const char*, int64_t len, int param, void* ptr);

class MessageKafka {
public:
    MessageKafka() { run = 1;}//part_id=part; }
    ~MessageKafka() {}

    int Init(const char *brokers, const char *group, const char *topic, std::vector<int>& parts, bool sasl=false, int worker_idx = -1);
    void Stop();
    void Process(CALLBACK_FUNC callback = NULL, void* ptr = NULL);

private:
    int run, param, worker_idx;
    std::vector<int> partitions;
    rd_kafka_t *rk,*rk2;
    rd_kafka_topic_partition_list_t *topics,*topics2;
    std::string topic;
    rd_kafka_queue_t *rkq;
    rd_kafka_topic_t *rkts[2];
};

#endif
