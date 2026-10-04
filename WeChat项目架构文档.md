# Qt 仿微信即时通信软件 — 项目架构文档

> 一个使用 C++ / Qt 构建的跨平台即时通信（IM）系统，后端采用分布式微服务架构，前端为像素级仿微信的桌面客户端。支持登录注册、好友管理、文本消息收发、长连接心跳与断线离线处理。

---

## 1. 总体架构

系统分为**客户端**与**服务端**两大部分。服务端由四个独立进程组成，通过 gRPC 与 Redis/MySQL 协作：

```
                        ┌─────────────────────────────┐
                        │        ChatClient (Qt)       │
                        │   Qt Widgets + QSS 仿微信 UI │
                        └───────┬──────────────┬───────┘
                                │              │
                     HTTP (8080)│              │TCP 长连接 (8090)
                    登录/注册/改密│              │protobuf 自定义帧协议
                                ▼              ▼
                    ┌───────────────┐   ┌──────────────────┐
                    │  GateServer    │   │   ChatServer      │
                    │ Boost.Asio+Beast│  │ Boost.Asio (TCP)  │
                    │ HTTP 网关      │   │ gRPC 服务端(50055)│
                    └──┬────┬────┬───┘   └───┬──────┬────────┘
              gRPC     │    │    │  gRPC     │      │  gRPC(跨服)
          (50052)      │    │    └──────────►│      │
                       ▼    │  (50053)       │      ▼
             ┌──────────────┐│ ┌──────────────┴─┐ ┌──────────────┐
             │ VerifyServer ││ │  StatusServer  │ │ ChatServer 2 │
             │  Node.js     ││ │  gRPC 服务端   │ │  (peer 节点) │
             │  邮件验证码  ││ │  负载均衡/发号 │ │  gRPC(50056) │
             └──────┬───────┘│ └───────┬────────┘ └──────┬───────┘
                    │        │         │                  │
                    └────────┴─────────┴──────────────────┘
                                     │
                          ┌──────────┴──────────┐
                          │   Redis    MySQL    │
                          │ 缓存/会话   持久存储 │
                          └─────────────────────┘
```

**核心设计思想：**

- **接入与业务分离**：GateServer 只做 HTTP 接入和无状态业务（注册/登录鉴权），不持有长连接；ChatServer 专职处理高并发 TCP 长连接与消息路由。
- **状态集中、服务无状态**：登录态（token、uid→server 映射、uid→session 映射）全部放 Redis，任意 ChatServer 均可通过 Redis 定位用户所在节点。
- **gRPC 做服务间通信**：C++ 服务间用 protobuf + gRPC；跨 ChatServer 的消息投递也走 gRPC，实现水平扩展。
- **连接池化**：Redis、MySQL、gRPC Stub 均实现连接池 + 后台保活线程，避免频繁建连。

---

## 2. 技术栈

| 层次 | 技术 |
|------|------|
| 客户端 UI | C++、Qt Widgets、QSS、QPainter、自定义气泡控件 |
| 客户端网络 | QtNetwork（QNetworkAccessManager 做 HTTP，QTcpSocket 做长连接） |
| 服务端网络 | Boost.Asio、Boost.Beast（HTTP）、epoll |
| 服务间通信 | gRPC + Protocol Buffers |
| 数据序列化 | JSON（客户端↔ChatServer、GateServer 内部）、protobuf（服务间） |
| 缓存/存储 | Redis（hiredis / ioredis）、MySQL（MySQL Connector/C++） |
| 辅助库 | jsoncpp、boost::property_tree（ini 解析）、nodemailer |
| 运行环境 | Linux（服务端）、跨平台（客户端） |

---

## 3. 服务端模块详解

### 3.1 GateServer — HTTP 接入网关（C++）

**职责**：面向客户端唯一 HTTP 入口，处理注册、登录、密码重置、验证码请求。

- **入口**：`GateServer/GateServer.cpp:125`。读取 `[GateServer] Port`（默认 8080），创建 `CServer` 监听，注册 SIGINT/SIGTERM 优雅退出。
- **HTTP 层**：`HttpConnection.cpp` 基于 Boost.Beast 异步读写。支持 GET（解析 query 参数）与 POST（读取 body）。内置 `deadline_` 定时器，连接空闲 60 秒自动关闭。
- **并发模型**：`AsioIOServicePool` 负责把新连接轮询分配到多个 `io_context` 线程，主 `io_context` 只做 accept。
- **路由表**（`LogicSystem`，GET 用 `_get_handlers`、POST 用 `_post_handlers`）：

  | 方法 | 路径 | 处理逻辑 |
  |------|------|----------|
  | GET | `/get_test` | 连通性测试 |
  | POST | `/get_verify_code` | 转 `VerifyGrpcClient` 请求验证码邮件 |
  | POST | `/user_register` | 校验 Redis 中 `code_<email>`，调用 `MysqlMgr::RegUser` 建号 |
  | POST | `/reset_pwd` | 校验验证码 + `CheckEmail`，调用 `UpdatePwd` |
  | POST | `/user_login` | `CheckPwd` 校验密码 → `StatusGrpcClient::GetChatServer(uid)` 取 token 与目标 ChatServer 地址 → 返回 host/port/token |

