#ifndef REDIS_H
#define REDIS_H

#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/unordered_map.hpp>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include "hiredis/hiredis.h"
#include "hiredis/async.h"

using namespace std;
//#define REDIS_DEBUG

#ifdef REDIS_DEBUG
static std::ofstream redis_out("/data0/vad/ms/log/redis.log", std::ios_base::app);
#endif

class Redis {
public:
    Redis() : port_(0), db_(0), isConnected_(false), rc_(NULL) { }
    ~Redis() { if (rc_) redisFree(rc_); }
    Redis(const Redis &orig) {
        host_ = orig.host_;
        port_ = orig.port_;
        db_ = orig.db_;
        isConnected_ = false;
        rc_ = NULL;
    }
    bool init(const std::string &host, int port, int db = 0, int timeout = 30) {
        host_ = host;
        port_ = port;
        db_ = db;
        isConnected_ = false;
        rc_ = NULL;
        return connect(timeout);
    }
    bool connect(int milliseconds = 3) {
        if (isConnected_) return true;
        if (rc_) redisFree(rc_);
        rc_ = NULL;
        struct timeval tv = {milliseconds / 1000, milliseconds * 1000};
        rc_ = redisConnectWithTimeout(host_.c_str(), port_, tv);
        if (rc_ == NULL) return false;
        if (rc_->err) {
            error_ = rc_->errstr;
            redisFree(rc_);
            rc_ = NULL;
            return false;
        }
#ifdef REDIS_DEBUG
redis_out << "connect " << host_ << ":" << port_ << " success" << std::endl;
#endif
        //if (!select(db_)) return false;
        isConnected_ = true;
        return true;
    }
    bool get(const std::string &key, std::string &value) {
        if (!connect()) return false;
        redisReply *reply = (redisReply *)redisCommand(rc_, "GET %s", key.c_str());
        if (reply == NULL) {
            isConnected_ = false;
            return false;
        } else if (reply->type == REDIS_REPLY_STRING) {
            char *str = reply->str;
            if (str == NULL)
                value = "";
            else
                value = str;
            freeReplyObject(reply);
            reply = NULL;
            return true;
        } else if (reply->type == REDIS_REPLY_NIL) {
            value = "";
            freeReplyObject(reply);
            reply = NULL;
            return true;
        } else {
            isConnected_ = false;
            freeReplyObject(reply);
            reply = NULL;
            return false;
        }
    }
    bool mget(boost::unordered_map<std::string, std::string> &kvs) {
        error_ = "";
        if (kvs.empty()) return true;
        if (!connect()) {
            if (rc_->err)
                error_ = rc_->errstr;
            return false;
        }
        std::ostringstream os;
        os << "MGET ";
        boost::unordered_map<std::string, std::string>::iterator it = kvs.begin();
        for ( ; it != kvs.end(); ++it)
            os << it->first << " ";
        redisReply *reply = (redisReply *)redisCommand(rc_, os.str().c_str());
        if (reply == NULL) {
            isConnected_ = false;
            return false;
        } else if (reply->type == REDIS_REPLY_ARRAY && reply->elements == kvs.size()) {
            it = kvs.begin();
            for (size_t i = 0; i < kvs.size(); ++i, ++it) {
                if (reply->element[i]->type == REDIS_REPLY_ERROR) {
                    error_ = string(reply->element[i]->str);
                }
                if (reply->element[i]->type != REDIS_REPLY_STRING) {
                    it->second = "";
                    continue;
                }
                char *str = reply->element[i]->str;
                if (str == NULL)
                    it->second = "";
                else
                    it->second = str;
            }
            freeReplyObject(reply);
            reply = NULL;
            return true;
        } else {
            if (REDIS_REPLY_ERROR == reply->type)
                error_ = "REDIS_REPLY_ERROR, detail:" + string(reply->str);
            isConnected_ = false;
            freeReplyObject(reply);
            reply = NULL;
            return false;
        }
    }
    bool select(int db) {
        redisReply *reply = (redisReply *)redisCommand(rc_, "select %d", db);
        if (reply == NULL) {
            return false;
        } else {
            freeReplyObject(reply);
            reply = NULL;
            return true;
        }
    }
    bool set(const std::string &key, const std::string &value) {
        if (!connect()) return false;
        redisReply *reply = (redisReply *)redisCommand(rc_, "SETEX %s 172800 %s", key.c_str(), value.c_str());
        if (reply == NULL) {
            isConnected_ = false;
            return false;
        }
        return true;
    }
    bool setex(const std::string &key, const std::string &value, int32_t expire) {
        if (!connect()) return false;
        redisReply *reply = (redisReply *)redisCommand(rc_, "SETEX %s %d %s", key.c_str(), expire, value.c_str());
        if (reply == NULL) {
            isConnected_ = false;
            return false;
        }
        return true;
    }
    bool mdel(const std::vector<std::string>& keys) {
        if (!connect()) return false;
        for (int k=0; k<keys.size(); k++) {
            if (REDIS_OK != redisAppendCommand(rc_, "DEL %s", keys[k].c_str()))
                return false;
            //printf("[%s%d]", keys[k].c_str(), timeout);
            //redisAppendCommand(rc_, "get %s", keys[k].c_str());
        }
        unsigned int total = keys.size();
        redisReply *reply = NULL;
        //while (total-- > 0) {
        for (int k=0; k<keys.size(); k++) {
            int r = redisGetReply(rc_, (void **)&reply);
            if (r == REDIS_ERR) {
                return false;
            }
            if (reply->type == REDIS_REPLY_ERROR) {
                error_ = "REDIS_REPLY_ERROR, detail:" + string(reply->str);
                return false;
            }else if (!reply) {
                isConnected_ = false;
                return false;
            }
            //printf("%s=%s\n", keys[k].c_str(), reply->str);
            freeReplyObject(reply);
            //printf("ok");
        }
        //pthread_t tid=pthread_self();
        //printf("--%d:%d\n", tid,total);
        return true;
    }
    bool set_expire(const std::string& key, unsigned int timeout) {
        if (!connect()) return false;
        redisReply *reply = (redisReply *)redisCommand(rc_, "expire %s %d", key.c_str(), timeout);
        if (reply == NULL) {
            isConnected_ = false;
            return false;
        }
        return true; 
    }
    bool mset_expire(const std::vector<std::string>& keys, unsigned int timeout) {
        if (!connect()) return false;
        for (int k=0; k<keys.size(); k++) {
            if (REDIS_OK != redisAppendCommand(rc_, "expire %s %d", keys[k].c_str(), timeout))
                return false;
            //printf("[%s%d]", keys[k].c_str(), timeout);
            //redisAppendCommand(rc_, "get %s", keys[k].c_str());
        }
        unsigned int total = keys.size();
        redisReply *reply = NULL;
        //while (total-- > 0) {
        for (int k=0; k<keys.size(); k++) {
            int r = redisGetReply(rc_, (void **)&reply);
            if (r == REDIS_ERR) { 
                return false;
            }
            if (reply->type == REDIS_REPLY_ERROR) { 
                error_ = "REDIS_REPLY_ERROR, detail:" + string(reply->str);
                return false;
            }else if (!reply) {
                isConnected_ = false;
                return false;
            }
            //printf("%s=%s\n", keys[k].c_str(), reply->str);
            freeReplyObject(reply);
            //printf("ok");
        }
        //pthread_t tid=pthread_self();
        //printf("--%d:%d\n", tid,total);
        return true;
    }

    const std::string &ErrorStr() const { return error_; }
private:
    std::string host_;
    int port_;
    int db_;
    bool isConnected_;
    redisContext *rc_;
    std::string error_;
};

#endif

