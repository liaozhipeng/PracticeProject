#include "TaskQueue.h"
#include "Task.h"
#include <functional>
int main(){
    TaskQueue<std::function<void()>> taskqueue;
    Task<std::function<void()>> task(taskqueue);

    task.start();

}