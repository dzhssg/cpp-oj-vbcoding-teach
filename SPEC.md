# OJ 系统规格说明书
## 1. 需求概述
| 维度 | 规格 |
|------|------|
| **业务⽬标** | 教学/训练平台 |
| **⽬标⽤⼾规模** | 1-20 ⼈同时在线 |
| **核⼼功能** | 题⽬列表、题⽬描述、测试⽤例、在线编辑/编译/运⾏/返回结果、题⽬管理后台
|
| **⻆⾊** | 普通⽤⼾（做题）、管理员（增删题⽬） |

| **语⾔⽀持** | 仅 C++ |
| **题⽬来源** | 管理员⼿动导⼊ |
| **执⾏隔离** | 进程级隔离（基础） |
| **部署形态** | 单机部署 |
| **性能要求** | <500ms 响应 |
| **持久化** | 题⽬存 MySQL，提交记录/代码暂不持久化 |
---
## 2. 系统架构
```
┌─────────────────────────────────────────────────┐
│ 前端 (Browser) │
│ HTML + CSS + JS (原⽣，⽆框架) │
└──────────────────┬──────────────────────────────┘
│ HTTP REST
┌──────────────────▼──────────────────────────────┐
│ C++ Backend │
│ cpp-httplib (HTTP) │
│ ┌─────────────┬─────────────┬────────────────┐ │
│ │ 题⽬服务 │ 代码执⾏服务 │ 认证/权限服务 │ │
│ │ (MySQL) │ (fork/popen)│ (Session) │ │
│ └─────────────┴─────────────┴────────────────┘ │
└─────────────────────────────────────────────────┘
```
---
## 2.1 项⽬ 录结构
```
cpp-oj-vibecoding-teach/
├── SPEC.md
├── README.md
├── CMakeLists.txt # 后端构建配置
├── config/
│ └── config.yaml # 配置⽂件
├── database/
│ └── init.sql # 数据库初始化脚本
├── src/
│ ├── main.cc # 程序⼊⼝
│ ├── server/
│ │ ├── server.cc # HTTP 服务器
│ │ ├── router.cc # 路由处理
│ │ └── router.h
│ ├── handler/

│ │ ├── problem_handler.cc # 题⽬相关 API
│ │ ├── submit_handler.cc # 代码提交执⾏
│ │ ├── auth_handler.cc # 登录注册
│ │ └── admin_handler.cc # 管理接⼝
│ ├── service/
│ │ ├── problem_service.cc # 题⽬业务逻辑
│ │ ├── executor_service.cc # 代码执⾏服务
│ │ └── auth_service.cc # 认证业务逻辑
│ ├── model/
│ │ ├── problem.cc # 题⽬数据模型
│ │ ├── test_case.cc # 测试⽤例模型
│ │ └── user.cc # ⽤⼾模型
│ ├── db/
│ │ ├── connection_pool.cc # MySQL 连接池
│ │ └── connection_pool.h
│ └── utils/
│ ├── logger.cc # ⽇志⼯具
│ ├── logger.h
│ ├── config.cc # 配置加载
│ └── config.h
├── public/
│ ├── index.html # 题⽬列表⻚
│ ├── login.html # 登录⻚
│ ├── register.html # 注册⻚
│ ├── problem.html # 题⽬详情⻚
│ ├── admin.html # 管理后台⻚
│ ├── css/
│ │ └── style.css # 样式⽂件
│ └── js/
│ ├── api.js # API 调⽤封装
│ ├── auth.js # 认证状态管理
│ ├── problem.js # 题⽬列表逻辑
│ ├── problem_detail.js # 题⽬详情逻辑
│ ├── submit.js # 提交执⾏逻辑
│ └── admin.js # 管理后台逻辑
└── tests/
├── unit/
│ ├── problem_test.cc
│ └── executor_test.cc
└── integration/
└── api_test.cc
```
---
## 3. API 边界

