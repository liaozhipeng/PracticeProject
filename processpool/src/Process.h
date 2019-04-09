#ifndef PROCESS_H
#define PROCESS_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<functional>
#include<assert.h>

using namespace std;

class Process
{
public:
    typedef function<void*()> ProcessFunc;
    Process(const ProcessFunc & func);
    Process();
    ~Process();
    void start();
    void wait();
    void setFunc(const ProcessFunc & func);

private:
    pid_t pid_;
    bool started_;
    bool waited_;
    ProcessFunc func_;
};


#endif
