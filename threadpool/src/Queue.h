/*************************************************************************
    > File Name: Queue.h
    > Author: LiaoZhipeng
    > Mail: 13249156840@163.com 
    > Created Time: 2019年03月19日 星期二 15时41分43秒
 ************************************************************************/
#ifndef QUEUE_H
#define QUEUE_H

#include<assert.h>
#include<iostream>

using namespace std;

template<typename T>
class Queue
{
public:
    Queue(int maxsize = 10);
    Queue(const Queue<T>& rhs);
    Queue<T> & operator=(const Queue<T>& rhs);
    ~Queue();

    bool empty() const;
    bool isFull() const;
    int size() const;
    void push(const T& data);
    void pop();
    T& front();
    T& front() const;
    T& back();
    T& back() const;
private:
    T* _array;
    int _front;
    int _rear;
    int _capacity;
};

template<typename T>
Queue<T>::Queue(int maxsize):_front(0),_rear(0),_capacity(maxsize)
{
    _array = new T[_capacity];
    assert(_array);
}

template<typename T>
Queue<T>::Queue(const Queue<T>& rhs):_front(rhs._front),_rear(rhs._rear),_capacity(rhs._capacity)
{
    _array = new T[_capacity];
    for(int i=0;i<_capacity;++i)
        _array[i] = rhs._array[i];
}

template<typename T>
Queue<T>& Queue<T>::operator=(const Queue<T>& rhs)
{
    if(this != &rhs){
        _front = rhs._front;
        _rear = rhs._rear;
        _capacity = rhs._capacity;
        delete[] _array;
        _array = new T[_capacity];
        for(int i=0;i<_capacity;++i)
            _array[i] = rhs._array[i];
    }
    return *this;
}

template<typename T>
Queue<T>::~Queue()
{
    delete[] _array;
}

template<typename T>
bool Queue<T>::empty() const
{
    return _front == _rear;
}

template<typename T>
bool Queue<T>::isFull() const
{
    return (_rear+1)%_capacity == _front;
}

template<typename T>
int Queue<T>::size() const
{
    return (_rear - _front + _capacity) % _capacity;
}

template<typename T>
void Queue<T>::push(const T& data)
{
    if(!isFull())
    {
        _array[_rear] = data;
        _rear = (_rear+1)%_capacity;
    }
    else{
        T* newarray = new T[2*_capacity];
        for(int i=0;i<2*_capacity && !this->empty();++i)
        {
            newarray[i] = this->front();
            this->pop();
        }
        delete[] _array;
        _array = newarray;
        _front = 0;//更新扩容后_front的位置
        _rear = _capacity-1;//更新扩容后_rear的位置
        _array[_rear] = data;
        ++_rear; 
        _capacity *= 2;
    }
}

template<typename T>
void Queue<T>::pop()
{
    if(empty())
    {
        cerr<<"empty queue!\n";
        return ;
    }
    _front = (_front+1)%_capacity;
}

template<typename T>
T& Queue<T>::front()
{
    if(empty())
    {
        cerr<<"Error,queue is empty!\n";
        abort();
    }
    return _array[_front];
}

template<typename T>
T& Queue<T>::front() const
{
    if(empty())        
    {
        cerr<<"Error,queue is empty!\n";
        abort();
    }
    return _array[_front];
}

template<typename T>
T& Queue<T>::back()
{
    if(empty())
    {
        cerr<<"Error,queue is empty!\n";
        abort();
    }
    return _array[_rear-1];
}

template<typename T>
T& Queue<T>::back() const
{
    if(empty())
    {
        cerr<<"Error,queue is empty!\n";
        abort();
    }
    return _array[_rear-1];
}

#endif