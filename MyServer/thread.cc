#include "thread.h"
#include "log.h"

namespace MyServer {

//声明一个线程期间的全局静态变量
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static MyServer::Logger::ptr g_logger = MYSERVER_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count) {
    if(sem_init(&m_semaphore, 0, count)) {
        throw std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() {
    sem_destroy(&m_semaphore);
}

void Semaphore::wait() {
    if(sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if(sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

Thread* Thread::GetThis() {
    return t_thread;
}
const std::string& Thread::GetName() {
    return t_thread_name;
}
void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    :m_cb(cb)
    ,m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOW";
    }
    //线程创建，第二个参数可以设置分离属性,但直接设置的时候仍有可能存在内存泄漏
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if(rt) {
        MYSERVER_LOG_ERROR(g_logger) << "pthread_create thread fail, rt=" << rt
            << " name=" << name;
        throw std::logic_error("pthread_create error");
    }
    //顺序问题，等线程构造好了才返回构造函数
    m_semaphore.wait();
}
Thread::~Thread() {
    if(m_thread) {
        pthread_detach(m_thread);
    }
}

void Thread::join() {
    if(m_thread) {
        //等待线程结束，线程间同步的操作
        int rt = pthread_join(m_thread, nullptr);
        if(rt) {
            MYSERVER_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
                << " name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

//静态成员，this指针必须显示的传入
void* Thread::run(void* arg) {
    Thread* thread = (Thread*)arg;
    //将创建的线程赋给线程局部变量
    t_thread = thread;
    t_thread_name = thread->m_name;
    //将系统的内核Id传入
    thread->m_id = MyServer::GetThreadId();
    //给线程命名
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    //当里面存在智能指针时，防止其不会被释放掉,用swap释放掉
    cb.swap(thread->m_cb);

    
    thread->m_semaphore.notify();
    //执行函数
    cb();
    return 0;
}


}