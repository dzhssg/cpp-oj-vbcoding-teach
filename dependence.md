# 依赖清单

## 系统级别依赖

| 依赖 | 用途 | Ubuntu/Debian | CentOS/RHEL |
|------|------|---------------|-------------|
| g++ / make | C++ 编译器、make 等工具链 | `sudo apt-get install -y build-essential` | `sudo yum groupinstall -y "Development Tools"` |
| cmake | 构建系统 | `sudo apt-get install -y cmake` | `sudo yum install -y cmake` |
| mysql-server | 数据库持久化 | `sudo apt-get install -y mysql-server` | `sudo yum install -y mysql-server` |
| mysql 客户端库 | C++ 连接 MySQL | `sudo apt-get install -y libmysqlclient-dev` | `sudo yum install -y mysql-devel` |
| yaml-cpp | 解析 config.yaml 配置文件 | `sudo apt-get install -y libyaml-cpp-dev` | `sudo yum install -y yaml-cpp-devel` |
| openssl | SSL/TLS 支持（httplib HTTPS、bcrypt） | `sudo apt-get install -y libssl-dev` | `sudo yum install -y openssl-devel` |

## 头文件/源码级依赖

以下库为单头文件或轻量库，建议在 CMakeLists.txt 中通过 FetchContent 自动引入，无需手动安装：

| 依赖 | 用途 |
|------|------|
| cpp-httplib | HTTP 服务器 |
| nlohmann/json | JSON 序列化/反序列化（REST API） |
| bcrypt | 用户密码哈希 |

## 一键安装命令

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake mysql-server libmysqlclient-dev libyaml-cpp-dev libssl-dev
```

**CentOS/RHEL:**

```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake mysql-server mysql-devel yaml-cpp-devel openssl-devel
```
