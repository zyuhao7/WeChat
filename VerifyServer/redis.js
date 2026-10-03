const config_module = require('./config')
const Redis = require("ioredis");

// create the Redis client instance
const RedisCli = new Redis({
  host: config_module.redis_host,        // Redis server host
  port: config_module.redis_port,        // Redis server port
  password: config_module.redis_passwd,  // Redis password

  enableOfflineQueue: false,             // disable the offline queue
  enableReadyCheck: true,                // enable connection-ready checking
});


/**
 * listen for connection error messages
 */
RedisCli.on("error", function (err) {
  console.log("Redis connection error", err);
  // try to reconnect
  RedisCli.connect();
});

// listen for the disconnect event
RedisCli.on("end", function () {
  console.log("Redis connection closed");
  // try to reconnect
  RedisCli.connect();
});
  
// heartbeat mechanism: send heartbeat messages on a timer
setInterval(() => {
  // send a heartbeat, e.g. write the current timestamp to a specific key
  RedisCli.set("heartbeat", Date.now());
}, 10000); // send a heartbeat every 10 seconds


/**
 * get the value by key
 * @param {*} key 
 * @returns 
 */
async function GetRedis(key) {
    try{
        const result = await RedisCli.get(key)
        if(result === null){
          console.log('result:','<'+result+'>', 'This key cannot be find...')
          return null
        }
        console.log('Result:','<'+result+'>','Get key success!...');
        return result
    }catch(error){
        console.log('GetRedis error is', error);
        return null 
    }
  }

/**
 * check whether the key exists in redis
 * @param {*} key 
 * @returns 
 */
async function QueryRedis(key){
    try{
        const result = await RedisCli.exists(key)
        //  check whether the value is empty; return null if so
        if (result === 0) {
          console.log('result:<','<'+result+'>','This key is null...');
          return null
        }
        console.log('Result:','<'+result+'>','With this value!...');
        return result
    }catch(error){
        console.log('QueryRedis error is', error);
        return null
    }

  }

/**
 * set the key and value with an expiration time
 * @param {*} key 
 * @param {*} value 
 * @param {*} exptime 
 * @returns 
 */
async function SetRedisExpire(key,value, exptime){
    try{
        // set the key and value
        await RedisCli.set(key,value)
        // set the expiration time (in seconds)
        await RedisCli.expire(key, exptime);
        return true;
    }catch(error){
        console.log('SetRedisExpire error is', error);
        return false;
    }
}

module.exports = {GetRedis, QueryRedis, SetRedisExpire,}