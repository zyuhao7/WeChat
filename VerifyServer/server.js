const grpc = require('@grpc/grpc-js');
const message_proto = require('./proto');
const const_module = require('./const');
const config_module = require('./config');
const verify = require('./verify');
const emailModule = require('./email');
const redis_module = require('./redis');

const PORT = 50052;

/**
 * GetVerifyCode grpc service that returns the verify code
 * @param {*} call the grpc request
 * @param {*} callback the grpc callback
 * @returns
 */
async function GetVerifyCode(call, callback) {
    const email = call.request.email;
    console.log('email is ', email);
    const reply = (error) => callback(null, { email: email, error: error });

    if (!verify.isValidEmail(email)) {
        reply(const_module.Errors.InvalidEmail);
        return;
    }

    // claim the resend slot first so concurrent requests can't each send a mail
    const cooldown = verify.cooldownKey(email);
    const claimed = await redis_module.SetRedisNxExpire(
        cooldown,
        '1',
        verify.RESEND_COOLDOWN_SECONDS,
    );
    if (claimed === null) {
        reply(const_module.Errors.RedisErr);
        return;
    }
    if (!claimed) {
        reply(const_module.Errors.TooFrequent);
        return;
    }

    try {
        // resend the still-valid code so an earlier mail keeps working
        let code = await redis_module.GetRedis(verify.codeKey(email));
        if (code === null) {
            code = verify.genCode();
        }
        const saved = await redis_module.SetRedisExpire(
            verify.codeKey(email),
            code,
            verify.CODE_TTL_SECONDS,
        );
        if (!saved) {
            await redis_module.DelRedis(cooldown);
            reply(const_module.Errors.RedisErr);
            return;
        }

        let mailOptions = {
            from: config_module.email_user,
            to: email,
            subject: '验证码',
            text: verify.mailText(code),
        };

        let send_res = await emailModule.SendMail(mailOptions);
        console.log('send res is ', send_res);

        reply(const_module.Errors.Success);
    } catch (error) {
        console.log('catch error is ', error);
        // a failed send must not lock the user out of retrying
        await redis_module.DelRedis(cooldown);
        reply(const_module.Errors.Exception);
    }
}

function main() {
    var server = new grpc.Server();
    server.addService(message_proto.VerifyService.service, { GetVerifyCode: GetVerifyCode });
    server.bindAsync('0.0.0.0:' + PORT, grpc.ServerCredentials.createInsecure(), (err) => {
        if (err) {
            console.error('verify server failed to bind on ' + PORT + ':', err.message);
            process.exit(1);
        }
        console.log('verify server started on ' + PORT);
    });
}

main();
