# Qt仿微信即时通信软件

> 使用 C++ + Qt 构建的跨平台即时通信软件，后端基于分布式微服务架构，支持高并发长连接、消息收发、好友管理等核心功能。

## 📌 项目简介

- **客户端：** 使用 Qt + C++ 实现，支持登录、聊天窗口、好友管理、表情与气泡 UI、美化样式（QSS）
- **后端服务：** 使用 Boost.Asio + gRPC 构建微服务架构，包括 GateServer / ChatServer / StatusServer / VerifyServer
- **数据管理：** 使用 Redis 做缓存、MySQL 作为主存储，支持断线重连、消息持久化
- **通信协议：** 自定义 protobuf 消息协议，支持长连接高并发通信

## 🚀 项目特点

- 支持 2W+ 长连接并发，消息延迟低于 10ms
- 系统稳定运行，无掉线丢包
- 架构清晰，可平滑扩展和部署

## 🧱 技术栈

| 模块 | 技术 |
|------|------|
| 前端 | C++, Qt, QSS, QPainter |
| 后端 | C++, Boost.Asio, gRPC, protobuf |
| 数据 | Redis, MySQL |
| 系统 | Linux, TCP/IP, Epoll |
| 部署 | Makefile, Shell 脚本 |

## 📁 项目结构


