#include "TaskQueue.h"
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>

// #define PATHNAME "."
// #define PROJ_ID 0x660 

int main()
{
    pid_t pid;
    // key_t key = ftok(PATHNAME,PROJ_ID);
    // char* a;
    // int id = shmget(key,1024,IPC_CREAT|0666);
    // if(id<0) id = shmget(key,0,IPC_CREAT);
    // a = (char*) shmat(id,0,0);
    // shmdt(a);
    TaskQueue<int> Q;
    int ret = Q.Init();
    if(ret<0) return -1;
    int i = 0;
    for(int j=0;j<20;j++)
    {
        usleep(200000);
        i = Q.pop();
        std::cout<<"son:"<<i<<" "<<Q.getSize()<<std::endl;
    }
    return 0;
}