- **依赖的下游**：VerifyServer（gRPC 50052）、StatusServer（gRPC 50053）、MySQL、Redis。
- **配置** `GateServer/config.ini`：`[GateServer] Port`、`[VerifyServer]`、`[StatusServer]`、`[Mysql]`、`[Redis]`。

### 3.2 StatusServer — 状态/调度服务（C++）

**职责**：登录 token 发号 + ChatServer 负载均衡选点。

- **入口**：`StatusServer/StatusServer.cpp:16`，gRPC 监听配置的 `[StatusServer] Host:Port`（50053）。
- **服务实现** `StatusServiceImpl.cpp`：
  - `GetChatServer(uid)`：
    1. 从配置的服务器列表（`cfg["chatservers"]["Name"]`）中，读取 Redis Hash `logincount` 各节点当前在线数；
    2. 选出**在线数最少**的 ChatServer（首次无数据按 `INT_MAX` 处理）；
    3. 生成 UUID token，写入 Redis `utoken_<uid>`；
    4. 返回 `host / port / token`。
  - `Login(uid, token)`：比对 Redis `utoken_<uid>`，供 ChatServer 二次校验。
- **配置**：需读取 `[chatservers] Name = chatserver1,chatserver2` 及每个节点的 Host/Port。仓库现已提供 `StatusServer/config.ini` 示例，部署时按实际环境调整。

### 3.3 VerifyServer — 邮件验证码服务（Node.js）

**职责**：生成验证码 → 存 Redis（600 秒过期）→ 通过 163 邮箱 SMTP 发送。

- **入口**：`VerifyServer/server.js`，gRPC 监听 `0.0.0.0:50052`（与 config.ini 及 C++ 侧一致）。
- **逻辑**：若 Redis 已有 `code_<email>` 则复用，否则用 uuidv4 截取 4 位，`SetRedisExpire(key, code, 600)`，再调用 `email.js`（nodemailer，`smtp.163.com:465` 加密发送）。
- **`redis.js`**：基于 ioredis，带 10 秒心跳保活，封装 `GetRedis / QueryRedis / SetRedisExpire`。
- **配置** `VerifyServer/config.json`：`email.user/pass`、`redis.host/port/passwd`（含 `mysql` 段但当前代码未使用）。

### 3.4 ChatServer — 长连接与消息服务（C++，核心）

**职责**：承载客户端 TCP 长连接，处理登录、好友申请、消息收发与心跳，跨节点消息经 gRPC 转发。

**入口**：`ChatServer/ChatServer.cpp:78`

1. `ConfigMgr::Inst()` 读取 `[SelfServer]`（Name/Host/Port/RPCPort）；
2. `RedisMgr` 初始化并把自己的在线数 `logincount[Name]` 置 0，退出时用 `Defer` 清理；
3. 启动 `CServer`（TCP，默认 8090）并开启心跳巡检定时器；
4. 启动 gRPC 服务端（`ChatServiceImpl`，默认 50055）于独立线程；
5. `LogicSystem` 绑定 `CServer`，进入 `io_context.run()`。

**关键类：**

| 类 | 文件 | 作用 |
|----|------|------|
| `CServer` | CServer.cpp/h | TCP acceptor，管理 `_sessions`（session_id→session），60 秒定时巡检心跳过期 |
| `CSession` | CSession.cpp/h | 单条连接：异步收发、消息帧解析、发送队列、心跳时间戳、异常会话清理 |
| `LogicSystem` | LogicSystem.cpp/h | 单线程消息消费队列 + 消息 ID → 回调分发；所有业务处理加锁串行化 |
| `UserMgr` | UserMgr.cpp/h | 单例，`uid → session` 映射，供跨会话推送 |
| `ChatServiceImpl` | ChatServiceImpl.cpp/h | gRPC 服务端：跨节点通知加好友/认证/文本消息/踢人 |
| `ChatGrpcClient` | ChatGrpcClient.cpp/h | gRPC 客户端，按目标服务器地址维护 Stub 连接池 |
| `AsioIOServicePool` | AsioIOServicePool.cpp/h | `io_context` 线程池，连接轮询分配 |
| `RedisMgr` | RedisMgr.cpp/h | hiredis 连接池 + 60 秒 PING 保活 + 分布式锁 |
| `MysqlMgr` / `MysqlDao` | MysqlMgr/MysqlDao | MySQL 连接池（60 秒 `SELECT 1` 保活）+ 用户/好友 DAO |
| `DistLock` | DistLock.cpp/h | 基于 Redis 的分布式锁（setnx + 过期 + 唯一标识） |

**业务回调（`LogicSystem::RegisterCallBacks`, LogicSystem.cpp:90）：**

