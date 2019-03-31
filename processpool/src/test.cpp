#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <thread>
#include <iostream>
using namespace std;
__thread int i=9;
void f()
{
    i++;
    cout<<i;
};

int main()
{
    i = 100;
    thread t1(f);
    thread t2(f);

    t1.join();
    t2.join();
    cout<<i;

    return 0;
    
}