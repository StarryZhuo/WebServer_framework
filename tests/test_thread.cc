#include "../MyServer/MyServer.h"
#include <unistd.h>

MyServer::Logger::ptr g_logger = MYSERVER_LOG_ROOT();

int count = 0;
// MyServer::RWMutex s_mutex;
MyServer::Mutex s_mutex;

void fun1() {
    MYSERVER_LOG_INFO(g_logger) << "name: " << MyServer::Thread::GetName()
                                << " this.name: " << MyServer::Thread::GetThis()->getName()
                                << " id: " << MyServer::GetThreadId()
                                << " this.id: " << MyServer::Thread::GetThis()->getId();

    for(int i = 0; i < 100000; ++i) {
        // MyServer::RWMutex::WriteLock lock(s_mutex);
        MyServer::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2() {
    while(true) {
        MYSERVER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3() {
    while(true) {
        MYSERVER_LOG_INFO(g_logger) << "========================================";
    }
}

int main(int argc, char** argv) {
    MYSERVER_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/starry/My_Server_Framework/bin/conf/test2.yml");
    MyServer::Config::LoadFromYaml(root);

    std::vector<MyServer::Thread::ptr> thrs;
    for(int i = 0; i < 2; ++i) {
        MyServer::Thread::ptr thr(new MyServer::Thread(&fun1, "name_" + std::to_string(i * 2)));
        MyServer::Thread::ptr thr2(new MyServer::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    MYSERVER_LOG_INFO(g_logger) << "thread test end";
    MYSERVER_LOG_INFO(g_logger) << "count=" << count;

    return 0;
}