| 消息 ID | 常量 | 处理函数 | 说明 |
|---------|------|----------|------|
| 1005 | `MSG_CHAT_LOGIN` | `LoginHandler` | token 校验、加载用户与好友/申请列表、会话绑定、顶号处理 |
| 1007 | `ID_SEARCH_USER_REQ` | `SearchInfo` | 按 uid（纯数字）或用户名搜索用户 |
| 1009 | `ID_ADD_FRIEND_REQ` | `AddFriendApply` | 写申请、定位对方服务器并通知 |
| 1013 | `ID_AUTH_FRIEND_REQ` | `AuthFriendApply` | 同意好友、写双向好友关系、通知对方 |
| 1017 | `ID_TEXT_CHAT_MSG_REQ` | `DealChatTextMsg` | 校验目标在线，转发文本消息 |
| 1023 | `ID_HEART_BEAT_REQ` | `HeartBeatHandler` | 回心跳应答 |

**消息路由核心逻辑**：处理任意消息时，先用 Redis `uip_<touid>` 查目标用户所在 ChatServer 名。若等于本机名，直接经 `UserMgr` 取 session 推送；否则调用 `ChatGrpcClient` 走 gRPC 转发到目标节点，由对端 `ChatServiceImpl` 落入其本地 session。

**心跳与离线**：每 60 秒巡检所有 session，`_last_heartbeat` 超过 60 秒判为过期 → `Close()` → `DealExceptionSesseion()`。后者用**分布式锁** `lock_<uid>` 保护，校验 Redis `usession_<uid>` 是否仍等于本 session（防误删新登录会话），再清理 `usession_`/`uip_` 键并移除会话。

---

## 4. 客户端模块详解（ChatClient）

### 4.1 入口与窗口流

`main.cpp` 加载 QSS 样式、读取 `config.ini` 拼出 `gate_url_prefix`，构造 `MainWindow`。`MainWindow` 以状态机切换中央控件：

```
LoginDialog ⇄ RegistDialog / ResetDialog ──登录成功──► ChatDialog（主界面，≥1050x900）
```

`ChatDialog` 为用户登录后的 IM 主壳（聊天列表 / 联系人 / 搜索 / 聊天页 / 心跳）。

### 4.2 网络层

- **HTTP（`httpmgr.cpp`）**：`QNetworkAccessManager` 异步 POST JSON，完成后发 `sig_http_finish`，按模块细分路由到注册/重置/登录信号。请求地址：`/get_verify_code`、`/user_register`、`/reset_pwd`、`/user_login`。
- **TCP 长连接（`tcpmgr.cpp`）**：
  - **帧格式**：4 字节头 = 2 字节消息 ID + 2 字节长度（大端），后接 JSON body；用 `QDataStream` 收发，半包用 `_b_recv_pending` 缓存。
  - **分发**：`_handlers` 按消息 ID 映射到槽函数（`HandleMsg`）。
  - **心跳**：由 `ChatDialog` 每 10 秒发 `ID_HEART_BEAT_REQ`。
  - **断线**：不自动重连，`disconnected` → 提示并强制回登录页。
  - 连接参数（Host/Port/Token/Uid）来自登录 HTTP 响应。

### 4.3 应用状态

`usermgr.cpp`（单例）缓存 `_user_info`、`_apply_list`、`_friend_list`、`_friend_map`、`_token`，并提供聊天/联系人列表分页（`CHAT_COUNT_PER_PAGE`）。`userdata.h` 定义纯数据结构：`SearchInfo`、`AddFriendApply`、`ApplyInfo`、`AuthInfo`、`FriendInfo`、`UserInfo`、`TextChatMsg` 等。

### 4.4 主要 UI 类

| 类 | 作用 |
|----|------|
| `LoginDialog` | 登录表单，登录成功后触发 TCP 连接 |
| `RegistDialog` | 邮箱验证码注册（带倒计时） |
| `ResetDialog` | 验证码重置密码 |
| `ChatDialog` | IM 主界面外壳，承载聊天/联系人/搜索/心跳 |
| `ChatPage` | 单个好友的聊天页（历史 + 输入区） |
| `ChatView` / `ChatUserWid` / `ChatUserList` | 聊天会话列表与滚动视图 |
| `ApplyFriend` / `AuthenFriend` / `ApplyFriendPage` | 发起加好友 / 处理好友申请 / 申请列表 |
| `FriendInfoPage` / `ContactUserList` / `ConUserItem` | 好友详情、联系人列表 |
| `SearchList` / `LoadingDlg` / `FindSuccessDlg` / `FindFailDlg` | 搜索结果、加载与结果弹窗 |
| `StateWidget` / `ClickedBtn` / `ClickedLabel` / `TimerBtn` / `FriendLabel` | 自定义美化控件（标签、按钮、倒计时等） |

### 4.5 消息气泡渲染

- `BubbleFrame`：QPainter 绘制圆角矩形 + 小三角尾巴；自己为绿色 `(158,234,106)`，对方为白色。
- `ChatItemBase`：网格布局行（昵称 + 42×42 头像 + 气泡），按 `ChatRole::Self/Other` 左右镜像。
- `TextBubble`：透明只读 QTextEdit，`eventFilter` 动态调整高度与宽度。
- `PictureBubble`：缩放 QLabel（最大 160×90）。
- `ChatView`：QScrollArea + QVBoxLayout，追加消息时自动滚到底部。

