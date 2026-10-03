#pragma once
#include <hiredis.h>
#include <string>

/**
 * @brief distributed lock implementation class
 */
class DistLock
{
public:
	static DistLock& Inst();
	~DistLock() = default;
	//  lockTimeout is the lock release timeout. acquireTime is the wait time to acquire the lock.
	std::string acquireLock(redisContext* context, const std::string& lockName, int lockTimeout, int acquireTimeout);
	// identifier, identifies which client holds the lock
	bool releaseLock(redisContext* context, const std::string& lockName, const std::string& identifier);
private:
	DistLock() = default;
};

