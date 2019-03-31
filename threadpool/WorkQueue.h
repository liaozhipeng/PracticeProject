/*************************************************************************
    > File Name: WorkQueue.h
    > Author: LiaoZhipeng
    > Mail: 13249156840@163.com 
    > Created Time: 2019年03月19日 星期二 22时19分36秒
 ************************************************************************/
//这个已经是线程安全的工作队列了，使用的时候就不用考虑加锁之类的
#ifndef WORKQUEUE_H
#define WORKQUEUE_H

#include<unistd.h>
#include"Queue.h"
#include"Mutex_Cond.h"
#include<sys/syscall.h>
#define gettid() syscall(SYS_gettid) //获取真实线程id防止不同进程的线程ID相同

template<typename T>
class WorkQueue
{
public:
    WorkQueue(int maxsize = 10);
    WorkQueue(const WorkQueue<T>& rhs);
    WorkQueue<T>& operator=(const WorkQueue<T>& rhs);
    ~WorkQueue();
    bool empty() const;
    bool isFull() const;
    int size() const;
    void push(const T& data);
    void pop();
    T& front();
    T& back();

private:
    int _capacity;
    Queue<T> _queue;
    Mutex_Cond _mc;
};

template<typename T>
WorkQueue<T>::WorkQueue(int maxsize):_capacity(maxsize),_queue(maxsize)
{

};

template<typename T>
WorkQueue<T>::WorkQueue(const WorkQueue<T>& rhs):_capacity(rhs._capacity), _queue(rhs._queue)
{

};

template<typename T>
WorkQueue<T>& WorkQueue<T>::operator=(const WorkQueue<T>& rhs)
{
    if(this!=&rhs){
        _capacity=rhs._capacity;
        _queue = rhs._queue;
    }
    return *this;
}

template<typename T>
WorkQueue<T>::~WorkQueue()
{
    
}

template<typename T>
bool WorkQueue<T>::empty() const
{
    return _queue.empty();  
}

template<typename T>
bool WorkQueue<T>::isFull() const
{
    return _queue.isFull();
}

template<typename T>
int WorkQueue<T>::size() const
{
    return _queue.size();
}

template<typename T>
void WorkQueue<T>::push(const T& data)
{
    _mc.lock();//锁，保护队列添加对象，线程安全
    cout<<"producer "<<gettid()<<" begin push..."<<endl;
    _queue.push(data);
    _mc.signal();//假如有pop操作在等待，就可以唤醒那个pop
    cout<<"prodecer "<<gettid()<<" notified consumer by condition variable..."<<endl;
    _mc.unlock();
}

template<typename T>
void WorkQueue<T>::pop()
{
    _mc.lock();
    //用while循环来防止惊群效应，如果是误触发，那么该操作会再次进入等待，只有任务队列为空才会进入等待
    //C++11可以使用conditional_variable的两参数wait函数来实现下面这个while循环
    while(_queue.empty())
    {
        cout<<"consumer "<<gettid()<<" begin wait a condition..."<<endl;
        _mc.wait();//如果任务队列空了，那么就挂起线程，等待新任务加入然后被唤醒
    }
    cout<<"consumer "<<gettid()<<" end wait a condition..."<<endl;
    cout<<"consumer "<<gettid()<<" begin pop..."<<endl;
    _queue.pop();
    _mc.unlock();
}

template<typename T>
T& WorkQueue<T>::front()
{
    _mc.lock();
    while(_queue.empty())
    {
        cout<<gettid()<<" wait a work..."<<endl;
        _mc.wait();
    }
    _mc.unlock();
    //这里有点问题，能用C++11 unique_lock控制返回后锁再释放会好一些
    return _queue.front();
}

template<typename T>
T& WorkQueue<T>::back()
{
    _mc.lock();
    while(_queue.empty())
    {
	cout<<gettid()<<" wait a work..."<<endl;
        _mc.wait();
    }
    _mc.unlock();
    return _queue.back();
}

#endif