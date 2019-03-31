#ifndef MY_PROCESS_H
#define MY_PROCESS_H

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<signal.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<pthread.h>

class process
{
public:
    process():_m_pid(-1){};

    pid_t _m_pid;
    int _m_pipefd[2];
};

template<typename T>
class processpool
{
private:
    processpool(int listenfd,int process_number = 8);
public:
    //单例类 懒汉模式 保证最多只创建一个进程池实例
    static processpool<T>* creat(int listenfd,int process_number = 8){
        if(!_m_instance){
            _m_instance = new processpool<T>(listenfd,process_number);
        }
        return _m_instance;
    }
    ~processpool(){
        delete[] _m_instance;
    };
    void run();
private:
    void setup_sig_pipe();
    void run_parent();
    void rrun_child();
    //进程允许的最大子进程数量
    static const int MAX_PROCESS_NUMBER = 16;
    //每个子进程最多能处理的客户数量
    static const int USER_PER_PROCESS = 65536;
    //epoll最多能处理的事件数
    static const int MAX_EVENT_NUMBER = 10000;
    //进程池中的进程总数,进程池初始化的时候确定
    int _m_process_number;
    //子进程在池中的序号，从0开始
    int _m_idx;
    //每个进程都有一个epoll内核事件表，用m_epoolfd标识
    int _m_epollfd;
    //监听socket
    int _m_listenfd;
    //子进程通过m_stop来决定是否停止运行
    int _m_stop;
    //保存所有子进程的描述信息
    process* _m_sub_process;
    //进程池静态实例
    static processpool<T>* _m_instance;
};

template<typename T>
processpool<T>* processpool<T>::_m_instance = NULL;
//这个是子进程自己用来通信的，用于信号的处理函数
static int sig_pipefd[2];
//设置非阻塞
static int setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

static void addfd(int epollfd,int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

static void removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

static void sig_handler(int sig)
{
    int save_errno = errno;//为什么要设置这个errno
    int msg = sig;
    send(sig_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
}

//添加信号处理函数 用sigaction方法
static void addsig(int sig,void(handler)(int),bool restart = true)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if(restart)
    {
        sa.sa_flags |=SA_RESTART;
    }
    //吧所有信号加入到sa_mask，相当于屏蔽所有信号
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!=-1);
}

