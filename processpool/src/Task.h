#ifndef TASK_H
#define TASK_H

#include "TaskQueue.h"
#include <functional>
//不断从一个工作队列取出任务, 并处理任务
template<typename T>
class Task
{
public:
    Task(const TaskQueue<T> &taskQueue):taskQueue_(taskQueue),stop_(false){};
    void start();
    void stop();
private:
    using TaskType = std::function<void()>;
    TaskQueue<T> taskQueue_;
    bool stop_;
};

template<typename T>
void Task<T>::start(){
    while(!stop_){
        if(taskQueue_.getSize()>0){
            T task = taskQueue_.pop();
            task();
        }
        else{
            std::cout<<"wait task...\n";
        }
    }
}

template<typename T>
void Task<T>::stop(){
    stop_ = true;
}

#endif