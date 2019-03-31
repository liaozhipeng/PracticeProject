#ifndef MY_THREAD_H
#define MY_THREAD_H
#include<pthread.h>
#include<functional>
#include<assert.h>
#include<memory>
#include<stdlib.h>
#include<unistd.h>
#include<functional>
class MyThread
{
public:
    typedef std::function<void()> ThreadFunc;
    template<typename func,typename... Args>
    //explicit MyThread(func&& threadFun,Args&&... args)
    MyThread(func&& threadFun,Args&&... args)
    :_bstarted(false),_bdetached(false),_bjoined(false)
    {
        _func = std::bind(std::forward<func>(threadFun),std::forward<Args>(args)...);
    };
    MyThread() = default;
    ~MyThread(){if(_bstarted&&!_bjoined&&!_bdetached) pthread_detach(_id);};
    void start();
    void join();
    void detach();
private:
    pthread_t _id;
    bool _bdetached;
    bool _bstarted;
    bool _bjoined;
    ThreadFunc _func;
    static void* thread_func(void* arg)
    {
        MyThread* t = (MyThread*)arg;
        t->_func();
        return nullptr;   
    }
    MyThread(const MyThread&);
    MyThread& operator=(const MyThread&);
};

#endif