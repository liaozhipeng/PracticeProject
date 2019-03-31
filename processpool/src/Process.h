#ifndef PROCESS_H
#define PROCESS_H

#include<unistd.h>
#include<sys/types.h>

class Process
{
private:
    pid_t _id;
    void start();
public:
    Process();
    ~Process();
};

Process::Process()
{
    _id = fork();
    if(_id == 0){
        //子进程运行
        start();
    }
    else if(_id >= 0)
    {
        //什么也不做，返回父进程
    }
    
}

Process::~Process()
{
}

void Process::start()
{
    while(1){

    }
}


#endif