### 4.6 工程配置

`Chat.pro`：`QT += core gui network widgets`，`CONFIG += c++11`，`RC_ICONS = icon.ico`，输出到 `./bin`。资源 `rc.qrc`（图标/图片），外部 `config.ini` 与 `style/stylesheet.qss` 构建后拷贝到 `bin`。

---

## 5. 通信协议

### 5.1 客户端 ↔ ChatServer（自定义 TCP 帧）

```
┌────────────┬────────────┬───────────────────────────┐
│ msg_id(2B) │ length(2B) │         body (JSON)        │
└────────────┴────────────┴───────────────────────────┘
        大端序（network byte order），body 最大 2*1024 字节
```

发送队列上限 `MAX_SENDQUE = 1000`，接收队列上限 `MAX_RECVQUE = 10000`。

### 5.2 服务间（gRPC / protobuf）

`message.proto`（ChatServer/GateServer/StatusServer 各持一份，内容一致）定义三个服务：

- `VerifyService.GetVerifyCode(GetVerifyReq) → GetVerifyRsp`
- `StatusService.GetChatServer / Login`
- `ChatService.NotifyAddFriend / RplyAddFriend / SendChatMsg / NotifyAuthFriend / NotifyTextChatMsg / NotifyKickUser`

### 5.3 消息 ID 常量（`ChatServer/const.h:77`）

| ID | 常量 | 方向 |
|----|------|------|
| 1005 / 1006 | `MSG_CHAT_LOGIN` / `_RSP` | 客户端登录 |
| 1007 / 1008 | `ID_SEARCH_USER_REQ` / `_RSP` | 搜索用户 |
| 1009 / 1010 | `ID_ADD_FRIEND_REQ` / `_RSP` | 加好友申请 |
| 1011 | `ID_NOTIFY_ADD_FRIEND_REQ` | 通知收到申请 |
| 1013 / 1014 | `ID_AUTH_FRIEND_REQ` / `_RSP` | 同意好友 |
| 1015 | `ID_NOTIFY_AUTH_FRIEND_REQ` | 通知认证通过 |
| 1017 / 1018 | `ID_TEXT_CHAT_MSG_REQ` / `_RSP` | 发文本消息 |
| 1019 | `ID_NOTIFY_TEXT_CHAT_MSG_REQ` | 通知收到消息 |
| 1021 | `ID_NOTIFY_OFF_LINE_REQ` | 通知离线/被踢 |
| 1023 / 1024 | `ID_HEART_BEAT_REQ` / `ID_HEARTBEAT_RSP` | 心跳 |

### 5.4 错误码（`const.h`）

`0 成功`、`1001 Json解析失败`、`1002 RPC失败`、`1003 验证码过期`、`1004 验证码错误`、`1005 用户已存在`、`1006 密码错误`、`1007 邮箱不匹配`、`1008 密码更新失败`、`1009 密码无效`、`1010 Token失效`、`1011 Uid无效`。

### 5.5 Redis 键约定

| 键 | 含义 |
|----|------|
| `utoken_<uid>` | 登录 token（StatusServer 写，ChatServer 校验） |
| `uip_<uid>` | 用户所在 ChatServer 名（消息路由用） |
| `usession_<uid>` | 用户当前 session id（防顶号误删） |
| `ubaseinfo_<uid>` | 用户基础信息缓存 |
| `nameinfo_<name>` | 用户名→信息缓存（按名搜索） |
| `logincount`（Hash） | 各 ChatServer 在线数（负载均衡） |
| `code_<email>` | 邮箱验证码，TTL 600 秒 |
| `lock_<uid>` / `lockcount` | 分布式锁与计数 |
| `ipcount_` | 已定义但当前未使用 |

---

## 6. 关键业务流程

### 6.1 注册

```
Client ──POST /get_verify_code──► Gate ──gRPC──► VerifyServer ──► 163 SMTP 发信
                                        └─► Redis code_<email> (TTL 600)
Client ──POST /user_register──► Gate ─► 校验 code_<email> ─► MysqlMgr::RegUser ─► 建号
```

### 6.2 登录与建连（核心链路）

```
Client ──POST /user_login──► GateServer
                               ├─ MysqlMgr::CheckPwd(uid, pwd)
                               └─ gRPC StatusServer.GetChatServer(uid)
                                     ├─ 选在线数最少的 ChatServer
                                     ├─ 生成 UUID token → Redis utoken_<uid>
                                     └─ 返回 host/port/token
Client ◄─ {error, uid, token, host, port}
Client ──TCP 连接 ChatServer:8090，发送 MSG_CHAT_LOGIN {uid, token}
ChatServer.LoginHandler
   ├─ 校验 Redis utoken_<uid>
   ├─ 拉取用户信息 + 好友列表 + 申请列表
   ├─ （顶号）若 uip_<uid> 已存在且为本机 → 踢掉旧会话
   ├─ Redis: uip_<uid>=本机名，usession_<uid>=sessionId
   └─ UserMgr: uid → session 绑定，回 MSG_CHAT_LOGIN_RSP
```

