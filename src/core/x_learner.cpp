#include "x_learner.h"
#include "utils/string_util.h"
using namespace std;

int igLearner::CheckLabel(std::tr1::unordered_map<string,string> &condition) {
    vector<string> table_code = {"14000014","22000001","22000002","25000001","25000002","27000001","27000002","27000003","27000005","27000006","29000001","29000002","29000003","29000005","50000005","60000001","60000003","80000006","80000010","80000011","80000030","80000060","80000070","80000089","50000009","50000010","60000010"};
    string code = condition["bhv_code"], filter = condition["filter"];
    string play_duration = condition["play_duration"], isautoplay = condition["isautoplay"], open_app = condition["open_app"];
    if (std::find(table_code.begin(), table_code.end(), code) == table_code.end()) {
        return -1; }
    if (filter != "0" && filter != "10000") {
       return -1;
    }
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
        return 1;
    } else {
        return -1;
    }
    return -1;
}

int isLearner::CheckLabel(std::tr1::unordered_map<string,string> &condition) {
    vector<string> table_code ={"14000003","14000005","14000008","14000045","14000098","21000001","21000002","21000003","21000004","80000003"};
    string code = condition["bhv_code"];
    string filter = condition["filter"];
    if (std::find(table_code.begin(), table_code.end(), code) == table_code.end()) {
        return -1; }
    if (filter != "0" && filter != "10000") {
       return -1;
    }
    return 1;
}
int irLearner::CheckLabel(std::tr1::unordered_map<string,string> &condition) {
    vector<string> table_code ={"27000004","29000004","50000001","50000002","50000004","50000006"}; 
    string code = condition["bhv_code"];
    string filter = condition["filter"];
    if (std::find(table_code.begin(), table_code.end(), code) == table_code.end()) {
        return -1; }
    if (filter != "0" && filter != "10000") {
       return -1;
    }
    return 1;
}

int ivCPELearner::CheckImp(std::tr1::unordered_map<string, string> &condition) {
    if (condition.find("bid_type") != condition.end()) {
        if (condition["bid_type"] == "8"){
            return 0; }
    }
    return -1;
}

int ivCPELearner::CheckLabel(std::tr1::unordered_map<string,string> &condition) {
    if (condition.find("bid_type") != condition.end()) {
        if (condition["bid_type"] != "8") return -1;
    } else
        return -1;

    string code = condition["bhv_code"];
    string open_app = condition["open_app"];
    string filter = condition["filter"];
    uint64_t openapp = 3;
    if (filter != "0")
        return -1;    
    if (open_app.size() > 0)
        isUInt64(open_app, &openapp);
    if (code !="28000001" && code != "80000020") {
        if (openapp != 1 && openapp != 2) {
            return 1;
        }
    }
   
    return -1;
}
