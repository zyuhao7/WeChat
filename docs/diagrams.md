# 架构与流程图（Mermaid）

本文件集中维护项目的架构图与关键流程图，取代各服务目录下原先的 `.drawio` 文件。所有图均为 Mermaid 文本，GitHub、GitLab、VS Code Markdown 预览可直接渲染，也可用 `npx @mermaid-js/mermaid-cli` 导出 SVG/PNG 用于汇报。

图中的消息 ID、Redis 键名与超时时间以代码为准；改动代码时请同步更新对应图块。

## 1. 总体架构

```mermaid
flowchart TB
    Client["ChatClient (Qt)<br/>Qt Widgets + QSS 仿微信 UI"]
    subgraph Gate["GateServer · HTTP 网关 :8080"]
        direction LR
        CServer["CServer / HttpConnection"] --> Logic["LogicSystem<br/>路由 + 鉴权"]
    end
    subgraph Verify["VerifyServer · Node.js :50052"]
        V["验证码生成 + SMTP"]
    end
    subgraph Status["StatusServer · C++ :50053"]
        S["选服 + token 发号"]
    end
    subgraph Chat1["ChatServer 1 · TCP :8090 / gRPC :50055"]
        CS1["CSession + LogicSystem + UserMgr"]
    end
    subgraph Chat2["ChatServer 2 · 对等节点 :50056"]
        CS2["CSession + LogicSystem + UserMgr"]
    end
    Redis[("Redis<br/>token · 路由 · 在线数")]
    MySQL[("MySQL<br/>用户 · 好友 · 聊天记录")]

    Client -->|HTTP| CServer
    Client -->|TCP 长连接| CS1
    Logic -->|gRPC| V
    Logic -->|gRPC| S
    Logic --> MySQL
    Logic --> Redis
    V --> Redis
    S --> Redis
    CS1 --> Redis
    CS1 --> MySQL
    CS1 <-->|gRPC 跨服转发| CS2
    CS2 --> Redis
    CS2 --> MySQL
```

## 2. GateServer：HTTP 接入

```mermaid
flowchart LR
    Client["Client"] -->|HTTP GET/POST| CServer["CServer<br/>accept"]
    CServer --> Conn["HttpConnection<br/>异步读写 · 60s 空闲超时"]
    Conn -->|HandleGet / HandlePost| Logic["LogicSystem<br/>路由表"]
    Logic -->|/user_register · /reset_pwd · /user_login| MySQL[("MySQL")]
    Logic -->|"校验 / 消费 code_{email}"| Redis[("Redis")]
    Logic -->|/get_verify_code| VG["VerifyGrpcClient"] -->|gRPC :50052| VS["VerifyServer"]
    Logic -->|/user_login| SG["StatusGrpcClient"] -->|gRPC :50053| SS["StatusServer"]
    Conn -->|WriteResponse · shutdown_send| Client
```

## 3. StatusServer：选服与发号

```mermaid
sequenceDiagram
    autonumber
    participant G as GateServer
    participant S as StatusServer
    participant R as Redis
    G->>S: gRPC GetChatServer(uid)
    loop 遍历配置的 ChatServer
        S->>R: HGET logincount {server_name}
        R-->>S: 在线数，空值视为已下线
    end
    Note over S: PickServer 选在线数最少者
    S->>R: SETEX utoken_{uid} 600 UUID token
    S-->>G: host + port + token
    Note over S,G: Login(uid, token) RPC 保留，但网关不调用；ChatServer 直接读 Redis 校验
```

## 4. VerifyServer：验证码

```mermaid
sequenceDiagram
    autonumber
    participant G as GateServer
    participant V as VerifyServer
    participant R as Redis
    participant E as SMTP / 用户邮箱
    G->>V: gRPC GetVerifyCode(email)
    alt 邮箱格式非法
        V-->>G: InvalidEmail (3)
    else 格式合法
        V->>R: SET verify_cooldown_{email} 1 EX 60 NX
        alt 名额已被占用
            V-->>G: TooFrequent (4)
        else 抢到名额
            V->>R: GET code_{email}
            Note over V: 有旧码则复用，否则生成 6 位随机码
            V->>R: SET code_{email} 6位码 EX 600
            V->>E: SMTP 发送验证码
            V-->>G: Success (0)
        end
    end
    Note over G,R: 注册/重置时 GateServer 自己 GET code_{email} 比对，成功后 DEL 消费
```

## 5. 客户端窗口流转

```mermaid
flowchart LR
    L["LoginDialog"] -->|switchRegister| RG["RegistDialog<br/>注册"]
    L -->|switchReset| RS["ResetDialog<br/>重置密码"]
    RG -->|sigSwitchLogin| L
    RS -->|switchLogin| L
    L -->|MSG_CHAT_LOGIN_RSP 成功<br/>TcpMgr::sig_swich_chatdlg| CD["ChatDialog<br/>主界面 ≥1050×900"]
    CD --> CP["ChatPage"]
    CD --> FI["FriendInfoPage"]
    CD --> AF["ApplyFriendPage"]
    CD --> AU["AuthenFriend"]
    CD --> AV["CropDialog"]
```