### 6.3 发文本消息

```
Client A ──1017 ID_TEXT_CHAT_MSG_REQ──► ChatServer A
   ├─ 查 Redis uip_<B>
   ├─ 同机：UserMgr 取 B 的 session → 直接 Send(1019)
   └─ 跨机：ChatGrpcClient.NotifyTextChatMsg(目标节点)
             └─ ChatServer B 的 ChatServiceImpl → 推给 B 的 session(1019)
   └─ 同时回 A: 1018 RSP
```

加好友申请、认证通过同理，均采用「Redis 定位 → 同机直推 / 跨机 gRPC」模式。

### 6.4 心跳与离线

```
Client 每 10s ──1023 心跳──► ChatServer 回 1024，并刷新 session._last_heartbeat
ChatServer 每 60s 巡检：超过 60s 未心跳 → 关连接 → DealExceptionSesseion
   └─ 分布式锁 lock_<uid> 保护下校验 usession_<uid>，清理路由与在线数
```

---

## 7. 并发模型小结

- **接入层**：`AsioIOServicePool` 多 `io_context` 线程处理 socket 读写；acceptor 单线程。
- **业务层**：`LogicSystem` 单工作线程消费消息队列，天然把并发业务串行化，避免大量加锁。
- **跨服**：gRPC 线程池 + Stub 连接池。
- **共享状态**：`UserMgr`、`CServer::_sessions` 均加锁；跨机状态收敛到 Redis，并用分布式锁保证顶号/清理的一致性。
- **保活**：Redis 连接 60s PING、MySQL 连接 60s `SELECT 1`、HTTP 连接 60s 空闲超时、应用层 60s 心跳巡检。

---

## 8. 端口与配置汇总

| 服务 | 协议 | 端口 | 配置文件 |
|------|------|------|----------|
| GateServer | HTTP | 8080 | `GateServer/config.ini` |
| VerifyServer | gRPC | 50052 | `VerifyServer/config.json` |
| StatusServer | gRPC | 50053 | `StatusServer/config.ini` |
| ChatServer | TCP | 8090 | `ChatServer/config.ini` |
| ChatServer | gRPC | 50055 | 同上 |
| ChatServer2 (peer) | gRPC | 50056 | 同上 |
| MySQL | TCP | 3306 | 各服务 config |
| Redis | TCP | 6380 | 各服务 config |

---

## 9. 构建与运行（环境与指令）

### 9.1 构建环境版本

已在 **Ubuntu 24.04 (noble)** 上验证通过：

| 工具 / 库 | 版本 | apt 包 |
|-----------|------|--------|
| gcc / g++ | 13.3.0 | 系统自带 |
| CMake | 要求 ≥ 3.16，实测 3.28.3 | `cmake` |
| C++ 标准 | C++17 | — |
| protoc | 3.21.12 | `protobuf-compiler` |
| gRPC C++ 插件 | 1.51.1 | `libgrpc++-dev`、`protobuf-compiler-grpc` |
| protobuf 开发库 | 3.21.12 | `libprotobuf-dev` |
| Boost | 1.83.0 | `libboost-all-dev` |
| jsoncpp | 1.9.5 | `libjsoncpp-dev` |
| hiredis | 1.2.0 | `libhiredis-dev` |
| MySQL Connector/C++ | 1.1.12 | `libmysqlcppconn-dev` |
| Node.js / npm | 20.20.2 / 10.8.2 | VerifyServer 用 |
| Qt（仅客户端） | 5.14+ 或 6.x，需 Widgets + Network | `qtbase5-dev` / `qt6-base-dev` |

安装后端依赖：

```bash
sudo apt update
sudo apt install -y cmake build-essential pkg-config \
  libboost-all-dev libprotobuf-dev protobuf-compiler protobuf-compiler-grpc \
  libgrpc++-dev libjsoncpp-dev libhiredis-dev libmysqlcppconn-dev \
  default-libmysqlclient-dev
```

> Debian/Ubuntu 下 MySQL 头文件位于 `/usr/include/mysql_driver.h` 与 `/usr/include/cppconn/`，
> 而源码以 `<jdbc/mysql_driver.h>` 形式包含。根目录 `CMakeLists.txt` 在 configure 阶段生成
> `mysql_shim/jdbc/` 软链接树做兼容，无需改动源码。

### 9.2 构建三个 C++ 服务

```bash
cmake -S . -B build
cmake --build build -j4          # 注意下方内存说明
```

产物为 `build/GateServer`、`build/StatusServer`、`build/ChatServer`。
构建期由 `protoc` + `grpc_cpp_plugin` 重新生成 `message.pb.*` / `message.grpc.pb.*` 到
`build/gen/<service>/`，仓库不再提交预生成的 protobuf 文件。

