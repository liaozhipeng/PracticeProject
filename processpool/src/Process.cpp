#include<iostream>
#include"Process.h"
#include<sys/wait.h>
Process::Process(const ProcessFunc & func):func_(func),pid_(0),started_(false),waited_(false)
{
}

Process::Process():func_(NULL),pid_(0),started_(false),waited_(false)
{
}

void Process::start()
{
    assert(!started_);
    started_=true;
    assert(pid_=fork()>=0);
    assert(func_);
    if(pid_==0)
    {
        func_();
        exit(0);
    }
}

void Process::wait()
{
    assert(started_);
    assert(!waited_);
    if(waitpid(pid_,NULL,0)!=pid_)
    {
        perror("waitpid err");
        exit(1);
    }
    waited_=true;
}

void Process::setFunc(const ProcessFunc & func)
{
    func_=func;
    return ;
}
