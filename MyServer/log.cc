#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "config.h"
#include <set>

namespace MyServer {
	
const char* LogLevel::ToString(LogLevel::Level level) {
	switch(level) {
#define XX(name) \
	case LogLevel::name: \
		return #name; \
		break;
	
		XX(DEBUG);
		XX(INFO);
		XX(WARN);
		XX(ERROR);
		XX(FATAL);
#undef XX
	default:
		return "UNKNOW";
	}
	return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e)
	:m_event(e) {

}

LogEventWrap::~LogEventWrap() {
	m_event->getLogger()->log(m_event->getLevel(), m_event);
	//析构造的时候将生成一个日志器logger(所以需要将时间和等级赋予日志器)
}

void LogEvent::format(const char* fmt, ...) {
	va_list al;
	va_start(al,fmt);
	format(fmt, al);
	va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
	char* buf = nullptr;
	//将格式化数据从可变参数列表写入缓冲区
	//(logger, "test macro fmt error %s", "aa")得出的结果是test macro fmt error aa
	int len = vasprintf(&buf, fmt, al);
	if(len != -1) {
		m_ss << std::string(buf, len);
		free(buf);
	}

}

std::stringstream& LogEventWrap::getSS() {
	return m_event->getSS();
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if(m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
	MessageFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getContent();
		
	}
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
	LevelFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << LogLevel::ToString(level);
	
	}
};


class ElapseFormatItem : public LogFormatter::FormatItem {
public:
	ElapseFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getElapse();
	
	}
};


class NameFormatItem : public LogFormatter::FormatItem {
public:
	NameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getLogger()->getName();
	
	}
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
	ThreadIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getThreadId();
	
	}
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
	FiberIdFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getFiberId();
	
	}
};


class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
	DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%s"):m_format(format) {
	if(m_format.empty()){
		m_format = "%Y-%m-%d %H:%M:%s";
	}
	}
	void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);//获取系统时间
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);//将时间按m_format的格式传入
        os << buf;
    }
private:
	std::string m_format;
};


class FileNameFormatItem : public LogFormatter::FormatItem {
public:
	FileNameFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getFile();
	
	}
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
	LineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << event->getLine();
	
	}
};


class NewLineFormatItem : public LogFormatter::FormatItem {
public:
	NewLineFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << std::endl;
	
	}
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
	TabFormatItem(const std::string& str = "") {}
	void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
		os << "\t";
	
	}
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time)
: m_logger(logger), m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time){

}

Logger::Logger(const std::string& name ):m_name(name) ,m_level(LogLevel::DEBUG) {
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

	// if(name == "root") {
	// 	m_appenders.push_back(StdoutAppender::ptr(new StdoutAppender));
	// }
}

void Logger::setFormatter(LogFormatter::ptr val) {
    MutexType::Lock lock(m_mutex);
    m_formatter = val;

	//给默认的赋值
    for(auto& i : m_appenders) {
		//防止操作的时候另一边输出日志
        MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val) {
    //std::cout << "---" << val << std::endl;
    MyServer::LogFormatter::ptr new_val(new MyServer::LogFormatter(val));
    if(new_val->isError()) {
        std::cout << "Logger setFormatter name=" << m_name
                  << " value=" << val << " invalid formatter"
                  << std::endl;
        return;
    }
    m_formatter = new_val;
    setFormatter(new_val);
}
//以yaml为载体输出日志信息
std::string Logger::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::ptr Logger::getFormatter() {
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender) {
    MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()) {
       MutexType::Lock ll(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
	for(auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
		if (*it == appender) {
			m_appenders.erase(it);
			break;
		}
	}
}

void Logger::clearAppenders() {
    m_appenders.clear();
}

//等级大于默认等级，在日志输出地集合遍历，同时将日志器本身返回
void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
	if(level >= m_level) {
		auto self = shared_from_this();
		MutexType::Lock lock(m_mutex);
		if(!m_appenders.empty()) {
			for(auto& it : m_appenders) {
				it->log(self, level, event);
			}
		} else if(m_root) {
			//递归自身
			m_root->log(level, event);
		}
		
	}		
}


void Logger::debug(LogEvent::ptr event) {
	log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
	log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
	log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
	log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
	log(LogLevel::FATAL, event);
}


FileLogAppender::FileLogAppender(const std::string& filename)
	:m_filename(filename) {

}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
	if (level >= m_level) {
		MutexType::Lock lock(m_mutex);
		//文件日志输出器传入文件格式
		m_filestream << m_formatter->format(logger, level, event);
	}
}