> **内存说明**：gRPC/protobuf 的单文件编译非常吃内存。在约 8 GB 机器上用
> `-j$(nproc)` 会出现 `cc1plus` 被 OOM 杀死，建议 `-j4`。

### 9.3 构建 Node.js 版 VerifyServer

```bash
cd VerifyServer
npm install
node server.js                    # 监听 0.0.0.0:50052
```

### 9.4 客户端（Qt）

客户端为 qmake 工程：用 Qt Creator 打开 `ChatClient/Chat.pro`，或在 Qt 工具链 shell 中
`qmake && make`。当前 `Chat.pro` 含 Windows 专用的构建后置步骤（`copy` / `xcopy`）和 MSVC 专用
参数，在 Linux/macOS 上构建需先删除这些。**客户端构建未在本 Linux 后端环境验证。**

### 9.5 数据存储

```bash
# MySQL 建库（db01 + reg_user 存储过程）
mysql -uroot -p < sql/db01.sql

# Redis 监听 6380，密码 123456（示例配置的默认值）
redis-server --port 6380 --requirepass 123456
```

### 9.6 启动顺序

按依赖顺序启动。三个 C++ 服务均从**各自的工作目录**读取 `config.ini`，因此需在服务目录内启动：

```bash
(cd StatusServer && ../build/StatusServer)     # gRPC 50053
(cd ChatServer   && ../build/ChatServer)       # TCP 8090 + gRPC 50055
(cd GateServer   && ../build/GateServer)       # HTTP 8080
```

### 9.7 冒烟测试

```bash
# 1. HTTP 连通性
curl "http://127.0.0.1:8080/get_test?foo=bar"

# 2. 注册。/get_verify_code 会真的发邮件，本地测试直接往 Redis 注入验证码：
redis-cli -p 6380 -a 123456 SET "code_me@example.com" 123456 EX 300
curl -X POST http://127.0.0.1:8080/user_register -H 'Content-Type: application/json' \
  -d '{"user":"me","email":"me@example.com","passwd":"123456","confirm":"123456","verifycode":"123456","icon":""}'

# 3. 登录（以 email 为键）——返回 token 与目标 ChatServer 的 host/port
curl -X POST http://127.0.0.1:8080/user_login -H 'Content-Type: application/json' \
  -d '{"email":"me@example.com","passwd":"123456"}'
```

登录成功返回 `{"error":0,"uid":..,"token":"..","host":"127.0.0.1","port":"8090"}`，
即验证了 GateServer → MySQL → StatusServer(gRPC) → Redis 整条链路。

---

## 10. 已修复问题与遗留注意事项

### 10.1 已修复（本次改动，分支 `dev_xh`）

1. **GateServer/LogicSystem.cpp 内容错误**：原文件实为 ChatServer 的 TCP 版 LogicSystem 拷贝，与 `LogicSystem.h` 声明的 HTTP 路由接口不匹配、无法编译。已从历史提交 `e434a92` 恢复正确的 HTTP 实现（`/get_test`、`/get_verify_code`、`/user_register`、`/reset_pwd`、`/user_login`），第 3.1 节路由表即按此描述。

2. **VerifyServer 服务名拼写不一致**：原 `message.proto` 服务名/方法为 `VarifyService.GetVarifyCode`（拼写错误），与 C++ 侧 `VerifyService.GetVerifyCode` 不符。已将 proto 与 `server.js` 统一为 `VerifyService.GetVerifyCode`，并验证 proto 可正常加载。

3. **VerifyServer 监听端口不一致**：`server.js` 绑定端口由 50051 改为 50052，与 `config.ini` 及 C++ 客户端一致。

4. **StatusServer 缺少配置文件**：已新增 `StatusServer/config.ini`（含 `[StatusServer]`、`[Mysql]`、`[Redis]`、`[chatservers]` 及节点段），按实际环境调整即可。

5. **ChatServer 缺少 ConfigMgr.cpp**：`ConfigMgr.h` 声明了构造函数与 `GetValue` 但目录内无实现（链接期未定义引用）。已补充 `ConfigMgr.cpp`，并修复 `operator=` 缺失 `return *this;` 的问题。

6. **仓库无构建系统**：原工程依赖 Windows/VS 手工配置。已新增根目录 `CMakeLists.txt`，一键构建 GateServer / StatusServer / ChatServer 三个后端服务（VerifyServer 用 npm 管理）。

### 10.2 遗留注意

- **硬编码的敏感信息**：`VerifyServer/config.json` 含真实邮箱账号/授权码及远程 MySQL/Redis 地址；`ChatServer/config.ini` 等含明文 Redis 密码。提交或部署前应改用环境变量或占位符。
- **客户端无自动重连**：断线后直接回登录页，未实现指数退避重连。
- **`ipcount_` 键**在 `const.h` 中定义但无任何使用，疑为历史遗留。
- **proto 生成代码版本**：各服务的 `message.pb.cc/.h` 为预生成产物，若本机 protoc 版本差异较大，建议用 `protoc` + `grpc_cpp_plugin` 重新生成。

---

## 11. 目录结构速览

