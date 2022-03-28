#ifndef __MYSERVER_THREAD_H__
#define __MYSERVER_THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>
#include <atomic>

namespace MyServer {

class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    //获取信号量,使数量减一
    void wait();

    //释放信号量，使数量加一
    void notify();

private:
    Semaphore(const Semaphore&) = delete;//关闭构造函数
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    sem_t m_semaphore;

};
//局部锁模板，传入锁类型即可
template<class T>
struct ScopedLockImpl {
public:

    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.lock();
        m_locked = true;
    }

    //析构函数,自动释放锁
    ~ScopedLockImpl() {
        unlock();
    }

    //加锁
    void lock() {
        if(!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    //解锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    // mutex
    T& m_mutex;
    // 是否已上锁
    bool m_locked;
};

//局部读模板，传入锁类型即可
template<class T>
struct ReadScopedLockImpl {
public:

    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.rdlock();
        m_locked = true;
    }

    ~ReadScopedLockImpl() {
        unlock();
    }
    //上读锁
    void lock() {
        if(!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    //释放锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    // mutex
    T& m_mutex;
    // 是否已上锁
    bool m_locked;
};

//局部写模板，传入锁类型即可
template<class T>
struct WriteScopedLockImpl {
public:

    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.wrlock();
        m_locked = true;
    }

    ~WriteScopedLockImpl() {
        unlock();
    }
    //上写锁
    void lock() {
        if(!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    //解锁
    void unlock() {
        if(m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    // Mutex
    T& m_mutex;
    // 是否已上锁
    bool m_locked;
};

class Mutex /*: Noncopyable*/ {
public: 
    // 局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    //加锁
    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    //解锁
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    // mutex
    pthread_mutex_t m_mutex;
};

class NullMutex {
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex();
    ~NullMutex();
    void lock();
    void unlock();
};

class RWMutex /*: Noncopyable*/{
public:
    // 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    // 局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    //上读锁
    void rdlock() {
        pthread_rwlock_rdlock(&m_lock);
    }
    //上写锁
    void wrlock() {
        pthread_rwlock_wrlock(&m_lock);
    }
    //解锁
    void unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    // 读写锁
    pthread_rwlock_t m_lock;
};

class NullRWMutex {
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex();
    ~NullRWMutex();
    void wrlock();
    void rdlock();
    void unlock();
};

class Spinlock /*: Noncopyable*/ {
public:
    // 局部锁
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }
    //上锁
    void lock() {
        pthread_spin_lock(&m_mutex);
    }
    //解锁
    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    // 自旋锁
    pthread_spinlock_t m_mutex;
};

class Thread {
public:


private:
    std::atomic_flag m_mutex;

};

class Thread {

public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }
    //实现pthread_jion()（等待线程结束，线程间同步的操作）的功能，并提供日志提示
    void join();

    // 获取当前运行线程，所以定义成静态变量
    static Thread* GetThis();
    // 获取当前运行线程的名称
    static const std::string& GetName();
    // 设置当前运行线程的名称
    static void SetName (const std::string& name);

private:
    Thread(const Thread&) = delete;//关闭构造函数
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;

};


}

#endif