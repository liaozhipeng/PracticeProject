#include "MyThread.h"

void MyThread::start()
{
    assert(!_bstarted);
    if(pthread_create(&_id,NULL,thread_func,this))
    {
        printf("pthread_create error\n");
        abort();
    };
    _bstarted = true;
}

void MyThread::join()
{
    assert(_bstarted);
    assert(!_bjoined);
    assert(!_bdetached);
    _bjoined = true;
    pthread_join(_id,0);
}

void MyThread::detach()
{
    assert(_bstarted);
    assert(!_bdetached);
    assert(!_bjoined);
    pthread_detach(_id);
    _bdetached = true;
}