```
WeChat/
├── ChatClient/      Qt 客户端（UI / 气泡控件 / TCP与HTTP 网络 / 样式资源）
├── GateServer/      HTTP 网关（注册/登录/改密，Boost.Beast）
├── StatusServer/    状态与调度（token 发号、负载均衡选点）
├── VerifyServer/    Node.js 邮件验证码服务
├── ChatServer/      长连接与消息服务（TCP + gRPC + Redis/MySQL）
├── CMakeLists.txt   后端构建脚本（三个 C++ 服务）
├── README.md
├── WeChat项目架构文档.md（中文版，本文档）
└── WeChat_Project_Architecture.md（英文版）
```

各服务目录中还包含 `.drawio` 架构/时序图文件，可用 draw.io 打开查看设计图。

---

## 12. 教程进度对照（gitbookcpp.llfc.club day01–day45）

本项目按教程逐日实现，当前进度跟进到 **day35**，另已提前完成 day32（分布式锁）与 day41（Qt 粘包处理）。day30 为面试技巧，无代码产出。

`[x]` 已实现 / `[ ]` 未实现：

```
[x] day01-05  框架概述/单例与http管理/boost配置/beast http/post+json   → GateServer 全套
[x] day06-07  grpc 编译与配置                                          → message.proto + gRPC
[x] day08-10  node邮箱认证/redis/多服务验证码派发                      → VerifyServer + Gate
[x] day11-13  注册/注册界面/重置界面                                   → regist/reset dialog
[x] day14     登录和状态服务                                           → StatusServer
[x] day15-17  客户端Tcp管理/asio tcp服务/登录验证与客户端数据管理      → tcpmgr + ChatServer
[x] day18-26  聊天主界面/搜索/列表/滚动/气泡/侧边栏/好友申请           → ChatDialog 全套 UI
[x] day27-29  分布式聊天服务/好友查询·申请·认证·通信                   → ChatGrpcClient 跨服
[x] day32     分布式锁                                                 → DistLock
[x] day33-35  单服踢人/跨服踢人/心跳检测                               → NotifyKickUser + HeartBeat
[ ] day31     文件传输                                                 → 缺文件传输协议
[ ] day36     Qt头像裁剪                                               → 缺裁剪对话框
[ ] day37     聊天信息存储方案                                         → MySQL 仅存好友关系，无聊天记录表
[ ] day38     断点续传
[ ] day39     多媒体信息传输（语音/视频消息）
[ ] day40     分布式事务死锁分析
[x] day41     Qt粘包分析和解决                                         → tcpmgr _b_recv_pending
[ ] day42     通知客户端异步下载聊天图片
[ ] day43     用户加载聊天资源
[ ] day44     webrtc coturn 服务搭建
[ ] day45     webrtc 信令服务器实现视频通信
```

**后续待办顺序**：`day31 文件传输` → `day36 头像裁剪` → `day37 聊天记录持久化` → `day38 断点续传` → `day39 多媒体` → `day40 分布式事务` → `day42-43 图片异步下载/聊天资源` → `day44-45 WebRTC 音视频`。

> 注意：day37 是分水岭。当前消息不落库、纯在线转发；做多媒体与视频前需先补齐聊天记录存储。

---

## 13. 功能验证与 Mock 账户（standard.txt 第 6 条）

### 13.1 Mock 账户：绕开真实邮箱验证码

注册链路（`/get_verify_code` → 邮件 → `/user_register`）依赖真实邮箱，本地无法闭环。为完整验证功能，新增 `scripts/mock_accounts.sh`：

```bash
scripts/mock_accounts.sh [count]     # 默认 5，生成 mock1..mock5
```

- 直接调用存储过程 `reg_user` 建号，幂等（已存在则复用），密码统一 `123456`。
- 同时往 Redis 写入 `code_<email> = 1234`（EX 600），使 `/user_register` 也能用 `verifycode=1234` 走通完整注册链路，无需真的收邮件。
- 当前已建：mock1=uid2、mock2=uid5、mock3=uid6、mock4=uid7、mock5=uid8、mock6=uid9。

### 13.2 端到端功能验证

`scripts/tcp_e2e.py`（`uv run scripts/tcp_e2e.py`）用裸 TCP 走完整协议，要求服务已启动 + mock 账户已建：

| 步骤 | 消息 ID | 断言 |
|------|---------|------|
| TCP 登录 | 1005/1006 | `error=0`，回包 `name` 正确 |
| 按名搜索用户 | 1007/1008 | 命中对方 uid + name |
| 加好友申请 | 1009/1010 + 通知 1011 | 申请方 `error=0`，对方收到 `applyuid` |
| 好友认证 | 1013/1014 + 通知 1015 | 认证方 `error=0`，申请方收到 `fromuid` |
| 文本聊天 | 1017/1018 + 通知 1019 | 发送方 `error=0`，对方收到 `content` |

脚本特意让 mock1 先登录、再去取 mock2 的渲染服务器，使二者落在**不同 ChatServer**（打印 `cross-node: True`），从而真正走通跨节点 gRPC 通知路径。最终结果 **11/11 通过**。