## 6. 消息气泡布局

```mermaid
flowchart TB
    subgraph Grid["ChatItemBase · QGridLayout（Self 时左右镜像）"]
        direction LR
        Spacer["列 0<br/>spacer"]
        Name["第 0 行 · 第 1 列<br/>m_pNameLabel"]
        Icon["第 0-1 行 · 第 2 列<br/>m_pIconLabel 42×42"]
        Bubble["第 1 行 · 第 1 列<br/>m_pBubble"]
    end
    Spacer --- Name
    Name --- Bubble
    Bubble -->|setMessageWidget| Frame["BubbleFrame<br/>圆角矩形 + 三角尾巴"]
    Frame --> Text["TextBubble<br/>只读 QTextEdit"]
    Frame --> Pic["PictureBubble<br/>QLabel 最大 160×90"]
```

## 7. 登录与建连

```mermaid
sequenceDiagram
    autonumber
    participant C as Client
    participant G as GateServer
    participant M as MySQL
    participant S as StatusServer
    participant R as Redis
    participant CS as ChatServer

    C->>G: POST /user_login (email + XOR 密码)
    G->>M: CheckPwd(email, pwd)
    M-->>G: uid
    G->>S: gRPC GetChatServer(uid)
    S->>R: HGET logincount (遍历所有 ChatServer)
    R-->>S: 各服在线数
    Note over S: PickServer 选在线数最少者，无计数视为已下线
    S->>R: SETEX utoken_{uid} 600 (UUID token)
    S-->>G: host + port + token
    G-->>C: {error, uid, token, host, port}

    C->>CS: TCP 连接 8090，MSG_CHAT_LOGIN (1005) {uid, token}
    CS->>R: GET utoken_{uid}
    R-->>CS: token
    alt token 不匹配
        CS-->>C: MSG_CHAT_LOGIN_RSP (1006) 错误
    else token 匹配
        CS->>M: LoadUserInfoByUid + 好友列表 + 申请列表
        M-->>CS: 用户信息
        CS->>R: SET uip_{uid} = 本机名，usession_{uid} = sessionId
        Note over CS: uip_{uid} 已是本机旧会话时，先下发 ID_NOTIFY_OFF_LINE_REQ (1021) 顶号
        CS->>R: HINCR logincount (本机名)
        CS-->>C: MSG_CHAT_LOGIN_RSP (1006) 成功
    end
```

## 8. 加好友与通过验证

```mermaid
sequenceDiagram
    autonumber
    participant A as Client A
    participant SA as ChatServer A
    participant R as Redis
    participant M as MySQL
    participant SB as ChatServer B
    participant B as Client B

    A->>SA: ID_ADD_FRIEND_REQ (1009) {touid}
    SA->>M: AddFriendApply(uid, touid)
    SA->>R: GET uip_{touid}
    R-->>SA: 对方服务器名
    alt 同服
        SA-->>B: ID_NOTIFY_ADD_FRIEND_REQ (1011)
    else 跨服
        SA->>SB: gRPC NotifyAddFriend(AddFriendReq)
        SB-->>B: ID_NOTIFY_ADD_FRIEND_REQ (1011)
    end
    SA-->>A: ID_ADD_FRIEND_RSP (1010)
    Note over SB,B: 对方离线时找不到 session，通知丢弃，但申请已落库
    B->>SB: ID_AUTH_FRIEND_REQ (1013) {touid = A}
    SB->>M: AuthFriendApply + AddFriend 双向写库
    SB->>R: GET uip_{fromuid}
    R-->>SB: 申请人服务器名
    alt 同服
        SB-->>A: ID_NOTIFY_AUTH_FRIEND_REQ (1015)
    else 跨服
        SB->>SA: gRPC NotifyAuthFriend(AuthFriendReq)
        SA-->>A: ID_NOTIFY_AUTH_FRIEND_REQ (1015)
    end
    SB-->>B: ID_AUTH_FRIEND_RSP (1014)
```

## 9. 登录 + 加好友全景（跨服）

```mermaid
flowchart TB
    subgraph Login["登录与建连"]
        direction TB
        L1["Client POST /user_login"] --> L2["GateServer CheckPwd"]
        L2 --> L3["StatusServer 选服<br/>SETEX utoken_{uid} 600"]
        L3 --> L4["Client TCP 1005 {uid, token}"]
        L4 --> L5["ChatServer GET utoken_{uid} 校验"]
        L5 --> L6["SET uip_{uid} · HINCR logincount"]
        L6 --> L7["1006 登录成功"]
    end

    subgraph Friend["加好友与审批"]
        direction TB
        F1["A 1009 {touid}"] --> F2["AddFriendApply 落库"]
        F2 --> F3["GET uip_{touid} 定位 B 所在服"]
        F3 -->|同服| F4["1011 直推 B"]
        F3 -->|跨服| F5["gRPC NotifyAddFriend"] --> F4
        F4 --> F6["A 收回 1010"]
        F6 --> F7["B 1013 审批"]
        F7 --> F8["AuthFriendApply + AddFriend<br/>双向写库"]
        F8 --> F9["GET uip_{fromuid} 定位 A 所在服"]
        F9 -->|同服| F10["1015 直推 A"]
        F9 -->|跨服| F11["gRPC NotifyAuthFriend"] --> F10
        F10 --> F12["B 收回 1014"]
    end

    L7 --> F1
```