bool FileLogAppender::reopen() {
	if (m_filestream) {
		m_filestream.close();
	}
	m_filestream.open(m_filename);
	return !!m_filestream;
}

void StdoutAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
    if(level >= m_level) {
        std::cout << m_formatter->format( logger, level, event);
    }
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
	//判断是继承的还是自己的
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

std::string StdoutAppender::toYamlString() {
	MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}
//解析格式
LogFormatter::LogFormatter(const std::string& pattern)
    :m_pattern(pattern) {
		init();
}

//遍历模式器里的开始输出
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event){
	std::stringstream ss;
	for(auto& i : m_items) {
		i->format(ss, logger, level, event);
	}
	return ss.str();

}

//"%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
//格式类型 %xxx %xxx{xxx} %%
//解析格式
void LogFormatter::init() {
	//str,format,type
	std::vector<std::tuple<std::string, std::string, int > > vec;
	size_t last_pos = 0;
	std::string nstr;
	//这里执行的操作是在输入的pattern里面，找到%xxx{xxx}
	for(size_t i = 0; i < m_pattern.size(); ++i) {
		if (m_pattern[i] != '%'){
			nstr.append(1, m_pattern[i]);
			continue;
		}
		if((i + 1) < m_pattern.size()) {
			if(m_pattern[i+1] == '%') {
				nstr.append(1, '%');
				continue;
			}
		}

		size_t n = i + 1;
		int fmt_status = 0;
		size_t fmt_begin = 0;

		std::string fmt;
		std::string str;
		while(n < m_pattern.size()){
			if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')) {
				break;
			}

			if(m_pattern[n] == '{' && fmt_status == 0) {
				str = m_pattern.substr(i + 1, n - i - 1);
				fmt_begin = n;
				fmt_status = 1;
				n++;
				continue;
			}
			if(m_pattern[n] == '}' && fmt_status == 1) {
				fmt_status = 2;
				fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin -1); 
				break;
			}
			n++;
		}
		if(fmt_status == 0) {
			if(!nstr.empty()) {
				vec.push_back(std::make_tuple(nstr, std::string(), 0));
				nstr.clear();
			}
			str = m_pattern.substr(i + 1, n - i -1);
			vec.push_back(std::make_tuple(str, fmt, 1));//这里的状态是0,说明没有遇到'{'，这里的fmt为空，需要往后看
			i = n - 1;//在while里面i之后的已经遍历
		} else if(fmt_status == 1) {
			std::cout << "pattern parse error:" << "-" << m_pattern.substr(i) << std::endl;
			vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
		} else if(fmt_status == 2) {	
			if(!nstr.empty()) {
				vec.push_back(std::make_tuple(nstr, "", 0));
				nstr.clear();
			}
			vec.push_back(std::make_tuple(str, fmt, 1));
			i = n;
		}
	}

	if(!nstr.empty()){
		vec.push_back(std::make_tuple(nstr, "", 0));
	}//对于普通的string的快速提取出来
	static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> >s_format_items = {
#define XX(str, C) \
	{#str, [](const std::string& fmt) {return FormatItem::ptr(new C(fmt));}}

	XX(m, MessageFormatItem),
	XX(p, LevelFormatItem),
	XX(r, ElapseFormatItem),
	XX(c, NameFormatItem),
	XX(t, ThreadIdFormatItem),
	XX(n, NewLineFormatItem), 
	XX(d, DateTimeFormatItem),
	XX(f, FileNameFormatItem),
	XX(l, LineFormatItem),
	XX(T, TabFormatItem),
	XX(F, FiberIdFormatItem),
#undef XX
	};

	for(auto& i : vec) {
		if(std::get<2>(i) == 0) {
			m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
		} else {
			auto it = s_format_items.find(std::get<0>(i));
			if(it == s_format_items.end()) {
				m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
				//formatter形式错误
				m_error = true;
			} else {
				m_items.push_back(it->second(std::get<1>(i)));
			}
		}
		//std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
	}
	//std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
	//新建一个Logger,接受新的对象
	m_root.reset(new Logger);
	//给新的Logger创建日志输出地
	m_root->addAppender(LogAppender::ptr(new StdoutAppender));
	//将新建的logger放入m_loggers里面
	m_loggers[m_root->m_name] = m_root;

	//初始化
	init();
}


