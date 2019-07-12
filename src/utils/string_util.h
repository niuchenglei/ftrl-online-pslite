#ifndef STRING_UTIL_H
#define STRING_UTIL_H
#include <unistd.h>
#include <vector>
#include <set>
#include <map>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <string>
using namespace std;

static string trim(string s){
    int i = 0;
    if(s.length() == 0)
        return "";

    while (s[i] == ' ') {
        i++;
    }
    s = s.substr(i);
    i = s.size()-1;
    if(i == -1)
        return "";

    while(s[i] ==' ') {
        i--;
    }
    s = s.substr(0,i+1);
    return s;
}
static int count(const std::string& str, char delim) {
    int cnt = 0;
    size_t last = 0;
    size_t index = str.find_first_of(delim, last);

    while (index != std::string::npos){
        last = index + 1;
        cnt += 1;
        index = str.find_first_of(delim, last);
    }
    if (index-last>0)
        cnt += 1;

    return cnt;
}
static std::string last_state(const std::string& str) {
    int idx = strlen(str.c_str())-1;
    int ll = idx+1;
    while (idx >= 0) {
        if (str[idx] == '\t') break;
        idx--;
    }
    string s = str.substr(idx+1, ll-idx-1);
    return s;
}
static std::vector<std::string> split(const std::string& str, char delim) {
    size_t last = 0;
    size_t index = str.find_first_of(delim, last);

    int len;
    vector<string> ret;
    string tmp = "";
    while (index != std::string::npos){
        tmp = trim(str.substr(last, index-last));
        ret.push_back(tmp);

        last = index+1;
        index = str.find_first_of(delim, last);
    }
    if (index-last>0){
        tmp = trim(str.substr(last, index-last));
        ret.push_back(tmp);
    }

    return ret;
}
static std::string find_kv(std::string& str, std::string key) {
    size_t index0 = str.find(key);
    size_t start0 = str.find(":", index0);
    size_t end0 = str.find("|", start0);

    return str.substr(start0+1, end0-start0-1);
}
static bool isFloat(const std::string& str, float* f) {
    try {
        string str1(str);
        boost::trim(str1);
        *f = boost::lexical_cast<float>(str1);
    } catch(...) {
        //DEBUG(LOGNAME,"not float string: str=%s", str.c_str());
        return false;
                   }
                        return true;
 }
static bool isUInt64(const std::string& str, uint64_t* l) {
    try {
        string str1(str);
        boost::trim(str1);
        *l = boost::lexical_cast<uint64_t>(str1);
    } catch(...) {
        //DEBUG(LOGNAME,"not long string: str=%s", str.c_str());
          return false;
                   }
      return true;
  }
        //
static float sigmoid(float x) {
    return 1 / (1 + exp(-x));
}
#endif
