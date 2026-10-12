#include <gtest/gtest.h>

#include "RedisMgr.h"

// Port 1 on loopback refuses immediately, so every slot starts out failed.
TEST(RedisConPool, UnreachableServerLeavesNoIdleConnections)
{
    RedisConPool pool(3, "127.0.0.1", 1, "pwd");
    EXPECT_EQ(pool.IdleCount(), 0u);
    EXPECT_EQ(pool.FailCount(), 3);
    EXPECT_EQ(pool.getConnNonBlock(), nullptr);
    pool.Close();
}

TEST(RedisConPool, CloseIsIdempotentAndUnblocksWaiters)
{
    RedisConPool pool(1, "127.0.0.1", 1, "");
    std::thread waiter([&pool] { EXPECT_EQ(pool.getConnection(), nullptr); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pool.Close();
    waiter.join();
    pool.Close();
    EXPECT_EQ(pool.getConnection(), nullptr);
    EXPECT_EQ(pool.getConnNonBlock(), nullptr);
}

TEST(RedisConPool, ReturningNullOrAfterCloseIsSafe)
{
    RedisConPool pool(1, "127.0.0.1", 1, "");
    pool.returnConnection(nullptr);
    EXPECT_EQ(pool.FailCount(), 1);
    pool.Close();
    EXPECT_EQ(pool.IdleCount(), 0u);
}

TEST(RedisConPool, ZeroSizedPool)
{
    RedisConPool pool(0, "127.0.0.1", 1, "");
    EXPECT_EQ(pool.IdleCount(), 0u);
    EXPECT_EQ(pool.FailCount(), 0);
    EXPECT_EQ(pool.getConnNonBlock(), nullptr);
}
