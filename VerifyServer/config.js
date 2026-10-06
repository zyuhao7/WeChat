
const fs = require('fs');

if (!fs.existsSync('config.json')) {
  throw new Error('config.json not found; copy config.example.json to config.json and fill in your credentials');
}
let config = JSON.parse(fs.readFileSync('config.json', 'utf8'));
let email_user = config.email.user;
let email_pass = config.email.pass;
let redis_host = config.redis.host;
let redis_port = config.redis.port;
let redis_passwd = config.redis.passwd;
let code_prefix = "code_";


module.exports = {email_pass, email_user, redis_host, redis_port, redis_passwd, code_prefix}
