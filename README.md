# Qt 仿微信即时通信软件

> 使用 C++ + Qt 构建的跨平台即时通信客户端，后端为 Boost.Asio + gRPC 的分布式微服务架构，支持高并发长连接、文本消息收发与好友管理。

## 📌 项目简介

- **客户端**：Qt + C++，包含登录 / 注册 / 找回密码、聊天窗口、好友申请与认证、消息气泡，界面基于 ElaWidgetTools（Fluent 风格）
- **后端服务**：GateServer（HTTP 网关）、StatusServer（状态与调度）、ChatServer（长连接与消息）、VerifyServer（邮件验证码，Node.js）
- **数据管理**：Redis 做缓存与在线状态，MySQL 作为主存储
- **通信协议**：客户端与 ChatServer 之间为自定义 TCP 帧，服务之间为 protobuf / gRPC

## 🚀 项目特点

- **异步 IO 多线程模型**：Boost.Asio `io_context` 线程池 + 每连接独立会话，面向高并发长连接设计
- **可水平扩展**：ChatServer 支持多节点部署，节点间通过 gRPC 互通，登录可跨节点踢掉旧连接
- **完整 IM 链路**：验证码注册 → 登录换 token → 拉取好友 → 申请 / 认证 → 文本消息 → 心跳保活 → 离线通知
- **现代客户端 UI**：Fluent 风格控件与自绘消息气泡

## 🧱 技术栈

| 模块 | 技术 |
|------|------|
| 客户端 | C++, Qt 6, ElaWidgetTools, QSS, QPainter |
| 后端 | C++, Boost.Asio / Beast, gRPC, protobuf |
| 数据 | Redis, MySQL |
| 系统 | Linux, TCP/IP, epoll |
| 构建 | CMake（后端）, qmake（客户端）, Shell 脚本 |

## 🏗️ 架构与端口

```
        Client (Qt)
        │      │
   HTTP │      │ TCP 长连接
  (8080)│      │ (8090)
        ▼      ▼
   ┌─────────┐  ┌──────────────────────┐
   │GateServer│  │      ChatServer      │
   │ HTTP 网关│  │ TCP 8090 / gRPC 50055│
   └────┬────┘  └───────┬──────────────┘
        │ gRPC          │ gRPC 50056（节点互通）
   ┌────▼─────┐   ┌─────▼──────┐
   │StatusServer│  │ ChatServer2│
   │ gRPC 50053│  │   peer     │
   └──────────┘   └────────────┘
        │
        ├── VerifyServer（gRPC 50052，邮件验证码，Node.js）
        ├── MySQL 3306（用户 / 好友 / 消息）
        └── Redis 6380（验证码、token、在线状态、登录计数）
```

| 服务 | 协议 | 端口 | 配置 |
|------|------|------|------|
| GateServer | HTTP | 8080 | `GateServer/config.ini` |
| VerifyServer | gRPC | 50052 | `VerifyServer/config.json` |
| StatusServer | gRPC | 50053 | `StatusServer/config.ini` |
| ChatServer | TCP | 8090 | `ChatServer/config.ini` |
| ChatServer | gRPC | 50055 | 同上 |
| ChatServer2（对等节点） | gRPC | 50056 | `run/chatserver2/config.ini` |
| MySQL | TCP | 3306 | 各服务配置 |
| Redis | TCP | 6380 | 各服务配置 |

## 📁 项目结构

```
WeChat/
├── ChatClient/          Qt 客户端（UI / 气泡控件 / TCP+HTTP 网络层 / 资源）
│   ├── third_party/ElaWidgetTools/   Fluent 控件库，以源码集直接编入
│   └── tests/           客户端单元测试（qmake 工程，单一测试可执行文件）
├── GateServer/          HTTP 网关：注册 / 登录 / 找回密码
├── StatusServer/        状态与调度：签发 token、按负载选 ChatServer
├── VerifyServer/        Node.js 邮件验证码服务
├── ChatServer/          长连接与消息服务（TCP + gRPC + Redis / MySQL）
├── run/chatserver2/     第二个 ChatServer 节点，用于验证跨节点逻辑
├── scripts/             启停脚本与端到端脚本
├── sql/db01.sql         MySQL 建表语句 + 存储过程
├── tests/               后端单元测试（CMake / GoogleTest）
├── CMakeLists.txt       后端构建脚本（三个 C++ 服务）
└── .github/workflows/   CI 配置
```

各服务目录下另有 `.drawio` 架构 / 时序图，可用 draw.io 打开。

## 🔧 构建与运行

### 依赖

```bash
# 后端（Ubuntu 24.04 验证）
sudo apt update
sudo apt install -y cmake build-essential pkg-config \
  libboost-all-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc \
  libgrpc++-dev libjsoncpp-dev libhiredis-dev libmysqlcppconn-dev \
  default-libmysqlclient-dev

# 客户端
sudo apt install -y qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
```

### 后端构建

```bash
cmake -S . -B build
cmake --build build -j4     # 每个 gRPC / protobuf 编译单元都很重，-j$(nproc) 在内存较小的机器上会被 OOM kill
```

产物为 `build/GateServer`、`build/StatusServer`、`build/ChatServer`。protobuf 生成代码在构建时由本机 `protoc` 输出到 `build/gen/`，仓库中不提交。

### VerifyServer（Node.js）

```bash
cd VerifyServer
npm install
node server.js              # 监听 0.0.0.0:50052
```

### 客户端

```bash
cd ChatClient
mkdir -p build && cd build
qmake6 ../Chat.pro
make -j4                    # 产物：ChatClient/build/bin/Chat
```

或直接使用启动脚本，它会补齐运行所需的 `config.ini` 与 `static/`：

```bash
scripts/start_client.sh
```

### 数据准备

```bash
mysql -uroot -p < sql/db01.sql                       # 建表 + 存储过程
```

Redis 由 `scripts/start_all.sh` 在 :6380 自动拉起（口令 `123456`，与示例配置一致）；如已在运行则不会重复启动。

### 配置

每个服务都从**自己的工作目录**读取 `config.ini`，因此启动时要 `cd` 到对应目录。`VerifyServer/config.json` 含邮箱授权码与数据源密码，不在版本库中：从 `VerifyServer/config.example.json` 复制并填写。

### 启动

```bash
scripts/start_all.sh        # 按依赖顺序拉起各服务
scripts/stop_all.sh         # 停止
```

## ✅ 测试

后端为 GoogleTest + CTest，客户端为 Qt Test，两者都由 CI 自动执行。

```bash
# 后端：5 个用例集（proto 编解码、URL 编解码、消息头边界、消息节点线序、ChatServer）
cmake -S . -B build && cmake --build build -j4
ctest --test-dir build --output-on-failure

# 客户端：4 个套件（UserMgr、列表分页加载、好友申请、文本消息）
mkdir -p build/client-tests && cd build/client-tests
qmake6 ../../ChatClient/tests/tests.pro && make -j"$(nproc)"
QT_QPA_PLATFORM=offscreen ./client_tests
```

## 🔄 持续集成

`.github/workflows/ci.yml` 在 push 到 master 与所有 PR 上运行两个任务：

- **backend**：安装后端依赖 → CMake 构建 → `ctest`
- **client**：安装 Qt6 → 构建客户端 → 构建测试 → `QT_QPA_PLATFORM=offscreen` 运行测试

## 📚 相关文档

- `WeChat_Project_Architecture.md` — 英文架构文档（模块、协议、关键流程、遗留问题）
- `WeChat项目架构文档.md` — 中文架构文档
