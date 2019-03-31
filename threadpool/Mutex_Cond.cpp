#include "Mutex_Cond.h"

Mutex_Cond::Mutex_Cond():_destroyed(false){
    pthread_mutex_init(&_mutex,NULL);
    pthread_cond_init(&_cond,NULL);
};
Mutex_Cond::~Mutex_Cond(){
    if(!_destroyed){
        _destroyed = true;
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }
};

int Mutex_Cond::lock(){
    return pthread_mutex_lock(&_mutex);
};

int Mutex_Cond::unlock(){
    return pthread_mutex_unlock(&_mutex);
};

int Mutex_Cond::wait(){
    return pthread_cond_wait(&_cond,&_mutex);
};

int Mutex_Cond::timewait(const struct timespec* abstime){
    return pthread_cond_timedwait(&_cond,&_mutex,abstime);
};

int Mutex_Cond::signal(){
    return pthread_cond_signal(&_cond);
};

int Mutex_Cond::broadcast(){
    return pthread_cond_broadcast(&_cond);
};

int Mutex_Cond::destroy(){
    if(!_destroyed){
        _destroyed = true;
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
    }
};