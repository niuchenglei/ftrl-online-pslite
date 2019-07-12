#ifndef LOGGER_H
#define LOGGER_H

#include<log4cpp/Category.hh>
#include<iostream>

//extern "C" int __get_worker_idx();
//日志优先级
enum Priority {
    ERROR,
    WARN,
    INFO,
    DEBUG
};

//用单例模式封装log4cpp
class Logger {
 public: 
    static Logger& getInstance();
    static void destory();
    void setPriority(Priority priority);
    void error(const char* msg);
    void warn(const char* msg);
    void info(const char* msg);
    void debug(const char* msg);

 private:
    Logger();  //单例模式：构造函数私有化

 private:
    static Logger *plog_;
    log4cpp::Category &category_ref_;
};

//*****************************************************
//注意：
//文件名 __FILE__ ,函数名 __func__ ，行号__LINE__ 是编译器实现的
//并非C++头文件中定义的 
//前两个变量是string类型，且__LINE__是整形，所以需要转为string类型
//******************************************************

//整数类型文件行号 ->转换为string类型
inline std::string int2string(int line) {
    std::ostringstream oss;
    oss << line;
    return oss.str();
}


//定义一个在日志后添加 文件名 函数名 行号 的宏定义
#define suffix(msg)  std::string(msg).append(" ##")\
        .append(__FILE__).append(":").append(__func__)\
        .append(":").append(int2string(__LINE__))\
        .append("##").c_str()


//不用每次使用时写 getInstance语句
//只需要在主函数文件中写: #define _LOG4CPP_即可在整个程序中使用
#ifdef _LOG4CPP_
Logger &__logger__ = Logger::getInstance();
#else
extern Logger &__logger__;
#endif

//缩短并简化函数调用形式
#define ERROR(msg) Logger::getInstance().error(suffix(msg))
#define FATAL(msg) Logger::getInstance().warn(suffix(msg))
#define INFO(msg) Logger::getInstance().info(suffix(msg))
#define DEBUG(msg) Logger::getInstance().debug(suffix(msg))

#endif

