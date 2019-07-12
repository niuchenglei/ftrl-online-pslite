#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>

#include "sys/stat.h"
#include "pthread.h"

using namespace std;

#define REGIST_HANDLER(file)    \
EventFile::RegistHandler(file, UpdateAux, this);

#define REGIST_HANDLER_PTR(file, ptr)    \
EventFile::RegistHandler(file, UpdateAux, ptr);

#define UNREGIST_HANDLER(file)      \
EventFile::UnRegistHandler(file, this);

#define UNREGIST_HANDLER_PTR(file, ptr)    \
EventFile::UnRegistHandler(file, ptr);

typedef bool (*HandleFunc)(void *);

bool UpdateAux(void*);
class AutoUpdate {
public:
    virtual bool Load() = 0;
   // virtual const std::string& getTypeName() const = 0;
    //virtual const std::string& getRegistName() const = 0;
    //virtual const bool getRegistFlag() const = 0;
};

class EventHandler {
public:
    EventHandler(const std::string &file, HandleFunc handle, void* ptr);
    std::string file_;
    HandleFunc handle_;
    void* ptr_;
    time_t mTime_;
};

class EventFile {
public:
    static EventFile* instance();
    static void destroy();
    static void RegistHandler(const std::string &file, HandleFunc handle, void* ptr);
    static void UnRegistHandler(const std::string &file, void* ptr);

private:
    static EventFile* spInstance;
    static bool mDestroyed;
    std::vector<EventHandler> pool_;
    pthread_mutex_t lock;

    EventFile();
    ~EventFile();

    static void *startMonitor(void *);
};

#endif
