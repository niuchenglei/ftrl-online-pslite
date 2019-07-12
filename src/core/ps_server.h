#ifndef PS_SERVER_H
#define PS_SERVER_H

#include <vector>
#include <unordered_map>
#include <ps/ps.h>
#include <boost/unordered_map.hpp>
#include "optimizer/ftrl.h"
#include <map>
using namespace std;
class PSS {
public:
    PSS(int appid, map<string, string>& server_conf);
    void StartFtrl();
    void SyncToRedis();
private:
    int app_id;
    int save_frequence;
    string save_path;
    Ftrl* ftrl;

    //void SyncToRedis();
};

#endif
