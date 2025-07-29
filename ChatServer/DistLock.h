#pragma once
#include <hiredis.h>
#include <string>

// 分布式锁
class DistLock
{
public:
	static DistLock& Inst();
	~DistLock() = default;
	//  lockTimeout 锁释放的超时时间.   acquireTime获取锁的等待时间.
	std::string acquireLock(redisContext* context, const std::string& lockName, int lockTimeout, int acquireTimeout);
	// identifier标识符，用于标识哪个客户端持有锁
	bool releaseLock(redisContext* context, const std::string& lockName, const std::string& identifier);
private:
	DistLock() = default;
};

