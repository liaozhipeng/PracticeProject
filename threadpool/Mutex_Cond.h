#ifndef MUTEX_COND_H
#define MUTEX_COND_H

#include<pthread.h>
class Mutex_Cond
{
public:
    Mutex_Cond();
    ~Mutex_Cond();

    int lock();
    int unlock();
    int wait();
    int timewait(const struct timespec* abstime);
    int signal();
    int broadcast();
    int destroy();
private:
    bool _destroyed;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
};

#endif