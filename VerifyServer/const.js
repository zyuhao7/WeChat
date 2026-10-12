let code_prefix = "code_";

// GateServer forwards these to the client verbatim; the client treats any non-zero as failure.
const Errors = {
    Success : 0,
    RedisErr : 1,
    Exception : 2,
    InvalidEmail : 3,
    TooFrequent : 4,
};


module.exports = {code_prefix,Errors}