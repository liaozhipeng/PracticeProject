/*************************************************************************
    > File Name: Worker.h
    > Author: LiaoZhipeng
    > Mail: 13249156840@163.com 
    > Created Time: 2019年03月20日 星期二 11时12分37秒
 ************************************************************************/
#ifndef WORKER_H
#define WORKER_H
#include "MyThread.h"
#include "WorkQueue.h"


class Worker
{
public:
    typedef std::function<void*()> taskFunc;
    Worker(int queueSize);//初始化队列容量
    ~Worker();
    void addWork(const taskFunc& task);//添加任务
    void join();
private:
    void* startWork();
    MyThread _worker;
    WorkQueue<taskFunc> _wq;
    bool _bworking;
};

#endif