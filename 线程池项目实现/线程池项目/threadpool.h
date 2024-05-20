#pragma once
#ifndef THREADPOLL_H
#define THREADPOLL_H

#include<vector>
#include<queue>
#include<memory>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<unordered_map>

//any ���ͣ� ���Խ��������������ݵ�����
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator = (const Any&) = delete;
	Any(Any&&) = default;
	Any& operator = (Any&&) = default;

	//������캯��������Any���ͽ�����������������
	template<typename T>  
	Any (T data) : base_(std::make_unique<Derive<T>>(data))
	{}

	//��������ܰ�Any��������洢��data������ȡ����
	template<typename T>
	T cast_()
	{
		//��ô��base�ҵ�����ָ���derive���󣬴�������ȡ��data��Ա����
		//  ����ָ��  =�� ������ָ��
		Derive<T>* pd = dynamic_cast(Derive<T>*)(base_.get());
		if (pd == nullptr)
		{
			throw "type is unmatch!";
		}
		return pd->data;
	}
private:
	//��������
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	//����������
	template<typename T>
	class Derive : public Base
	{
	public:
		Derive(T data) : data_(data){}
		T data_;  //�������������������
	};
private:
	//����һ�������ָ��
	std::unique_ptr<Base> base_;
};

//ʵ��һ���ź�����
class Semaphore
{
public:
	Semaphore(int limit=0):resLimit_(limit){}
	~Semaphore() = default;

	//��ȡһ���ź�����Դ
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		// �ȴ��ź�������Դ��û����Դ�Ļ�����������ǰ�߳�
		cond.wait(lock, [&]()->bool {return resLimit_ > 0 });
		resLimit_--;
	}

	// ����һ���ź���
	void post()
	{
		std::unqiue_lock<std::mutex> lock(mtx_);
		resLimit++;
		//linux�µ�condition_variable����������ʲôҲû�в�������������״̬ʧЧ���޹�����
		cond_.ontify_all();// ֪ͨwait
	}
private:
	int resLimit_;
	std::mutex mtx_;
	std::condition_variable cond_;
};

//Task���͵�ǰ������
class Task;

//ʵ�ֽ����ύ���̳߳ص�task����ִ����ɺ�ķ���ֵ���� Result
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isVaild_ = true);
	~Result() = default;
	//����һ  setVal��������ȡ����ִ����ɵķ���ֵ
	void setVal(Any any);
	//  �����  get����  �û��������������ȡtask�ķ���ֵ
	Any get();
private:
	Any any_; //�洢����ķ���ֵ
	Semaphore sem_; //�߳�ͨ���ź���
	std::shared_ptr<Task> task_; //ָ���Ӧ��ȡ����ֵ���������
	std::atomic_bool isVaild_;  //����ֵ�Ƿ���Ч
};

// ����������
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);

	// �û������Զ��������������ͣ���Task�̳У���дrun������ʵ���Զ���������
	virtual Any  run() = 0;


private:
	Result* result_; 
};

//�̳߳�֧�ֵ�ģʽ
enum class PoolMode
{
	MODE_FIXED, //�̶��������߳�
	MODE_CACHED,  //�߳������ɶ�̬����
};

//�߳�����
class  Thread
{
public:
	//�̺߳�����������
	using ThreadFunc = std::fuction<void(int)>;

	//�̹߳���
	Thread(ThreadFunc func);
	~Thread();
	//�����߳�
	void start();

	//��ȡ�߳�id
	int getId() const;

private:
	ThreadFunc func_;
	static int generateId_;
	int threadId_; //�����߳�id
};
//�̳߳�����
class ThreadPool
{
public:
	//�̳߳ع���
	ThreadPool();
	~ThreadPool();
	//�����̳߳صĹ�����ʽ
	void setMode(PoolMode mode);

	// ����task�������������ֵ
	void setTaskQueMaxThreshHold(int threshhold);

	// �����̳߳�cachedģʽ���߳���ֵ
	void setThreadSizeThreshHold(int threshhold);

	// ���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);

	// �����̳߳�
	void start(int initThreadSize = std::thread::hardware_concurrency());

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

private:
	//�����̺߳���
	void threadFunc(int threadid);
	//���pool������״̬
	bool checkRunningState() const;

private:
	std::unorder_map<int, std::unique_ptr<Thread>> threads_; //�߳��б�

	int initThreadSize_; //��ʼ�߳�����
	int threadSizeThreshHold_;  //�߳�����������ֵ
	std::atomic_int curThreadSize_;  //��¼��ǰ�̳߳������̵߳�������
	std::atomic_int idleThreadSize_; //��¼�����߳�����

	std::queue<std::shared_ptr<Task>>  taskQue_;  //�������
	std::atomic_int taskSize_;  //��������
	int taskQueMaxThreshHold_;   //�������������������ֵ

	std::mutex taskQueMtx_;  //��֤��������̰߳�ȫ
	std::condition_variable notFull_;     //��ʾ������в���
	std::condition_variable notEmpty_;    //��ʾ������в���
	std::condition_variable exitCond_;   //�ȵ��߳���Դȫ������

	PoolMode poolMode_;  //��ǰ�̳߳ع���ģʽ
	std::atomic_bool isPoolRunning_;  //��ʾ��ǰ�̳߳ص�����״̬
};

#endif
