#ifndef TASKQUEUE_H
#define TASKQUEUE_H
#include <iostream>
#include <string>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <stdint.h>
#define SHMQUEUEOK 1
#define SHMQUEUEERROR -1
#define SHMKEY   20190301//用于共享内存创建的键值
#define SEMKEY   20190302//用于信号量创建的键值 

//这个队列这里应该优化，传入key值，这样可以控制共享内存打开哪一个

class SemLock//通过信号量实现锁
{
public:
	union semun {
		int val; /* value for SETVAL */
		struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
		unsigned short *array; /* array for GETALL, SETALL */
		struct seminfo *__buf; /* buffer for IPC_INFO */
	};
	SemLock() :m_iSemId(-1), m_isCreate(-1) {
	}
	~SemLock() {
	}
	int Init() ;
	int Lock() ;
	int unLock() ;
	inline std::string GetErrMsg() {return m_sErrMsg;} ;
private:
	int m_iSemId;
	int m_isCreate;
	std::string m_sErrMsg;
};
class semLockGuard //RAII锁的控制对象
{
public:
	semLockGuard(SemLock sem) :m_Sem(sem) {
		m_Sem.Lock();
	}
	~semLockGuard() {
		m_Sem.unLock();
	}
private:
	SemLock &m_Sem;
};

static const uint32_t SHMSIZE = 4*100+24;//这里的24是下面六个成员变量需要占用的空间 4*100是共享内存占得空间 应该要弄大一点
class QueueHead {
public:
	uint32_t uDataCount;//数据量计数
	uint32_t uFront;//队头
	uint32_t uRear;//当前取数据头
	uint32_t uAllCount;//总计数
	uint32_t uAllSize;//总大小
	uint32_t uItemSize;//队列中单个项目所占大小
public:
	QueueHead() :uDataCount(0), uFront(0), uRear(0),
		uAllCount(0), uAllSize(0), uItemSize(0){
	}
};
template<typename T>
class TaskQueue //RAII，这是一个固定队列大小的基于共享内存的先入先出环形队列
{
public:
explicit TaskQueue(int iCreate=-1):m_pShm(NULL),m_iStatus(-1), m_iShmId(-1), m_iCreate(iCreate){
		m_pShm = NULL;
	}
	~TaskQueue() {
		shmdt(m_pShm);//将共享内存块从当前进程分离，只是让共享内存对当前进程不再可用
	}
	int Init();
	bool isEmpty();
	bool isFull();
	int getSize();
	int  push(T a);
	T pop();
	inline std::string GetErrMsg() {return m_sErrMsg;};
private:
	char *m_pShm;//共享内存开始地址
	int m_iShmId;//共享内存标识符
	int m_iStatus;
	int m_iCreate;//共享内存是否创建
	SemLock m_Sem;//该队列的信号锁
	std::string m_sErrMsg;
};

template<typename T>
int TaskQueue<T>::Init()
{
	int ret = m_Sem.Init();//初始化信号量
	if (ret < 0) {
		m_sErrMsg.clear();
		m_sErrMsg = m_Sem.GetErrMsg();
		return ret;
	}
	m_iShmId = shmget(SHMKEY,SHMSIZE,0);//根据key开辟指定大小的共享内存，返回的是共享内存标识符
	if (m_iShmId < 0) {//如果这块内存不存在就要自己创建
		m_iCreate = 1;
		m_iShmId = shmget(SHMKEY, SHMSIZE, IPC_CREAT);
	}
	if (m_iShmId < 0) {
		m_sErrMsg.clear();
		m_sErrMsg = "shmget error ";
		m_iStatus = SHMQUEUEERROR;
		return m_iShmId;
	}
	m_pShm = (char*)shmat(m_iShmId,NULL,0);//读写模式，根据共享内存标识符获取内存起点
	if (m_pShm == NULL) {
		m_sErrMsg.clear();
		m_sErrMsg = "shmat error ";
		return -1;
	}

	if (m_iCreate == 1) {
		QueueHead oQueryHead;//初始化队列的信息
		oQueryHead.uItemSize = sizeof(T);//根据初始化的模板类确定Item大小
		oQueryHead.uAllSize = SHMSIZE - 24;
		oQueryHead.uAllCount = oQueryHead.uAllSize / oQueryHead.uItemSize;
		oQueryHead.uDataCount = 0;
		oQueryHead.uFront = 0;
		oQueryHead.uRear = 0;
		memcpy(m_pShm,&oQueryHead,sizeof(QueueHead));//把队列信息复制共享内存区
	}
	
	return m_iShmId;
};

template<typename T>
bool TaskQueue<T>::isEmpty()
{
	QueueHead oQueryHead;
	memcpy(&oQueryHead,m_pShm,sizeof(QueueHead));//因为m_pShm只是一个地址，所以要把这个地址
	return oQueryHead.uFront == oQueryHead.uRear;//开头的信息取出来复制到队列头结构体中，再通过结构体读取队列信息
};

template<typename T>
bool TaskQueue<T>::isFull() {
	QueueHead oQueryHead;
	memcpy(&oQueryHead, m_pShm, sizeof(QueueHead));
	return oQueryHead.uFront == (oQueryHead.uRear + 1) % oQueryHead.uAllCount;
}

template<typename T>
int TaskQueue<T>::getSize() {
	QueueHead oQueryHead;
	memcpy(&oQueryHead, m_pShm, sizeof(QueueHead));
	return oQueryHead.uDataCount;
}

template<typename T>
int TaskQueue<T>::push(T a){
	semLockGuard oLock(m_Sem);//改变队列信息的时候要通过信号锁，进程安全
	if (isFull()) {//满了就不能放了
		return -1;
	}
	QueueHead oQueryHead;
	memcpy(&oQueryHead, m_pShm, sizeof(QueueHead));//获取队列信息
	char *p = m_pShm + sizeof(QueueHead) + oQueryHead.uRear * sizeof(T);//计算队列尾的开始地址
	//(uint32_t*)(m_pShm+sizeof(int)*2) 这个东西在计算队列信息结构体中uRear的开始地址
	*((uint32_t*)(m_pShm + sizeof(int) * 2)) = (oQueryHead.uRear + 1) % oQueryHead.uAllCount;//更新队列尾的位置
	memcpy(p,&a,sizeof(T));//把Item放入队列
	return 0;
}

template<typename T>
T TaskQueue<T>::pop() 
{
	semLockGuard oLock(m_Sem);
	if (isEmpty()) {
		return T();//应该选择抛出异常等方式，待改进
	}
	QueueHead oQueryHead;
	memcpy(&oQueryHead, m_pShm, sizeof(QueueHead));
	char *p = m_pShm + sizeof(QueueHead) + oQueryHead.uFront * sizeof(T);//计算队列头的开始地址
	
	T odata;
	memcpy(&odata,p,sizeof(T));//取出数据
	//(uint32_t*)(m_pShm+sizeof(int)*1) 这个东西在计算队列信息结构体中uFront的开始地址
	*((uint32_t*)(m_pShm+sizeof(int)*1))= ((oQueryHead.uFront+ 1) % oQueryHead.uAllCount);//更新队列头的位置
	return odata;
}


#endif // !SHMQUEUE_H

