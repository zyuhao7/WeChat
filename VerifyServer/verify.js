const crypto = require('crypto');
const { code_prefix } = require('./const');

const CODE_LENGTH = 6;
const CODE_TTL_SECONDS = 600;
const RESEND_COOLDOWN_SECONDS = 60;
const MAX_EMAIL_LENGTH = 254;
const MAX_LOCAL_LENGTH = 64;

// Local part: anything but whitespace and '@' (UTF-8 local parts are allowed).
// Domain: dot-separated labels of letters/digits/hyphens (IDN allowed), no leading/trailing hyphen.
const EMAIL_RE = /^[^\s@]+@(?:[\p{L}\p{N}](?:[\p{L}\p{N}-]*[\p{L}\p{N}])?\.)+[\p{L}]{2,}$/u;

function genCode() {
    return crypto
        .randomInt(0, 10 ** CODE_LENGTH)
        .toString()
        .padStart(CODE_LENGTH, '0');
}

// The address is used verbatim as the Redis key that GateServer later looks up, so
// it is validated rather than normalized: surrounding spaces are rejected, not trimmed.
function isValidEmail(email) {
    if (typeof email !== 'string' || email.length === 0 || email.length > MAX_EMAIL_LENGTH) {
        return false;
    }
    const at = email.indexOf('@');
    if (at < 1 || at > MAX_LOCAL_LENGTH) {
        return false;
    }
    return EMAIL_RE.test(email);
}

function codeKey(email) {
    return code_prefix + email;
}

// Distinct leading prefix so no email can make this collide with a codeKey().
function cooldownKey(email) {
    return 'verify_cooldown_' + email;
}

function mailText(code) {
    return '您的验证码为' + code + '，请在' + Math.round(CODE_TTL_SECONDS / 60) + '分钟内完成验证';
}

module.exports = {
    CODE_LENGTH,
    CODE_TTL_SECONDS,
    RESEND_COOLDOWN_SECONDS,
    MAX_EMAIL_LENGTH,
    genCode,
    isValidEmail,
    codeKey,
    cooldownKey,
    mailText,
};
