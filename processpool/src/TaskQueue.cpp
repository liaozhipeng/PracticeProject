#include "TaskQueue.h"

int SemLock::Init()
{
	key_t key = ftok(PATHNAME,PROJ_ID1);
		if(key < 0)
		{
			std::cerr << "ftok error" << std::endl;
			exit(2);
		}
	m_iSemId = ::semget(key, 1, 0);//根据key创建信号集，数量为1，flag标志位没东西
	if (m_iSemId < 0) {
		m_iSemId = ::semget(key, 1, IPC_CREAT);//如果信号量不存在就自己创建一个
		m_isCreate = 1;
	}
	if (m_iSemId < 0) {
		m_sErrMsg.clear();
		m_sErrMsg = "semget error ";
		return m_iSemId;
	}
	if (m_isCreate == 1) {
		union semun arg;
		arg.val = 1;
		int ret = semctl(m_iSemId, 0, SETVAL, arg.val);//设置信号量的值为1
		if (ret < 0) {
			m_sErrMsg.clear();
			m_sErrMsg = "sem setval error ";
			return ret;
		}
	}
	return m_iSemId;//返回信号量的ID
};

int SemLock::Lock()
{
    union semun arg;
	int val = semctl(m_iSemId, 0, GETVAL, arg);//获取信号量的值，如果不为1就返回0
	if (val == 1) {//信号量-1
		struct sembuf sops = { 0,-1, SEM_UNDO };
		int ret = semop(m_iSemId, &sops, 1);
		if (ret < 0) {
			m_sErrMsg.clear();
			m_sErrMsg = "semop -- error ";
			return ret;
		}
	}
	return 0;
};

int SemLock::unLock() 
{
	union semun arg;
	int val = semctl(m_iSemId, 0, GETVAL, arg);
	if (val == 0) {//信号量+1
		struct sembuf sops = { 0,+1, SEM_UNDO };
		int ret = semop(m_iSemId, &sops, 1);
		if (ret < 0) {
			m_sErrMsg.clear();
			m_sErrMsg = "semop ++ error ";
			return ret;
		}
	}
	return 0;
};
