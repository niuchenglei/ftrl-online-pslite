#include<iostream>
#include "logger.h"
#include<log4cpp/PatternLayout.hh>
#include<log4cpp/OstreamAppender.hh>
#include<log4cpp/FileAppender.hh>
#include<log4cpp/Priority.hh>

using namespace std;

extern "C" int __get_worker_idx();

Logger* Logger::plog_ = NULL;

//获取log指针
Logger& Logger::getInstance() {
    if ( plog_ == NULL ) {
        plog_ = new Logger;
    }
    return *plog_;
}


//销毁
void Logger::destory() {
    if (plog_) {
        plog_->category_ref_.info("Logger destroy");
        plog_->category_ref_.shutdown();
        delete plog_;
    }
}


//构造函数
Logger::Logger(): 
    category_ref_(log4cpp::Category::getRoot()) {
    //自定义输出格式
    log4cpp::PatternLayout *pattern_one =
        new log4cpp::PatternLayout;
    pattern_one->setConversionPattern("%d: %p %c %x:%m%n");

    log4cpp::PatternLayout *pattern_two =
        new log4cpp::PatternLayout;
    pattern_two->setConversionPattern("%d: %p %c %x:%m%n");

    //获取屏幕输出
    log4cpp::OstreamAppender *os_appender = 
        new log4cpp::OstreamAppender("osAppender",&std::cout);
    os_appender->setLayout(pattern_one);

    //获取文件日志输出 （ 日志文件名:mylog.txt )
    int worker_idx = __get_worker_idx();
    string vv = "logs/log_"+std::to_string(worker_idx)+".txt";
    log4cpp::FileAppender *file_appender = 
        new log4cpp::FileAppender("fileAppender", vv);
    file_appender->setLayout(pattern_two);

    category_ref_.setPriority(log4cpp::Priority::DEBUG);
    //category_ref_.addAppender(os_appender);
    category_ref_.addAppender(file_appender);

    category_ref_.info("Logger created!");
}


//设置优先级
void Logger::setPriority(Priority priority) {
    switch (priority) {
        case (ERROR):
            category_ref_.setPriority(log4cpp::Priority::ERROR);
            break;

        case (WARN):
            category_ref_.setPriority(log4cpp::Priority::WARN);
            break;

        case (INFO):
            category_ref_.setPriority(log4cpp::Priority::INFO);
            break;

        case (DEBUG):
            category_ref_.setPriority(log4cpp::Priority::DEBUG);
            break;

        default:
            category_ref_.setPriority(log4cpp::Priority::DEBUG);
            break;    
    }
}


void Logger::error(const char* msg) {
    category_ref_.error(msg);
}

void Logger::warn(const char* msg) {
    category_ref_.warn(msg);
}

void Logger::info(const char* msg) {
    category_ref_.info(msg);
}

void Logger::debug(const char* msg) {
    category_ref_.debug(msg);
}
