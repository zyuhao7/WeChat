const test = require('node:test')
const assert = require('node:assert/strict')
const verify = require('./verify')
const { code_prefix } = require('./const')

test('genCode is always CODE_LENGTH digits', () => {
    for (let i = 0; i < 2000; i++) {
        const code = verify.genCode()
        assert.match(code, /^\d+$/)
        assert.equal(code.length, verify.CODE_LENGTH)
    }
})

test('genCode keeps leading zeros', () => {
    const seen = new Set()
    for (let i = 0; i < 20000; i++) {
        seen.add(verify.genCode()[0])
    }
    assert.ok(seen.has('0'), 'a leading zero never appeared in 20000 codes')
})

test('isValidEmail accepts ordinary addresses', () => {
    for (const email of [
        'a@b.co',
        'mock1@test.com',
        'first.last+tag@sub.example.org',
        'user_name-1@my-host.io',
        '用户@例子.中国',
    ]) {
        assert.equal(verify.isValidEmail(email), true, email)
    }
})

test('isValidEmail rejects malformed input', () => {
    for (const email of [
        '',
        '@',
        'a@',
        '@b.com',
        'plain',
        'a@b',
        'a@b.c',
        'a@@b.com',
        'a@b..com',
        'a@.b.com',
        'a@-b.com',
        'a@b-.com',
        'a b@c.com',
        ' a@b.com',
        'a@b.com ',
        'a@b.com\n',
    ]) {
        assert.equal(verify.isValidEmail(email), false, JSON.stringify(email))
    }
})

test('isValidEmail rejects non-strings', () => {
    for (const v of [undefined, null, 0, 1, {}, [], ['a@b.com'], true]) {
        assert.equal(verify.isValidEmail(v), false, String(v))
    }
})

test('isValidEmail enforces the local-part and total length limits', () => {
    const domain = '@example.com'
    assert.equal(verify.isValidEmail('a'.repeat(64) + domain), true)
    assert.equal(verify.isValidEmail('a'.repeat(65) + domain), false)

    const local = 'a'.repeat(64)
    const longDomain = `@${'b'.repeat(60)}.${'c'.repeat(60)}.${'d'.repeat(63)}.com`
    assert.equal((local + longDomain).length, verify.MAX_EMAIL_LENGTH)
    assert.equal(verify.isValidEmail(local + longDomain), true)
    assert.equal(verify.isValidEmail(local + 'e' + longDomain), false)
})

test('codeKey matches the key GateServer reads', () => {
    assert.equal(verify.codeKey('a@b.com'), code_prefix + 'a@b.com')
})

test('cooldownKey can never collide with a codeKey', () => {
    for (const email of ['a@b.com', 'verify_cooldown_a@b.com', '']) {
        assert.notEqual(verify.cooldownKey(email), verify.codeKey(email))
        assert.ok(!verify.cooldownKey(email).startsWith(code_prefix))
    }
})

test('mailText states the real TTL and carries the code', () => {
    const text = verify.mailText('012345')
    assert.ok(text.includes('012345'))
    assert.ok(text.includes(String(verify.CODE_TTL_SECONDS / 60)))
})

test('cooldown is shorter than the code lifetime', () => {
    assert.ok(verify.RESEND_COOLDOWN_SECONDS < verify.CODE_TTL_SECONDS)
})
