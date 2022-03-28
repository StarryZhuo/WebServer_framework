#include <iostream>
#include <thread>
#include "../MyServer/log.h"
#include "../MyServer/util.h"
#include "../MyServer/singleton.h"

int main(int argc, char** argv) {
	MyServer::Logger::ptr logger(new MyServer::Logger);
	logger->addAppender(MyServer::LogAppender::ptr(new MyServer::StdoutAppender));

	MyServer::FileLogAppender::ptr file_appender(new MyServer::FileLogAppender("./log.txt"));
	logger->addAppender(file_appender);

	MyServer::LogFormatter::ptr fmt(new MyServer::LogFormatter("%d%T%p%T%m%n"));
	file_appender->setFormatter(fmt);//这里其实已经替换了formatter但是由于m_ss里面的输出

	file_appender->setLevel(MyServer::LogLevel::ERROR);
	
	//thread id只能通过stream来获取，不能强转
	//MyServer::LogEvent::ptr event(new MyServer::LogEvent(__FILE__, __LINE__, 0, (int32_t)std::this_thread::get_id(), 2, time(0)));
	//MyServer::LogEvent::ptr event(new MyServer::LogEvent(__FILE__, __LINE__, 0, MyServer::GetThreadId(), MyServer::GetFiberId(), time(0)));
	//event->getSS() << "hello summer";

	//logger->log(MyServer::LogLevel::DEBUG, event);
	std::cout << "hello MyServer log" << std::endl;

	MYSERVER_LOG_INFO(logger) << "test macro";
	MYSERVER_LOG_ERROR(logger) << "test macro error";

	MYSERVER_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

	auto l = MyServer::LoggerMgr::GetInstance()->getLogger("xx");
	MYSERVER_LOG_INFO(l) << "xxx";
	return 0;
}
