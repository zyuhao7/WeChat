const nodemailer = require('nodemailer');
const config_module = require("./config")

/**
 * create the mail-sending transport
 */
let transport = nodemailer.createTransport({
    host: config_module.email_host,
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user, // sender email address
        pass: config_module.email_pass // mailbox auth code or password
    }
});

/**
 * function that sends the mail
 * @param {*} mailOptions_ mail sending options
 * @returns 
 */
function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
    })
   
}

module.exports.SendMail = SendMail