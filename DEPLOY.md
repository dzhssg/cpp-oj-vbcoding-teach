# 部署文档 (DEPLOY.md)

本文档描述 **cpp-oj-vibecoding-teach** 在单机 Linux 环境下的部署步骤。系统面向 1-20 人的教学/训练场景，采用单机部署形态。

---

## 1. 环境要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Linux（Ubuntu/Debian 或 CentOS/RHEL） |
| 编译器 | g++（支持 C++17，即 GCC 7+） |
| 构建工具 | CMake ≥ 3.16、make |
| 数据库 | MySQL 5.7 / 8.0 |
| 网络 | 构建阶段需可访问 GitHub（FetchContent 拉取 cpp-httplib / nlohmann-json） |

> **判题依赖**：运行时判题使用 `/usr/bin/g++` 编译用户提交的 C++ 代码，请确保部署机上已安装 g++ 且路径为 `/usr/bin/g++`。

---

## 2. 安装系统依赖

### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake mysql-server \
    libmysqlclient-dev libyaml-cpp-dev libssl-dev
```

### CentOS / RHEL

```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake mysql-server mysql-devel yaml-cpp-devel openssl-devel
```

依赖说明：

| 依赖 | 用途 |
|------|------|
| g++ / make | C++ 编译器与构建工具链（同时用于运行时判题编译） |
| cmake | 构建系统 |
| mysql-server | 数据库持久化 |
| libmysqlclient-dev / mysql-devel | C++ 连接 MySQL 的客户端库 |
| libyaml-cpp-dev / yaml-cpp-devel | 解析 `config/config.yaml` |
| libssl-dev / openssl-devel | OpenSSL 支持（bcrypt / HTTPS） |

> cpp-httplib、nlohmann/json 为单头文件库，由 CMake 通过 FetchContent 在构建时自动拉取，无需手动安装。

---

## 3. 初始化数据库

### 3.1 启动 MySQL 服务

```bash
sudo systemctl start mysqld     # 或 sudo systemctl start mysql
sudo systemctl enable mysqld
```

### 3.2 执行初始化脚本

初始化脚本 `database/init.sql` 会创建数据库 `cpp_oj` 及以下表：`problems`、`test_cases`、`users`、`sessions`。

```bash
sudo mysql < database/init.sql
```

若 MySQL root 设置了密码：

```bash
mysql -u root -p < database/init.sql
```

---

## 4. 配置文件

编辑 `config/config.yaml`，根据实际环境修改数据库连接信息：

```yaml
server:
  host: "0.0.0.0"     # 监听地址，0.0.0.0 表示所有网卡
  port: 8080          # 监听端口

database:
  host: "127.0.0.1"
  port: 3306
  user: "root"        # 修改为你的 MySQL 用户
  password: ""        # 修改为你的 MySQL 密码
  name: "cpp_oj"
  pool_size: 4        # 连接池大小

logger:
  level: "INFO"       # 日志级别：DEBUG / INFO / WARN / ERROR
  file: "logs/oj.log" # 日志文件路径
```

> 服务从**当前工作目录**读取 `config/config.yaml`，并将静态资源挂载到 `./public`。因此启动时请在项目根目录执行，见第 6 节。

创建日志目录（若不存在）：

```bash
mkdir -p logs
```

---

## 5. 构建

在项目根目录执行：

```bash
cmake -S . -B build
cmake --build build -j
```

构建产物位于 `build/` 目录：

| 可执行文件 | 说明 |
|------------|------|
| `oj_server` | 主服务程序 |
| `add_admin` | 创建/重置管理员账号工具 |
| `db_reset` | 重置数据库工具（清空数据并插入示例题目） |

> 如需构建测试：`cmake -S . -B build -DBUILD_TESTS=ON && cmake --build build -j`

---

## 6. 初始化数据与账号

以下工具默认读取 `config/config.yaml`（可通过第一个参数指定其他配置路径），请在项目根目录运行。

### 6.1 创建管理员账号

```bash
./build/add_admin
```

创建默认管理员：

| 用户名 | 密码 |
|--------|------|
| `admin` | `admin123` |

> 若已存在同名管理员会先删除再重建。**首次部署后请尽快修改默认密码。**

### 6.2 （可选）重置数据库并插入示例题目

```bash
./build/db_reset
```

该工具会：清理会话、清理非管理员用户、清空题目及测试用例、确保管理员账号存在，并插入一道示例题目「A+B Problem」。

---

## 7. 启动服务

在项目根目录运行（保证能正确读取 `config/config.yaml` 与 `./public`）：

```bash
./build/oj_server
```

启动成功后，浏览器访问：

```
http://<服务器IP>:8080/login.html
```

使用管理员账号 `admin / admin123` 登录，进入 `/admin.html` 管理题目；普通用户可注册后在 `/index.html` 浏览题目并提交代码。

> 服务通过 `SIGINT` / `SIGTERM` 优雅退出（`Ctrl+C` 或 `kill <pid>`）。

---

## 8. 后台常驻运行（可选）

### 使用 nohup

```bash
nohup ./build/oj_server > logs/stdout.log 2>&1 &
```

### 使用 systemd

创建 `/etc/systemd/system/oj_server.service`：

```ini
[Unit]
Description=cpp-oj server
After=network.target mysql.service

[Service]
Type=simple
WorkingDirectory=/home/dzh/project/cpp-oj-vbcoding-teach
ExecStart=/home/dzh/project/cpp-oj-vbcoding-teach/build/oj_server
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

> `WorkingDirectory` 必须指向项目根目录，以便正确加载配置与静态资源。

启用并启动：

```bash
sudo systemctl daemon-reload
sudo systemctl enable oj_server
sudo systemctl start oj_server
sudo systemctl status oj_server
```

---

## 9. 验证部署

| # | 验证项 |
|---|--------|
| 1 | 访问 `http://<IP>:8080/login.html` 页面正常加载 |
| 2 | 使用 `admin / admin123` 登录成功，可进入管理后台 |
| 3 | 管理员新增题目后，题目列表可见 |
| 4 | 普通用户注册、登录，进入题目详情提交 C++ 代码 |
| 5 | 提交后返回判题结果（AC/WA/TLE/RE/CE） |
| 6 | 管理员可删除题目 |

---

## 10. 常见问题

**Q: 构建时卡在拉取 cpp-httplib / json？**
构建阶段需访问 GitHub。若网络受限，可配置代理或使用镜像源后重试。

**Q: 启动报无法加载 `config/config.yaml`？**
请在项目根目录启动，或确认配置文件路径正确。

**Q: 提交代码始终返回 CE（编译错误）？**
确认部署机安装了 g++ 且位于 `/usr/bin/g++`；判题子进程直接调用该路径编译。

**Q: 无法连接数据库？**
检查 `config/config.yaml` 中的数据库账号密码、MySQL 服务是否启动、`cpp_oj` 数据库是否已初始化。

---

## 11. 判题隔离与资源限制

用户提交的代码在独立子进程中通过 `fork + g++` 编译运行，并施加以下限制（见 `src/service/executor_service.cc`）：

| 限制项 | 值 |
|--------|-----|
| CPU 时间 | 5 秒 |
| 内存 (RLIMIT_AS) | 256 MB |
| 输出大小 | 64 KB |
| Core Dump | 禁用 |
| 墙钟超时 | 5 秒（超时判为 TLE） |

> **安全提示**：当前为进程级隔离，安全性有限，适用于小规模可信教学场景。生产/开放环境建议升级为容器级隔离（如 Docker/nsjail）。
