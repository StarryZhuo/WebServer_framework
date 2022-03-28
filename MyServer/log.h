#ifndef __SERVER_FRAMEWORK_LOG_H
#define __SERVER_FRAMEWORK_LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <fstream>
#include <sstream>//<strstream>
#include <vector>
#include <list>
#include <map>
#include "util.h"
#include "singleton.h"
#include "thread.h"

//
#define MYSERVER_LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
		MyServer::LogEventWrap(MyServer::LogEvent::ptr(new MyServer::LogEvent(logger, level, __FILE__, __LINE__, 0, MyServer::GetThreadId(), \
						MyServer::GetFiberId(), time(0)))).getSS()

#define MYSERVER_LOG_DEBUG(logger) MYSERVER_LOG_LEVEL(logger, MyServer::LogLevel::DEBUG)
#define MYSERVER_LOG_INFO(logger) MYSERVER_LOG_LEVEL(logger, MyServer::LogLevel::INFO)
#define MYSERVER_LOG_WARN(logger) MYSERVER_LOG_LEVEL(logger, MyServer::LogLevel::WARN)
#define MYSERVER_LOG_ERROR(logger) MYSERVER_LOG_LEVEL(logger, MyServer::LogLevel::ERROR)
#define MYSERVER_LOG_FATAL(logger) MYSERVER_LOG_LEVEL(logger, MyServer::LogLevel::FATAL)

#define MYSERVER_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		MyServer::LogEventWrap(MyServer::LogEvent::ptr(new MyServer::LogEvent(logger, level, \
							__FILE__, __LINE__, 0,  MyServer::GetThreadId(), \
							MyServer::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)//可变参数的宏，替代...

#define MYSERVER_LOG_FMT_DEBUG(logger, fmt, ...) MYSERVER_LOG_FMT_LEVEL(logger, MyServer::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define MYSERVER_LOG_FMT_INFO(logger, fmt, ...) MYSERVER_LOG_FMT_LEVEL(logger, MyServer::LogLevel::INFO, fmt, __VA_ARGS__)
#define MYSERVER_LOG_FMT_WARN(logger, fmt, ...) MYSERVER_LOG_FMT_LEVEL(logger, MyServer::LogLevel::WARN, fmt, __VA_ARGS__)
#define MYSERVER_LOG_FMT_ERROR(logger, fmt, ...) MYSERVER_LOG_FMT_LEVEL(logger, MyServer::LogLevel::ERROR, fmt, __VA_ARGS__)
#define MYSERVER_LOG_FMT_FATAL(logger, fmt, ...) MYSERVER_LOG_FMT_LEVEL(logger, MyServer::LogLevel::FATAL, fmt, __VA_ARGS__)

//通过LoggerMgr得到一个logger
#define MYSERVER_LOG_ROOT() MyServer::LoggerMgr::GetInstance()->getRoot()

//得到一个日志器
#define MYSERVER_LOG_NAME(name) MyServer::LoggerMgr::GetInstance()->getLogger(name)

namespace MyServer {


class Logger;
class LoggerManager;

//日志级别
class LogLevel {
public:	
	enum  Level{
		UNKNOW = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5
	};

	static const char* ToString(LogLevel::Level level);
	//将字符串变成相应的Level
	static LogLevel::Level FromString(const std::string& str);
};

//日志事件
class LogEvent {
public:
	typedef std::shared_ptr<LogEvent> ptr; 
	LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t m_line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);

	const char* getFile() const { return m_file;}
	int32_t getLine() const { return m_line;}

	uint32_t getElapse() const { return m_elapse;}

   	uint32_t getThreadId() const { return m_threadId;}

   	uint32_t getFiberId() const { return m_fiberId;}

   	uint64_t getTime() const { return m_time;}

    std::string getContent() const { return m_ss.str();}

	std::shared_ptr<Logger> getLogger() const { return m_logger; }

	LogLevel::Level getLevel() { return m_level; }

	//返回日志内容字符串流
	std::stringstream& getSS() {return m_ss;}

