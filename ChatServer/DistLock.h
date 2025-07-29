#pragma once
#include <hiredis.h>
#include <string>

// �ֲ�ʽ��
class DistLock
{
public:
	static DistLock& Inst();
	~DistLock() = default;
	//  lockTimeout ���ͷŵĳ�ʱʱ��.   acquireTime��ȡ���ĵȴ�ʱ��.
	std::string acquireLock(redisContext* context, const std::string& lockName, int lockTimeout, int acquireTimeout);
	// identifier��ʶ�������ڱ�ʶ�ĸ��ͻ��˳�����
	bool releaseLock(redisContext* context, const std::string& lockName, const std::string& identifier);
private:
	DistLock() = default;
};