### 3.1 公开接⼝（普通⽤⼾）
| ⽅法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/problems` | 题⽬列表 |
| GET | `/api/problems/:id` | 题⽬详情（含描述/⽤例） |
| POST | `/api/submit` | 提交代码执⾏ |
| POST | `/api/login` | 登录 |
| POST | `/api/logout` | 登出 |
| POST | `/api/register` | 注册新⽤⼾ |
### 3.2 管理接⼝（管理员）
| ⽅法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/admin/problems` | 新增题⽬ |
| DELETE | `/api/admin/problems/:id` | 删除题⽬ |
---
## 4. 前端⻚⾯
| ⻚⾯ | 路径 | 说明 | 访问权限 |
|------|------|------|----------|
| 登录⻚ | `/login.html` | ⽤⼾名 + 密码登录，登录成功跳转题⽬列表 | 所有⼈ |
| 注册⻚ | `/register.html` | ⽤⼾名 + 密码 + 确认密码，注册成功跳转登录⻚ | 所有⼈ |
| 题⽬列表⻚ | `/index.html` | 展⽰所有题⽬（编号、标题、难度），点击进⼊详情 | 已登录
⽤⼾ |
| 题⽬详情⻚ | `/problem.html?id=:id` | 题⽬描述 + 在线代码编辑器 + 提交按钮 + 结果展
⽰ | 已登录⽤⼾ |
| 后台管理⻚ | `/admin.html` | 新增题⽬表单、题⽬列表（含删除操作） | 管理员 |
---
## 5. 数据模型 (MySQL)
```sql
-- 题⽬表
CREATE TABLE problems (
id INT PRIMARY KEY AUTO_INCREMENT,
title VARCHAR(255) NOT NULL,
difficulty ENUM('Easy','Medium','Hard') NOT NULL,
content TEXT NOT NULL, -- 题⽬描述 (Markdown)
template TEXT, -- 代码模板
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
-- 测试⽤例表 (与题⽬ 1:N 关联)
CREATE TABLE test_cases (
id INT PRIMARY KEY AUTO_INCREMENT,
problem_id INT NOT NULL,
input TEXT NOT NULL, -- 输⼊数据
expected TEXT NOT NULL, -- 期望输出
position INT NOT NULL DEFAULT 0, -- 排序序号
FOREIGN KEY (problem_id) REFERENCES problems(id) ON DELETE CASCADE
);
-- ⽤⼾表
CREATE TABLE users (
id INT PRIMARY KEY AUTO_INCREMENT,
username VARCHAR(64) UNIQUE NOT NULL,
password VARCHAR(128) NOT NULL, -- bcrypt 哈希
role ENUM('user','admin') DEFAULT 'user',
created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```
---
## 6. TODO 清单
### Phase 1 - 基础设施
- [X] 项⽬ 录结构搭建
- [X] MySQL 数据库初始化脚本
- [X] cpp-httplib 基础 HTTP 服务
- [X] 配置管理
- [X] ⽇志封装
- [X] 数据库连接池实现
### Phase 2 - 题⽬模块
- [X] 题⽬数据模型映射
- [X] 题⽬ CRUD API（管理员）
- [X] 题⽬列表/详情 API（⽤⼾）
### Phase 3 - 代码执⾏模块
- [X] C++ 代码编译（fork + g++）
- [X] 代码运⾏ + 超时控制
- [X] 结果⽐较（stdout vs expected）
- [X] 进程级资源限制（CPU/内存）
### Phase 4 - 登录注册模块
- [X] Session/Cookie 认证机制
- [X] ⽤⼾注册 API（⽤⼾名唯⼀性校验）
- [X] ⽤⼾登录 API
- [] ⽤⼾退出登录
### Phase 5 - 前端
- [X] 登录⻚⾯ (`/login.html`)
- [X] 注册⻚⾯ (`/register.html`)
- [X] 题⽬列表⻚⾯
- [X] 题⽬详情⻚⾯（描述 + 在线编辑器）
- [X] 提交结果展⽰
- [X] 管理后台（新增/删除题⽬）
- [ ] 大屏落地页
### Phase 6 - 安全与部署
- [ ] 管理员权限校验
- [ ] ⽤⼾认证（Session/Cookie）
- [ ] 基础输⼊校验
- [ ] 部署⽂档
---
## 7. 验收标准
| # | 标准 |
|---|------|
| 1 | 管理员可成功新增题⽬并在前端列表看到 |
| 2 | 普通⽤⼾可查看题⽬、在线编辑 C++ 代码并提交 |
| 3 | 代码在 <5s 超时限制内执⾏并返回结果 |
| 4 | 正确判断 AC/WA/TLE/RE 并反馈给⽤⼾ |
| 5 | 管理员可删除题⽬ |
| 6 | 普通⽤⼾⽆法访问管理接⼝ |
| 7 | 部署⽂档完整，单机可运⾏ |
| 8 | ⻚⾯⽆需刷新可完成⼀次完整提交 |
| 9 | 新⽤⼾可注册账号并登录 |
---
## 8. 潜在⻛险与权衡
| ⻛险 | 权衡 |
|------|------|
| 进程级隔离安全性低（恶意代码可能影响系统） | ⼩规模教学场景可接受，建议后期升级容器级隔
离 |
| 代码不持久化，重启丢失 | 当前阶段明确知晓，后续按需加 Redis/DB |
| <500ms 在代码编译时难以保证 | 放宽⾄ 5s 内返回，或预编译缓存优化 |
| MySQL 单机部署⽆主从 | ⼩规模可接受，注意备份 |