#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = []
# ///
"""End-to-end TCP smoke test for the ChatServer protocol.

Covers: chat login, user search, add-friend apply + notify, friend auth +
notify, text chat message + notify. Requires the stack running and mock
accounts seeded (scripts/mock_accounts.sh).
"""
import json
import socket
import struct
import sys
import urllib.request

GATE = "http://127.0.0.1:8080"

MSG_LOGIN = 1005
MSG_LOGIN_RSP = 1006
ID_SEARCH_REQ = 1007
ID_SEARCH_RSP = 1008
ID_ADD_REQ = 1009
ID_ADD_RSP = 1010
ID_NOTIFY_ADD = 1011
ID_AUTH_REQ = 1013
ID_AUTH_RSP = 1014
ID_NOTIFY_AUTH = 1015
ID_TEXT_REQ = 1017
ID_TEXT_RSP = 1018
ID_NOTIFY_TEXT = 1019

results = []


def check(name, ok, detail=""):
    results.append((name, ok))
    print(f"[{'PASS' if ok else 'FAIL'}] {name} {detail}")


def http_post(path, obj):
    req = urllib.request.Request(
        GATE + path,
        data=json.dumps(obj).encode(),
        headers={"Content-Type": "application/json"},
    )
    return json.loads(urllib.request.urlopen(req).read().decode())


class Conn:
    def __init__(self, host, port, name):
        self.name = name
        self.sock = socket.create_connection((host, int(port)), timeout=5)
        self.sock.settimeout(5)
        self.pending = []

    def send(self, mid, obj):
        body = json.dumps(obj).encode()
        self.sock.sendall(struct.pack(">HH", mid, len(body)) + body)

    def _read(self, n):
        buf = b""
        while len(buf) < n:
            chunk = self.sock.recv(n - len(buf))
            if not chunk:
                raise EOFError(f"{self.name}: connection closed")
            buf += chunk
        return buf

    def recv(self):
        mid, ln = struct.unpack(">HH", self._read(4))
        body = json.loads(self._read(ln).decode())
        return mid, body

    def expect(self, mid, what):
        # drain already-buffered frames first
        for i, (m, b) in enumerate(self.pending):
            if m == mid:
                return self.pending.pop(i)[1]
        while True:
            m, b = self.recv()
            if m == mid:
                return b
            self.pending.append((m, b))

    def close(self):
        self.sock.close()


def login(email):
    r = http_post("/user_login", {"email": email, "passwd": "123456"})
    if r.get("error") != 0:
        raise SystemExit(f"http login failed for {email}: {r}")
    return r


def tcp_login(cred, name):
    c = Conn(cred["host"], cred["port"], name)
    c.send(MSG_LOGIN, {"uid": cred["uid"], "token": cred["token"]})
    rsp = c.expect(MSG_LOGIN_RSP, name)
    check(f"{name} tcp login", rsp.get("error") == 0, f"error={rsp.get('error')}")
    check(f"{name} login payload", rsp.get("name") == name, f"name={rsp.get('name')}")
    return c


def main():
    # Fetch A's rendezvous first, then TCP-login A so its server's login
    # count rises before B asks for a server — this spreads A and B onto
    # different ChatServers and exercises the cross-node gRPC notify path.
    a_cred = login("mock1@test.com")
    a = tcp_login(a_cred, "mock1")
    b_cred = login("mock2@test.com")
    b = tcp_login(b_cred, "mock2")
    print(f"mock1 uid={a_cred['uid']} -> {a_cred['host']}:{a_cred['port']}")
    print(f"mock2 uid={b_cred['uid']} -> {b_cred['host']}:{b_cred['port']}")
    cross = (a_cred["host"], a_cred["port"]) != (b_cred["host"], b_cred["port"])
    print(f"cross-node: {cross}")

    # 1. search user by name (server returns a flat user object)
    a.send(ID_SEARCH_REQ, {"uid": "mock2"})
    rsp = a.expect(ID_SEARCH_RSP, "mock1")
    found = rsp.get("uid") == b_cred["uid"] and rsp.get("name") == "mock2"
    check("search user by name", rsp.get("error") == 0 and found, f"rsp={rsp}")

    # 2. add friend apply -> notify peer
    a.send(ID_ADD_REQ, {
        "uid": a_cred["uid"],
        "applyname": "mock1",
        "bakname": "mock1",
        "touid": b_cred["uid"],
    })
    rsp = a.expect(ID_ADD_RSP, "mock1")
    check("add-friend apply rsp", rsp.get("error") == 0, f"error={rsp.get('error')}")
    notify = b.expect(ID_NOTIFY_ADD, "mock2")
    check("peer got add-friend notify",
          notify.get("applyuid") == a_cred["uid"], f"notify={notify}")

    # 3. friend auth (peer approves) -> notify applicant
    b.send(ID_AUTH_REQ, {
        "fromuid": b_cred["uid"],
        "touid": a_cred["uid"],
        "back": "hi-mock1",
    })
    rsp = b.expect(ID_AUTH_RSP, "mock2")
    check("auth-friend rsp", rsp.get("error") == 0, f"error={rsp.get('error')}")
    notify = a.expect(ID_NOTIFY_AUTH, "mock1")
    check("applicant got auth notify",
          notify.get("fromuid") == b_cred["uid"], f"notify={notify}")

    # 4. text chat -> rsp to sender + notify to peer
    a.send(ID_TEXT_REQ, {
        "fromuid": a_cred["uid"],
        "touid": b_cred["uid"],
        "text_array": [{"msgid": "1", "content": "hello from mock1"}],
    })
    rsp = a.expect(ID_TEXT_RSP, "mock1")
    check("text chat rsp", rsp.get("error") == 0, f"error={rsp.get('error')}")
    notify = b.expect(ID_NOTIFY_TEXT, "mock2")
    ok = (notify.get("fromuid") == a_cred["uid"]
          and notify.get("text_array", [{}])[0].get("content") == "hello from mock1")
    check("peer got text notify", ok, f"notify={notify}")

    a.close()
    b.close()

    failed = [n for n, ok in results if not ok]
    print(f"\n{len(results) - len(failed)}/{len(results)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