Logger::ptr LoggerManager::getLogger(const std::string& name) {
	MutexType::Lock lock(m_mutex);
	auto it = m_loggers.find(name);
	//查找之前的logger,没有的话返回新的
	if(it != m_loggers.end()) {
		return it->second;
	}
	Logger::ptr logger(new Logger(name));//新创建的没有appender
	//将LoggerManager构造函数创建的Logger赋值给新创建的logger
	logger->m_root = m_root;
	//新创建的logger添加到LoggerManager的m_loggers里面
	m_loggers[name] = logger;
	return logger;
}

//日志输出地定义
struct LogAppenderDefine {
	int type = 0; // 1 File, 2 Stdout
	LogLevel:: Level level = LogLevel::UNKNOW;
	//具体的格式
	std::string formatter;
	//具体文件
	std::string file;

	bool operator==(const LogAppenderDefine& oth) const {
		return type == oth.type
			&& level == oth.level
			&& formatter == oth.formatter
			&& file == oth.file;	
	}


};

//日志定义
struct LogDefine {
	std::string name;
	LogLevel:: Level level = LogLevel::UNKNOW;
	std::string formatter;
//可能有多个输出地
	std::vector<LogAppenderDefine> appenders;

	bool operator==(const LogDefine& oth) const {
		return name == oth.name
			&& level == oth.level
			&& formatter == oth.formatter
			&& appenders == oth.appenders;
	}
	// 红黑树的查找find是按照重载的<来判断
	bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }
};

//LexicalCast类型转化偏特化，set的已经定义，这里只需要定义LogDefine即可
template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
		//XML或者其他的都有一个判断属性是否存在，yaml直接n["name"].IsDefined()
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
		//以std::string 类型输出
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if(n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }
		//遍历yaml里面的appenders字符串数组，构建LogAppenderDefine
        if(n["appenders"].IsDefined()) {
            //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for(size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutAppender") {
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKNOW) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if(!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }

        for(auto& a : i.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutAppender";
            }
            if(a.level != LogLevel::UNKNOW) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if(!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

//全局变量定义在main函数之前
MyServer::ConfigVar<std::set<LogDefine> >::ptr g_log_defines = 
	MyServer::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

//全局变量定义在main函数之前
//这里的格式定义了yaml
struct LogIniter {
    LogIniter() {
        g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine>& old_value,
                    const std::set<LogDefine>& new_value){
            MYSERVER_LOG_INFO(MYSERVER_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                MyServer::Logger::ptr logger;
				//如果新的日志名称在原来的里面没有
                if(it == old_value.end()) {
                    //新增logger
                    logger = MYSERVER_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = MYSERVER_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                //std::cout << "** " << i.name << " level=" << i.level
                //<< "  " << logger << std::endl;
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    MyServer::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        // if(!MyServer::EnvMgr::GetInstance()->has("d")) {
                            ap.reset(new StdoutAppender);
                        // } else {
                        //     continue;
                        // }
                    }
                    ap->setLevel(a.level);
					//解析formatter
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }
            }

            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = MYSERVER_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;

std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LoggerManager::init() {

}


}
