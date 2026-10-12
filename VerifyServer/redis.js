const config_module = require('./config');
const Redis = require('ioredis');

// create the Redis client instance
const RedisCli = new Redis({
    host: config_module.redis_host, // Redis server host
    port: config_module.redis_port, // Redis server port
    password: config_module.redis_passwd, // Redis password

    enableOfflineQueue: false, // disable the offline queue
    enableReadyCheck: true, // enable connection-ready checking
});

// ioredis reconnects on its own (retryStrategy); calling connect() here again would
// reject with "already connecting" and crash the process as an unhandled rejection.
RedisCli.on('error', function (err) {
    console.log('Redis connection error', err);
});

RedisCli.on('end', function () {
    console.log('Redis connection closed');
});

// heartbeat: keeps the connection warm; with the offline queue disabled it fails while
// Redis is down, which is expected and must not crash the process
setInterval(() => {
    RedisCli.set('heartbeat', Date.now()).catch((err) => {
        console.log('Redis heartbeat failed', err.message);
    });
}, 10000);

/**
 * get the value by key
 * @param {*} key
 * @returns
 */
async function GetRedis(key) {
    try {
        const result = await RedisCli.get(key);
        if (result === null) {
            console.log('result:', '<' + result + '>', 'This key cannot be find...');
            return null;
        }
        console.log('Result:', '<' + result + '>', 'Get key success!...');
        return result;
    } catch (error) {
        console.log('GetRedis error is', error);
        return null;
    }
}

/**
 * check whether the key exists in redis
 * @param {*} key
 * @returns
 */
async function QueryRedis(key) {
    try {
        const result = await RedisCli.exists(key);
        //  check whether the value is empty; return null if so
        if (result === 0) {
            console.log('result:<', '<' + result + '>', 'This key is null...');
            return null;
        }
        console.log('Result:', '<' + result + '>', 'With this value!...');
        return result;
    } catch (error) {
        console.log('QueryRedis error is', error);
        return null;
    }
}

/**
 * set the key and value with an expiration time
 * @param {*} key
 * @param {*} value
 * @param {*} exptime
 * @returns
 */
async function SetRedisExpire(key, value, exptime) {
    try {
        // value and TTL in one command, so a failure can never leave a key without expiry
        await RedisCli.set(key, value, 'EX', exptime);
        return true;
    } catch (error) {
        console.log('SetRedisExpire error is', error);
        return false;
    }
}

/**
 * set the key only if it does not exist yet, with an expiration time
 * @returns true if set, false if the key already existed, null on error
 */
async function SetRedisNxExpire(key, value, exptime) {
    try {
        const result = await RedisCli.set(key, value, 'EX', exptime, 'NX');
        return result === 'OK';
    } catch (error) {
        console.log('SetRedisNxExpire error is', error);
        return null;
    }
}

async function DelRedis(key) {
    try {
        await RedisCli.del(key);
        return true;
    } catch (error) {
        console.log('DelRedis error is', error);
        return false;
    }
}

module.exports = { GetRedis, QueryRedis, SetRedisExpire, SetRedisNxExpire, DelRedis };
