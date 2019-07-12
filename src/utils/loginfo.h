#ifndef RECORDLOG_H
#define RECORDLOG_H
#include <string>
#include "logger.h"

using namespace std;

typedef struct RecordLog{
    float stat[13];
    int cnt[13], pre_min = -1;
    int neghour[24], poshour[24];
    char buf[1024];

    void StatDelay1(int64_t time1, int64_t time2) {
        stat[1] += time2 - time1;
        cnt[11]++;//未经处理的互动日志数
        time_t tTime = (time_t) time1;
        tm *ttm = localtime(&tTime);
        int h = int(ttm->tm_hour);
        poshour[h]++;
     }
    RecordLog() {
        for (int i=0; i<13; i++) {
            stat[i] = 0; cnt[i] = 0; }
        pre_min = -1;
        for (int i=0; i<24; i++) {
            neghour[i] = 0; poshour[i] = 0;}
     }
    void StatDelay0(int64_t time1, int64_t time2 ) {
        stat[0] += time2 - time1;
        cnt[10]++;
        time_t tTime = (time_t) time1;
        tm *ttm = localtime(&tTime);
        int h = int(ttm->tm_hour);
        neghour[h]++;
    }
    void StatSampleNum(int label) {
        if (label == 0) cnt[0]++;
        if (label == 1) cnt[1]++;
    }
    //void statis3() { cnt[3]++; }
    //void statis2() { cnt[2]++; }
    //void statis0() { cnt[0]++; }
    //void statis1() { cnt[1]++; }
    void StatQPS(float usetime) {
        stat[8] += usetime;}
    void StatInfo(float ll1, float ll2, float p1, float p2, float vari, float vari_online) {
        stat[2] += ll1; stat[3] += ll2; stat[4] += p1; stat[5] += p2; cnt[4]++; stat[6] += vari; stat[7] += vari_online; } //printf("lp\n"); }
    void Printbyapp(int app_id) {

        time_t t;
        t = time(NULL);
        tm *curtime = localtime(&t);
        int hour = curtime->tm_hour, min = curtime->tm_min, sec = curtime->tm_sec;
        if ( min%5 == 0 && pre_min != min) {
            pre_min = min;
            //int total = cnt[0]+cnt[1];
            float QPS = stat[8];
            //float QPS = total*1000.0/(stat[8]+0.00001);
            //printf(" QPS\n");
            sprintf(buf, "Tn:%d Tp:%d ctr:%.5f ll_tflr:%.5f ll_ftrl:%.5f ptflr:%.5f pftrl:%.5f vari:%.5f vari_onlie:%.5f QPS(s/sec):%.0f app:%d", cnt[0], cnt[1], (cnt[1]/(cnt[0]+0.01)), stat[2]/(cnt[4]+0.0001), stat[3]/(cnt[4]+0.0001), stat[4]/(cnt[4]+0.0001), stat[5]/(cnt[4]+0.0001), stat[6]/(cnt[4]+0.0001), stat[7]/(cnt[4]+0.0001), QPS,app_id);
            string msg = string(buf);
            INFO(msg);
            for (int k=0; k<13; k++) {
                stat[k] = 0; cnt[k] = 0;
            }
            //pre_min = min;
        }
    }
    void Print() {
        time_t t;
        t = time(NULL);
        tm *curtime = localtime(&t);
        int hour = curtime->tm_hour, min = curtime->tm_min, sec = curtime->tm_sec;
        if ( min%5 == 0 && pre_min != min) {
            pre_min = min;
            sprintf(buf, "delay0: %.0f delay:%.0f", stat[0]/(cnt[10]+0.0001), stat[1]/(cnt[11]+0.0001));
            string msg = string(buf);
            INFO(msg);
            for (int k=0; k<13; k++) {
                stat[k] = 0; cnt[k] = 0;
            }
        }
    }
} ;
#endif
