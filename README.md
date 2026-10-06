# Qt WeChat-style Instant Messaging App

> A cross-platform instant messaging client built with C++ and Qt, backed by a distributed
> microservice architecture on Boost.Asio and gRPC, supporting high-concurrency long
> connections, text messaging, and friend management.

## 📌 Overview

- **Client**: Qt + C++, with sign-in / sign-up / password reset, a chat window, friend requests
  and authorization, and message bubbles, styled with ElaWidgetTools (Fluent)
- **Backend services**: GateServer (HTTP gateway), StatusServer (status and scheduling),
  ChatServer (long connections and messaging), VerifyServer (email verification codes, Node.js)
- **Data**: Redis for caching and online state, MySQL as the primary store
- **Protocols**: a custom TCP frame between client and ChatServer, protobuf / gRPC between services

## 🚀 Highlights

- **Asynchronous, multi-threaded I/O**: a Boost.Asio `io_context` thread pool with one session
  object per connection, designed for high-concurrency long connections
- **Horizontally scalable**: ChatServer runs as multiple nodes that talk to each other over
  gRPC, and a new login can kick the stale connection on another node
- **End-to-end IM flow**: code-based registration → login for a token → friend list → request /
  authorization → text messages → heartbeat keepalive → offline notification
- **Modern client UI**: Fluent-styled widgets and hand-drawn message bubbles

## 🧱 Tech stack

| Layer | Technology |
|-------|------------|
| Client | C++, Qt 6, ElaWidgetTools, QSS, QPainter |
| Backend | C++, Boost.Asio / Beast, gRPC, protobuf |
| Data | Redis, MySQL |
| System | Linux, TCP/IP, epoll |
| Build | CMake (backend), qmake (client), shell scripts |

## 🏗️ Architecture and ports

```
        Client (Qt)
        │      │
   HTTP │      │ TCP long connection
  (8080)│      │ (8090)
        ▼      ▼
   ┌─────────┐  ┌──────────────────────┐
   │GateServer│  │      ChatServer      │
   │HTTP edge │  │ TCP 8090 / gRPC 50055│
   └────┬────┘  └───────┬──────────────┘
        │ gRPC          │ gRPC 50056 (node-to-node)
   ┌────▼─────┐   ┌─────▼──────┐
   │StatusServer│  │ ChatServer2│
   │ gRPC 50053│  │   peer     │
   └──────────┘   └────────────┘
        │
        ├── VerifyServer (gRPC 50052, email codes, Node.js)
        ├── MySQL 3306 (users / friends / messages)
        └── Redis 6380 (codes, tokens, online state, login counts)
```

| Service | Protocol | Port | Config |
|---------|----------|------|--------|
| GateServer | HTTP | 8080 | `GateServer/config.ini` |
| VerifyServer | gRPC | 50052 | `VerifyServer/config.json` |
| StatusServer | gRPC | 50053 | `StatusServer/config.ini` |
| ChatServer | TCP | 8090 | `ChatServer/config.ini` |
| ChatServer | gRPC | 50055 | same |
| ChatServer2 (peer) | gRPC | 50056 | `run/chatserver2/config.ini` |
| MySQL | TCP | 3306 | per-service config |
| Redis | TCP | 6380 | per-service config |

## 📁 Project layout

```
WeChat/
├── ChatClient/          Qt client (UI / bubble widgets / TCP+HTTP network / resources)
│   ├── third_party/ElaWidgetTools/   Fluent widget toolkit, compiled in as sources
│   └── tests/           Client unit tests (qmake project, one test binary)
├── GateServer/          HTTP gateway: register / login / password reset
├── StatusServer/        Status and scheduling: token issuance, load-balanced ChatServer pick
├── VerifyServer/        Node.js email verification-code service
├── ChatServer/          Long-connection and messaging service (TCP + gRPC + Redis / MySQL)
├── run/chatserver2/     A second ChatServer node, used to exercise cross-node logic
├── scripts/             Start/stop scripts and end-to-end scripts
├── sql/db01.sql         MySQL schema and stored procedures
├── tests/               Backend unit tests (CMake / GoogleTest)
├── CMakeLists.txt       Backend build (the three C++ services)
└── .github/workflows/   CI configuration
```

Each service directory also holds `.drawio` architecture / sequence diagrams, openable in draw.io.

## 🔧 Build and run

### Dependencies

```bash
# Backend (verified on Ubuntu 24.04)
sudo apt update
sudo apt install -y cmake build-essential pkg-config \
  libboost-all-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc \
  libgrpc++-dev libjsoncpp-dev libhiredis-dev libmysqlcppconn-dev \
  default-libmysqlclient-dev

# Client
sudo apt install -y qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
```

### Build the backend

```bash
cmake -S . -B build
cmake --build build -j4     # each gRPC / protobuf translation unit is heavy; -j$(nproc) gets OOM-killed on smaller machines
```

This produces `build/GateServer`, `build/StatusServer`, and `build/ChatServer`. The protobuf
sources are regenerated into `build/gen/` by the local `protoc` at build time and are not
committed.

### Build VerifyServer (Node.js)

```bash
cd VerifyServer
npm install
node server.js              # listens on 0.0.0.0:50052
```

### Build the client

```bash
cd ChatClient
mkdir -p build && cd build
qmake6 ../Chat.pro
make -j4                    # output: ChatClient/build/bin/Chat
```

Or use the launcher, which also copies the `config.ini` and `static/` the app expects next to
the binary:

```bash
scripts/start_client.sh
```

### Prepare the data stores

```bash
mysql -uroot -p < sql/db01.sql                       # schema and stored procedures
```

`scripts/start_all.sh` brings up Redis on :6380 itself (password `123456`, matching the sample
configs) and leaves an instance that is already running untouched.

### Configuration

Every service reads `config.ini` from **its own working directory**, so it must be started from
inside its directory. `VerifyServer/config.json` holds a mailbox app password and data-store
credentials, so it is not in the repository: copy `VerifyServer/config.example.json` and fill
it in.

### Start the stack

```bash
scripts/start_all.sh        # brings the services up in dependency order
scripts/stop_all.sh         # stops them
```

## ✅ Tests

The backend uses GoogleTest + CTest and the client uses Qt Test; both run in CI automatically.

```bash
# Backend: 5 test sets (proto codec, URL codec, frame-header bounds, message-node wire order, ChatServer)
cmake -S . -B build && cmake --build build -j4
ctest --test-dir build --output-on-failure

# Client: 4 suites (UserMgr, list paging, friend requests, text messages)
mkdir -p build/client-tests && cd build/client-tests
qmake6 ../../ChatClient/tests/tests.pro && make -j"$(nproc)"
QT_QPA_PLATFORM=offscreen ./client_tests
```

## 🔄 Continuous integration

`.github/workflows/ci.yml` runs two jobs on every push to master and on every pull request:

- **backend**: install backend dependencies → CMake build → `ctest`
- **client**: install Qt6 → build the client → build the tests → run them with
  `QT_QPA_PLATFORM=offscreen`

## 📚 Further documentation

- `WeChat_Project_Architecture.md` — architecture document (modules, protocols, key flows,
  remaining issues)
- `WeChat项目架构文档.md` — the same in Chinese
