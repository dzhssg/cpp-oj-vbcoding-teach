# C++ 在线判题（OJ）系统 | 独立开发

面向教学/训练场景，从零设计并实现的轻量级在线判题系统，采用前后端分离架构，涵盖题目管理、在线编译运行判题、用户认证与权限控制全流程。

- **后端架构**：基于 C++17 与 cpp-httplib 搭建 RESTful HTTP 服务，采用 Handler / Service / Model / DB 四层分层架构，使用 nlohmann/json 处理数据交互，通过 CMake（FetchContent）管理构建与第三方依赖。
- **判题引擎**：基于 `fork + execvp + g++` 实现进程级隔离的代码编译与执行，利用 `setrlimit` 限制 CPU 时间、内存（256MB）、输出大小及禁用 Core Dump，通过 `waitpid(WNOHANG)` 轮询结合信号处理实现超时控制，精确判定 AC / WA / TLE / RE / CE 五种结果。
- **数据与并发**：设计 MySQL 数据模型（题目、测试用例、用户、会话），实现基于互斥锁与条件变量的线程安全 MySQL 连接池，并封装 RAII 风格的 `ScopedConnection` 自动管理连接生命周期。
- **认证与安全**：基于 OpenSSL 生成随机 Session ID，实现 Session + HttpOnly Cookie 认证机制，采用 bcrypt 对密码加盐哈希，并对管理接口做角色权限校验（401/403）与输入合法性校验。
- **前端实现**：使用原生 HTML/CSS/JavaScript（无框架）实现题目列表、在线代码编辑器、实时判题结果反馈及管理后台，页面无需刷新即可完成完整提交。
- **测试与质量保障**：构建覆盖单元、接口、UI 三层的自动化测试体系——基于 GoogleTest 对判题引擎、连接池、认证等核心模块做单元测试，基于 curl 编写分模块 API 自动化测试脚本，并基于 Playwright 实现端到端（E2E）UI 回归测试，保障关键路径质量。

**技术栈**：C++17、cpp-httplib、nlohmann/json、MySQL、OpenSSL、bcrypt、GoogleTest、Playwright、CMake、Linux 系统编程（fork/pipe/rlimit/signal）