	//这里va_list用来解决变参问题
	//格式化写入日志内容
	void format(const char* fmt, ...);
	//格式化写入日志内容
	void format(const char* fmt, va_list al);
private:
	const char* m_file = nullptr;  //文件名
	int32_t m_line = 0;             //行号
	uint32_t m_elapse = 0;          //程序启动开始到现在的毫秒数，因为内存没对齐
	uint32_t m_threadId = 0;        //线程id
	uint32_t m_fiberId = 0;         //协程id
	uint64_t m_time = 0;            //时间戳
	std::stringstream m_ss;

	std::shared_ptr<Logger> m_logger;
	LogLevel::Level m_level;
};

//日志事件包装器，析构后将事件传入日志器
class LogEventWrap {
public:
	LogEventWrap(LogEvent::ptr e);
	~LogEventWrap();
	LogEvent::ptr getEvent() { return m_event; }
	std::stringstream& getSS();
private:
	LogEvent::ptr m_event;
};


//日志格式器
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	LogFormatter(const std::string& pattern);

	//%t   %threadId %m %n
	//逐一输出格式项目解析parttern后的
	std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
public:
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		FormatItem(const std::string& fmt = "") {}
		virtual ~FormatItem() {}
		virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	};
	//pattern 的解析
	void init();
	//日志类型错误的话直接标记出来
	bool isError() const { return m_error; }
	const std::string getPattern() const { return m_pattern; }

private:
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;

	bool m_error = false;

};

//日志输出地
class LogAppender{
friend class Logger;
public:
	typedef std::shared_ptr<LogAppender> ptr;
	typedef Spinlock MutexType;
	virtual ~LogAppender() {}
//这里使用的是std::shared_ptr<Logger> logger而不是Logger::ptr为了确定LogAppender里面的logger被调用的次数
	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level Level, LogEvent::ptr event) = 0;
	
	virtual std::string toYamlString() = 0;//与mutex有关的都需要加锁toYamlString()，setFormatter(),getFormatter()

	void setFormatter(LogFormatter::ptr val);
	
	LogFormatter::ptr getFormatter();

	LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level val) { m_level = val; }
protected:
	LogLevel::Level m_level = LogLevel::DEBUG;
	bool m_hasFormatter = false;
	MutexType m_mutex;
	LogFormatter::ptr m_formatter;
};


//日志器	
class Logger : public std::enable_shared_from_this<Logger>{ //类的成员寒暑使用智能指针

//需要使用LoggerManager中的m_root 复制给成员变量m_root
friend class LoggerManager;
public:
	typedef std::shared_ptr<Logger> ptr;
	typedef Spinlock MutexType;

	Logger(const std::string& name = "root");
	//生成日志器
	void log(LogLevel::Level level, LogEvent::ptr event);

	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);

	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);

	void clearAppenders();

	LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level val) { m_level = val; }

	const std::string& getName() const {return m_name;}
	//设置日志格式
	void setFormatter(const std::string& val);
	void setFormatter(LogFormatter::ptr val);

	LogFormatter::ptr getFormatter();

	//将logger信息输出到Yaml上
	std::string toYamlString();

private:
	std::string m_name;                        //日志名称
	LogLevel::Level m_level;                   //日志集合
	std::list<LogAppender::ptr> m_appenders;   //Appender集合
	//没有fmt时备用的fmt
	LogFormatter::ptr m_formatter;

	Logger::ptr m_root;//找到已有的

	MutexType m_mutex;
};

//输出到控制台的Appender
class StdoutAppender : public LogAppender {
public:
	typedef std::shared_ptr<StdoutAppender> ptr;
	void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
	std::string toYamlString() override;
};

//定义输出到文件的Appender
class FileLogAppender : public LogAppender {
public:
	typedef std::shared_ptr<FileLogAppender> ptr;
	FileLogAppender(const std::string& filename);
	void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
	//重新打开文件，文件打开成功返回true
	bool reopen();
	std::string toYamlString() override;

private:
	std::string m_filename;
	std::ofstream m_filestream;
};

//管理所有的logger,需要就调用
class LoggerManager {
public:
	typedef Spinlock MutexType;
	LoggerManager();
	Logger::ptr getLogger(const std::string& name);

	void init();
	Logger::ptr getRoot() const { return m_root; }

	std::string toYamlString();
private:
	MutexType m_mutex;
	std::map<std::string, Logger::ptr> m_loggers;
	Logger::ptr m_root;
};

typedef MyServer::Singleton<LoggerManager> LoggerMgr;

}


#endif