## 10. 会话生命周期

```mermaid
stateDiagram-v2
    [*] --> Authenticating: TCP 连接 8090
    Authenticating --> Online: 1005 且 utoken_{uid} 匹配
    Authenticating --> Closed: token 不匹配 / 解析失败
    Online --> Online: 1023 心跳 · 收发消息
    Online --> Online: 同服重复登录，旧连接收 1021 后被替换
    Online --> Closed: 300s 无任何消息
    Online --> Closed: 客户端断开 / 服务停止
    Closed --> [*]
    note right of Online: 跨服踢人尚未实现，1021 只发给本机旧会话
```

## 11. 分布式锁

```mermaid
sequenceDiagram
    autonumber
    participant W as ChatServer 业务
    participant R as Redis
    W->>R: SET lock:lock_{uid} UUID NX EX 10
    alt 抢锁成功
        R-->>W: OK
        Note over W: 临界区<br/>读写 uip_{uid} / usession_{uid} / logincount
        W->>R: EVAL 比对 UUID 后 DEL
        R-->>W: 1 成功 / 0 已过期或易主
    else 已被占用
        R-->>W: nil
        Note over W: sleep 1ms 后重试，最多 5s
    end
    Note over W,R: DistLock 自动加 lock: 前缀，实际键为 lock:lock_{uid}；Defer 保证释放
```

## 12. 聊天记录分页

```mermaid
sequenceDiagram
    autonumber
    participant U as ChatDialog
    participant CS as ChatServer
    participant M as MySQL
    U->>CS: 1028 {touid, limit 50}（首次无 last_id）
    CS->>M: GetChatHistory(uid, touid, before_id=0, limit≤50)
    M-->>CS: 升序消息 + has_more
    Note over CS: 按 24KB 预算从新到旧裁剪，溢出则置 has_more
    CS-->>U: 1029 {touid, text_array, has_more}
    U->>U: 按 msgid 去重 · 按 seq 排序 · 滚到底部
    U->>CS: 滚动到顶部：1028 {touid, last_id=当前最旧 seq}
    CS->>M: GetChatHistory(before_id=last_id)
    M-->>CS: 更旧的一页
    CS-->>U: 1029，继续向上拼接
```

## 13. 文件分片转发

```mermaid
sequenceDiagram
    autonumber
    participant A as Client A
    participant SA as ChatServer A
    participant R as Redis
    participant SB as ChatServer B
    participant B as Client B
    A->>SA: ID_FILE_MSG_REQ (1025) {touid, fileid, index, total, data}
    SA->>R: GET uip_{touid}
    R-->>SA: 对方服务器名
    alt 同服且在线
        SA->>B: ID_NOTIFY_FILE_MSG_REQ (1027)
    else 跨服
        SA->>SB: gRPC NotifyFileMsg(FileMsgReq)
        SB->>B: ID_NOTIFY_FILE_MSG_REQ (1027)
    end
    SA-->>A: ID_FILE_MSG_RSP (1026) {fileid, index, error}
    Note over SA: 一帧一个分片，服务端无状态转发；对端中途离线立即回 UidInvalid
```

## 14. 客户端 TCP 帧与粘包处理

```mermaid
flowchart TB
    Send["发送<br/>msg_id (quint16 BE) + len (quint16 BE) + JSON body"] --> Socket["QTcpSocket"]
    Socket --> Buf["_buffer.append(readAll())"]
    Buf --> Enough{"_buffer ≥ 4 字节?"}
    Enough -->|否| Wait["等待更多数据"]
    Enough -->|是| Header["解析 msg_id + len<br/>移除前 4 字节"]
    Header --> Body{"_buffer ≥ len?"}
    Body -->|否| Pending["_b_recv_pending = true<br/>等待下一批"]
    Body -->|是| Consume["取 len 字节作为包体<br/>_buffer 前移"]
    Consume --> Handle["HandleMsg(msg_id, body)"]
    Handle --> Buf
```

---

## 与旧 `.drawio` 的对应关系

| 原文件 | 本文件章节 |
|--------|-----------|
| `ChatClient/Client.drawio` | §5 客户端窗口流转 |
| `ChatClient/MsgBubble.drawio` | §6 消息气泡布局 |
| `GateServer/GateServer.drawio` | §2 GateServer |
| `StatusServer/StatusServer.drawio` | §3 StatusServer |
| `VerifyServer/VerifyServer.drawio` | §4 VerifyServer |
| `ChatServer/ChatServer.drawio` | §7 登录与建连 |
| `ChatServer/AddFriend.drawio` | §8 加好友与通过验证 |
| `ChatServer/LoginAndAddFriend.drawio` | §9 登录 + 加好友全景 |

新增图：§10 会话生命周期、§11 分布式锁、§12 聊天记录分页、§13 文件分片转发、§14 客户端 TCP 帧与粘包处理。
