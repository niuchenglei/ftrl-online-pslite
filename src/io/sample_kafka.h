#ifndef SAMPLE_KAFKA_H
#define SAMPLE_KAFKA_H

#include "message_kafka.h"
#include "parser.h"
//void CALLBACK_SHELL()
class SampleKafka {
public:
    SampleKafka(std::vector<LearnerBase*>& learners, map<string,string> &ini_config); 
    ~SampleKafka();

    void Start(); 

    void Stop();
    bool Wait();

private:
    MessageKafka *message_imp, *message_bhv;
    Parser* parser;
    std::vector<LearnerBase*> learners;
    int workid;
    int stat; //0 init ;1 read ;2 ;3  exit; 4 ;finish
};



#endif
