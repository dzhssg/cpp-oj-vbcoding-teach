# cpp-oj-vibecoding-teach

一个面向 **教学 / 训练** 场景的轻量级 C++ 在线判题（OJ）系统。支持题目管理、在线编写 C++ 代码、编译运行并实时返回判题结果（AC / WA / TLE / RE / CE）。

- **目标规模**：1-20 人同时在线
- **语言支持**：仅 C++
- **部署形态**：单机部署
- **执行隔离**：进程级隔离（fork + g++ + rlimit 资源限制）

---

## 功能特性

- 题目列表 / 题目详情浏览（含题目描述、代码模板）
- 在线代码编辑、提交、编译、运行与结果反馈
- 用户注册、登录、登出（Session/Cookie 认证）
- 管理后台：新增 / 删除题目
- 判题状态：`AC`（通过）、`WA`（答案错误）、`CE`（编译错误）、`TLE`（超时，5s）、`RE`（运行时错误）
- 代码执行资源限制：CPU 时间、内存（256MB）、输出大小、Core Dump 禁用

---

## 技术栈

| 层次 | 技术 |
|------|------|
| 前端 | 原生 HTML + CSS + JavaScript（无框架） |
| 后端 | C++17 + [cpp-httplib](https://github.com/yhirose/cpp-httplib) |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) |
| 数据库 | MySQL 8（题目、测试用例、用户、会话） |
| 认证 | Session + Cookie（`session_id`，HttpOnly，Max-Age=86400s） |
| 密码 | bcrypt 哈希 |
| 构建 | CMake（≥ 3.16） |

---

## 系统架构

```
┌─────────────────────────────────────────────────┐
│ 前端 (Browser)                                    │
│ HTML + CSS + JS (原生，无框架)                     │
└──────────────────┬──────────────────────────────┘
                   │ HTTP REST
┌──────────────────▼──────────────────────────────┐
│ C++ Backend (cpp-httplib)                         │
│ ┌─────────────┬─────────────┬────────────────┐  │
│ │ 题目服务     │ 代码执行服务  │ 认证/权限服务    │  │
│ │ (MySQL)     │ (fork/g++)  │ (Session)       │  │
│ └─────────────┴─────────────┴────────────────┘  │
└─────────────────────────────────────────────────┘
```

---

## 目录结构

```
cpp-oj-vibecoding-teach/
├── SPEC.md               # 规格说明书
├── API.md                # API 接口文档
├── DEPLOY.md             # 部署文档
├── dependence.md         # 依赖清单
├── CMakeLists.txt        # 后端构建配置
├── config/
│   └── config.yaml       # 配置文件
├── database/
│   └── init.sql          # 数据库初始化脚本
├── src/                  # 后端源码
│   ├── main.cc           # 程序入口
│   ├── server/           # HTTP 服务器与路由
│   ├── handler/          # API 处理层（problem/submit/auth/admin）
│   ├── service/          # 业务逻辑（problem/executor/auth/session）
│   ├── model/            # 数据模型（problem/test_case/user/session）
│   ├── db/               # MySQL 连接池
│   └── utils/            # 日志、配置、bcrypt
├── public/               # 前端静态资源
│   ├── index.html / login.html / register.html
│   ├── problem.html / admin.html / problem_list.html
│   ├── css/style.css
│   └── js/               # api / auth / problem / submit / admin
├── tools/                # 运维工具（add_admin / db_reset）
└── tests/                # 单元与集成测试
```

---

## API 概览

基础路径：`http://<host>:8080`，数据格式 JSON。详见 [API.md](API.md)。

### 公开接口

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/problems` | 题目列表 |
| GET | `/api/problems/:id` | 题目详情（含描述/用例） |
| POST | `/api/submit` | 提交代码执行 |
| POST | `/api/register` | 注册新用户 |
| POST | `/api/login` | 登录 |
| POST | `/api/logout` | 登出 |

### 管理接口（需管理员）

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/problems` | 新增题目 |
| DELETE | `/api/admin/problems/:id` | 删除题目 |

---

## 前端页面

| 页面 | 路径 | 访问权限 |
|------|------|----------|
| 登录页 | `/login.html` | 所有人 |
| 注册页 | `/register.html` | 所有人 |
| 题目列表页 | `/index.html` | 已登录用户 |
| 题目详情页 | `/problem.html?id=:id` | 已登录用户 |
| 后台管理页 | `/admin.html` | 管理员 |

---

## 快速开始

详见 [DEPLOY.md](DEPLOY.md)。简要步骤：

```bash
# 1. 安装依赖（Ubuntu/Debian）
sudo apt-get update
sudo apt-get install -y build-essential cmake mysql-server \
    libmysqlclient-dev libyaml-cpp-dev libssl-dev

# 2. 初始化数据库
sudo mysql < database/init.sql

# 3. 配置 config/config.yaml（数据库账号密码等）

# 4. 构建
cmake -S . -B build
cmake --build build -j

# 5. 创建管理员账号（用户名 admin / 密码 admin123）
./build/add_admin

# 6. 启动服务
./build/oj_server

# 浏览器访问 http://localhost:8080/login.html
```

---

## 默认账号

| 角色 | 用户名 | 密码 |
|------|--------|------|
| 管理员 | `admin` | `admin123` |

> 首次部署后请尽快修改默认密码。

---

## 潜在风险与权衡

| 风险 | 说明 |
|------|------|
| 进程级隔离安全性有限 | 小规模教学场景可接受，生产环境建议升级容器级隔离 |
| 提交记录/代码暂不持久化 | 重启后丢失，后续可按需引入 Redis/DB |
| MySQL 单机无主从 | 小规模可接受，注意定期备份 |

---

## 相关文档

- [SPEC.md](SPEC.md) — 规格说明书
- [API.md](API.md) — API 接口文档
- [DEPLOY.md](DEPLOY.md) — 部署文档
- [dependence.md](dependence.md) — 依赖清单
