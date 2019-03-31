#include "Worker.h"

Worker::Worker(int queueSize):_wq(queueSize),_worker(std::bind(&Worker::startWork,this))
{
    _bworking = true;
    _worker.start();
}

Worker::~Worker()
{
    if(_bworking)
    {
        _bworking = false;
        _worker.join();
    }
}

void Worker::addWork(const taskFunc& task)
{
    _wq.push(task);
}

void* Worker::startWork()
{
    while(_bworking || !_wq.empty()){//保证任务队列为空之后再退出worker
        taskFunc task = _wq.front();
        task();
        _wq.pop();
    }
    return nullptr;
}

void Worker::join()
{
    _bworking = false;
    _worker.join();
}

