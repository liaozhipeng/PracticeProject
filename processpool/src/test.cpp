#include "Task.h"
#include "TaskQueue.h"
#include <functional>
#include <vector>
void gg(int a){
    std::cout<<a<<std::endl;
}

int main(){
    using tasktype = std::function<void()>;
    TaskQueue<tasktype> taskqueue;
    taskqueue.Init();
    std::vector<tasktype> b;
    tasktype temp ;
    while(taskqueue.getSize()>0) temp = taskqueue.pop();
    temp = std::bind(gg,1);
    temp();
    taskqueue.push(temp);
    
    auto bbb = taskqueue.pop();
    bbb();

    // std::vector<tasktype> q;
    // tasktype temp ;
    // temp = std::bind(gg,1);
    // temp();
    // q.push_back(temp);
    
    // tasktype bbb = tasktype(q[0]);
    // bbb();
    // bbb();
}