//进程池构造函数。
//参数listenfd是监听socket，它必须在创建进程池之前被创建，否则
//子进程无法直接引用它，参数process_number指定进程池中子进程的数量。
//这个构造函数主要用来实现创建子进程组 在里面创建全双工的socket对用于进程间通信
//通过设置_m_idx判断是不是子进程
template<typename T>
processpool<T>::processpool(int listenfd,int process_number)
:_m_listenfd(listenfd),_m_process_number(process_number),_m_idx(-1),_m_stop(false)
{
    assert((process_number>0)&&(process_number<=MAX_PROCESS_NUMBER));
    //子进程组
    _m_sub_process = new process[process_number];
    assert(_m_sub_process);
    for(int i=0;i<process_number;++i){
        int ret = socketpair(PF_UNIX,SOCK_STREAM,0,_m_sub_process[i]._m_pipefd);
        assert(ret == 0);
        _m_sub_process[i]._m_pid = fork();
        assert(_m_sub_process[i]._m_pid >=0);
        if(_m_sub_process[i]._m_pid >0)//父进程运行这个
        {
            close(_m_sub_process[i]._m_pipefd[1]);//父进程用_m_pipefd[0]
            continue;
        }
        else//子进程运行这个
        {
            close(_m_sub_process[i]._m_pipefd[0]);//子进程用_m_pipefd[1]
            _m_idx = i;
            break;
        }
    }
};
//统一事件源
//设置信号管道
template<typename T>
void processpool<T>::setup_sig_pipe()
{
    _m_epollfd = epoll_create(5);
    assert(_m_epollfd != -1);
    int ret = socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
    assert(ret != -1);
    setnonblocking(sig_pipefd[1]);
    //这个加入到epoll中，如果有信号来自进程，就会触发sig_handler，
    //然后他会往sig_pipefd[1]发信息，sig_pipefd[0]就会接收到可读信息
    addfd(_m_epollfd,sig_pipefd[0]);//统一使用sig_pipefd[0]放入epoll进行操作
    addsig( SIGCHLD, sig_handler );
    addsig( SIGTERM, sig_handler );
    addsig( SIGINT, sig_handler );
    addsig( SIGPIPE, SIG_IGN );
}
//父进程中m_idx值为-1，子进程中m_idx值大于等于0，我们据此判断下来
//要运行的是父进程代码还是子进程代码
template<typename T>
void processpool<T>::run()
{
    if(_m_idx != -1)
    {
        rrun_child();
        return;
    }
    run_parent();
};
//子进程的run
// template<typename T>
// void processpool<T>::run_child()
// 1.实现setup_sig_pipe 尽力通信管道设置
// 2.获取和父进程通信的管道pipefd = _m_sub_process[_m_idx]._m_pipefd[1]
// 3.运行主循环
//     a.epoll_wait() 提取队列中的事件，并循环处理提取出来的事件的fd
//     b.如果是pipefd表示是来自父进程的消息，可读，调用accept函数接收客户端请求，并把
//         请求加入到子进程的epoll对象中，以后这个客户端就由该子进程管理
//     c.如果是sig_pipefd[0]的可读消息 那么接收内容并解析 如果是SIGCHLD表示有子进程挂了，用
//         waitpid清理，知道没有挂掉的进程了，如果是SIGTERM或SIGINT表示终止进程了，主循环标志
//         弄成停止
//     d.如果是别的epoll事件响应，表示是客户端发来的信息，处理
// 4.退出进程时的一些处理，关闭fd文件描述符 回收资源
template<typename T>
void processpool<T>::rrun_child()
{
    setup_sig_pipe();
    //每个子进程都通过其在进程池中的序号值m_idx找到与父进程通信的管道
    int pipefd = _m_sub_process[_m_idx]._m_pipefd[1];
    //子进程需要监听管道文件描述pipefd，因为父进程将通过它来通知子进程
    //accept新连接
    addfd(_m_epollfd,pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T* users = new T[USER_PER_PROCESS];
    assert(users);
    int number = 0;
    int ret = -1;
    while(!_m_stop)
    {
        //传入events事件的缓冲区开始地址,然后重写这块区域
        number = epoll_wait(_m_epollfd,events,MAX_EVENT_NUMBER,-1);
        if ( ( number < 0 ) && ( errno != EINTR ) )
        {
            printf( "epoll failure\n" );
            break;
        }
        for(int i=0;i<number;i++)
        {
            int sockfd = events[i].data.fd;
            if((sockfd == pipefd)&&(events[i].events & EPOLLIN))//这里要用&与，因为时间是置标志位的
            {
                int client = 0;
                //从父/子进程之间的管道读取数据，并将结果保存在变量client中。
                //如果读取成功，则表示有新的客户连接到来。
                ret = recv(sockfd,(char* )&client,sizeof(client),0);
                if( ( ( ret < 0 ) && ( errno != EAGAIN ) ) || ret == 0 )
                {
                    continue;
                }
                else
                {
                    struct sockaddr_in client_address;
                    socklen_t client_addrlength = sizeof(client_address);
                    int connfd = accept(_m_listenfd,(struct sockaddr*)&client_address,&client_addrlength);
                    if(connfd < 0)
                    {
                        printf("errno is:%d\n",errno);
                        continue;
                    //把连接加入到epoll中
                    addfd(_m_epollfd,connect);
                    //模板T必须实现init方法，以初始化一个客户连接
                    //我们直接使用connfd来索引逻辑处理对象
                    //T类型的对象，以提高程序效率
                    users[connfd].init(_m_epollfd, connfd, client_address);
                }
            }
            else if((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                //每接收到一个信号都会写入到缓冲区，所以handler函数虽然只写一个字符但是最后读的端口信息可能是多次写入的
                ret = recv(sig_pipefd[0],signal,sizeof(signal),0);
                if(ret<=0)
                {
                    continue;
                }
                else
                {
                    for(int i=0;i<ret;++i)
                    {
                        switch (signals[i])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1,&stat,WNOHANG))>0){
                                    continue;
                                }
                            }
                                break;
                            case SIGTERM:
                            case SIGINT:
                            {
                                _m_stop = true;
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }
            //如果是其他可读数据，那么必然是客户请求到来。
            //调用逻辑对象的process方法处理之
            else if(events[i].events & EPOLLIN){
                users[sockfd].process();
            }
            else
            {
                continue;
            }
        }
    }
    delete[] users;
    users = NULL;
    close(pipefd);
    //close(_m_listenfd);
    //我们将这句话注销掉，以提醒读者，应该有m_listenfd的创建者
    //来关闭这个文件描述符，即所谓的“对象（比如一个文件描述符，又或者一
    //堆内存）由那个函数创建，就应该由那个函数销毁
    close(_m_epollfd);
}
//父进程的run
//1.实现setup_sig_pipe 尽力通信管道设置
//2.把_m_listenfd监听端口加入到epoll对象中，进行监听
//3.主循环
//     a.epoll_wait() 提取队列中的事件，并循环处理提取出来的事件的fd
//     b.如果是_m_listenfd表示有客户端连接过来了，采用RR的简单轮询找到正在运行的子进程，通过_m_pipefd
//         管道给子进程发消息，告诉他要接收客户端信息了，如果没有找到可以运行的子进程表示所有子进程都挂了
//         ，那主循环标志弄成停止，关闭父进程
//     c.如果是sig_pipefd[0]的可读消息 那么接收内容并解析每个内容的信息，如果是SIGCHLD表示有子进程挂了，
//         同时将该进程对应的进程池内pid号置-1，并关闭父子进程的管道_m_sub_process[i]._m_pipefd[0]，回收资源
//         检查一下是不是所有的子进程都挂了，如果是父进程退出主循环，进程进程退出的资源清理
//     d.如果是SIGTERM或SIGINT表示终止进程了，循环用kill给子进程发送终止信号，等待子进程终止。在这里不用退
//         出主循环，而是等待子进程结束后发回标志过来，然后用waitpid清理完僵死进程，然后在SIGCHLD那里退出
// 4.退出进程时的一些处理，关闭fd文件描述符 回收资源
template<typename T>
void processpool<T>::run_parent()
{
    setup_sig_pipe();
    //父进程监听m_listenfd
    addfd(_m_epollfd,_m_listenfd);

    epoll_event events[MAX_EVENT_NUMBER];
    //这个sub_process_counter子进程计数还是有用的采用环形轮询，每个进程都可以分配到任务
    int sub_process_counter = 0;
    int new_conn = 1;
    int number = 0;//epoll监听事件相应数量
    int ret = -1;

    while(!_m_stop)
    {
        number = epoll_wait(_m_epollfd,events,MAX_EVENT_NUMBER,-1);
        if((number<0)&&(errno != EINTR))//EINTR表示中断
        {
            printf("epoll failure\n");
            break;
        }
        for(int i=0;i<number;++i)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == _m_listenfd)
            {
                //如果有新连接到来，就采用RR方式将其分配给一个子进程处理 RR（round-robin）简单轮询
                int i = sub_process_counter;//这个i是个局部变量 跟外循环的i不同
                do{
                    if(_m_sub_process[i]._m_pid != -1)
                    {
                        break;
                    }
                    i=(i+1)%_m_process_number;
                }while(i!=sub_process_counter);
                //能进这个if表示所有子进程都挂了
                if(_m_sub_process[i]._m_pid == -1)
                {
                    _m_stop = true;
                    break;
                }
                sub_process_counter = (i+1)%_m_process_number;
                //send( m_sub_process[sub_process_counter++].m_pipefd[0], ( char* )&new_conn, sizeof( new_conn ), 0 );
                send(_m_sub_process[i]._m_pipefd[0],(char*) new_conn,sizeof(new_conn),0);
                printf( "send request to child %d\n", i );
                //sub_process_counter %= m_process_number;
            }
            else if((sockfd == sig_pipefd[0])&&(events[i].events & EPOLLIN))
            {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
                if(ret<=0)
                {
                    continue;
                }
                else
                {
                    for(int i = 0;i<ret;++i)
                    {
                        //如果进程池中第i个子进程退出了，
                        //则主进程关闭通信管道，并设置相应的m_pid为-1，以标记该子进程已退出
                        switch (signal[i])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while((pid = waitpid(-1,&stat,WNOHANG))>0)
                                {
                                    //找出是哪个子进程退出了，把进程池里面关于他的记录改一下
                                    for(int i = 0;i<_m_process_number;++i)
                                    {
                                        if(_m_sub_process[i]._m_pid == pid)
                                        {
                                            printf("child %d join\n",i);
                                            close(_m_sub_process[i]._m_pipefd[0]);//把文件描述符关了，回收资源
                                            _m_sub_process[i]._m_pid = -1;
                                        }
                                    }
                                }
                                //如果所有的子进程都退出了，那么父进程也退出
                                _m_stop = true;
                                for(int i=0;i<_m_process_number;++i)
                                {
                                    if(_m_sub_process[i]._m_pid != -1)
                                    {
                                        _m_stop = false;
                                        break;
                                    }
                                }
                                break;
                            }
                            //中断信号
                            case SIGTERM:
                            case SIGINT:
                            {
                                //如果父进程接收到终止信号，那么就杀死所有子进程，并等待它们全部结束
                                //，所以在这里不用退出主循环，而是等待子进程结束后发回标志过来，然后用
                                //waitpid清理完僵死进程，然后在SIGCHLD那里退出
                                //通知子进程结束更好的方法是向父/子进程之间的通信管道发送特殊数据
                                printf("kill all the child now\n");
                                for(int i = 0;i<_m_process_number;++i)
                                {
                                    if(_m_sub_process[i]._m_pid != -1)
                                    {
                                        kill(_m_sub_process[i]._m_pid,SIGTERM);
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }
            else//这个是父进程管理进程池，除了_m_listenfd 和sig_pipefd[0]应该就没子进程跟父进程发消息了
            {
                continue;
            }
        }
    }
    //由创建者关闭这个文件描述符 哪个位置调用进程池的create，就在哪个地方的结尾关闭_m_listenfd;
    //close(_m_listenfd);
    close(_m_epollfd);
}

#endif