### 13.3 本轮验证发现并修复的问题

1. **MySQL 排序规则不一致导致注册静默失败**：`db01.sql` 建库用 `COLLATE utf8mb4_unicode_ci`，而 InnoDB 表默认 `utf8mb4_0900_ai_ci`；存储过程入参继承库级排序规则，`CALL reg_user(...)` 报 `ERROR 1267 Illegal mix of collations`，被 `EXIT HANDLER` 吞掉后返回 -1。修复：`sql/db01.sql` 去掉库级 `COLLATE`，并 `ALTER DATABASE db01 COLLATE utf8mb4_0900_ai_ci`，重建 `reg_user`。注册、登录随即恢复。

2. **连接池事务状态泄漏（autocommit 未复位）**：`ChatServer/MysqlDao.cpp` 的 `AddFriend` 与 `GateServer/MysqlDao.cpp` 的 `RegUserTransaction` 都调用了 `setAutoCommit(false)` 开启事务，但归还连接到池之前从未复位。连接被 `Defer` 原样放回后仍处手动提交模式，下一个借用者会落在一个陈旧未提交事务里、拿锁阻塞——表现为 TCP E2E 的「好友认证」偶发卡死。修复：在两个 `Defer` 归还连接前加 `setAutoCommit(true)`（含异常保护）。

3. **负载均衡计数从不更新（`logincount` 只在 60s 定时器里刷新）**：`RedisMgr::IncreaseCount/DecreaseCount` 有定义但**全工程无任何调用**，`logincount` 仅由每 60s 的巡检定时器写入实际在线数。服务刚起的头一分钟内所有客户端读到的计数都是 0，StatusServer 的「最少连接」退化成永远选 `unordered_map` 里的第一个节点——所有客户端挤到同一台 ChatServer，跨节点 gRPC 路径根本走不到。修复：登录成功处 `LogicSystem::LoginHandler` 调用 `IncreaseCount`，会话清理处 `CServer::ClearSession` 对已登录会话 `DecreaseCount`（定时器继续做绝对值对账）。修复后 E2E 中两名用户稳定分散到 8090/8091，`cross-node: True`。

4. **Redis 未被纳入启动脚本**：Redis（:6380）是 ChatServer 与 VerifyServer 的硬依赖，但没有任何脚本启动它。Redis 不在时整套服务「看似起来了」实则半死：ChatServer 阻塞在连接、VerifyServer 直接崩溃、只剩 Gate+Status。修复：`start_all.sh` 在启动前探测 6380，未响应则拉起本地实例（端口/密码取自各配置）；`stop_all.sh` 会一并关停。

5. **重启时 acceptor 报 `Address already in use`**：ChatServer/GateServer 的 `CServer` 在构造函数初始化列表里直接 bind 端口，没机会设置 `SO_REUSEADDR`。快速重启时上一实例残留的连接会让新 bind 失败（表现为 chatserver2 起不来，`:8091` 拒绝连接）。修复：改为显式 `open → set_option(reuse_address) → bind → listen`。

### 13.4 需要 sudo 的指令（暂未执行，请手动运行）

```bash
# 安装 DBeaver（MySQL/Redis 图形客户端；.deb 已下载到 /tmp/dbeaver-ce.deb）
sudo apt install -y /tmp/dbeaver-ce.deb

# 若缺少系统库（Qt6 运行/构建、Boost、hiredis、mysql-client 等按报错补齐）
sudo apt install -y libboost-all-dev libhiredis-dev default-libmysqlclient-dev \
                    mysql-client redis-tools qt6-base-dev qt6-base-dev-tools
```

### 13.5 Qt UI 美化：可复用的框架/主题

当前 UI 是 Qt Widgets + QSS（`ChatClient/resource/stylesheet.qss`），改主题只需替换 QSS，不必重写控件。可选方案：

- **QDarkStyleSheet**（最省事）：成熟的 Qt Widgets 暗色主题，`pip install qdarkstyle` 或直接取 `qdarkstyle/style.qss`，一行 `app.setStyleSheet(qdarkstyle.load_stylesheet())` 即可换肤，与本项目 QSS 机制完全兼容。
- **Qt-Material**：Material Design 主题（`qt-material`），支持明/暗与多主色，同样面向 Qt Widgets。
- **QFluentWidgets / ElaWidgetTools**：现代 Win11/云母风格组件库，观感最好但需替换部分控件基类，改动量最大。
- **手改 QSS**：在现有 `stylesheet.qss` 基础上重配色/圆角/间距，零依赖，适合只做视觉微调。

**编辑方式**：`.ui` 文件是纯 XML，在 WSL 里用任意编辑器改或在 Qt Creator 的 Design 模式改都行——两者等价，Design 模式只是可视化预览。QSS 本身任何编辑器可改，改完直接在 WSL 里以 `./scripts/start_client.sh` 起客户端看效果（WSLg 可直接出窗口）。**建议**先上 QDarkStyleSheet 快速换肤，再按需局部调 QSS。
