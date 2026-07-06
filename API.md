# OJ 系统 API 接口文档

## 概述

- **基础路径**: `http://host:8080`
- **数据格式**: JSON（`Content-Type: application/json`）
- **认证方式**: Session/Cookie 认证，Cookie 名为 `session_id`，HttpOnly，Max-Age=86400s

### 执行状态码说明

| 状态码 | 含义 |
|--------|------|
| AC | Accepted（通过） |
| WA | Wrong Answer（答案错误） |
| CE | Compile Error（编译错误） |
| TLE | Time Limit Exceeded（超时，5s） |
| RE | Runtime Error（运行时错误） |

---

## 一、公开接口

### 1. POST /api/register — 用户注册

- **认证**: 不需要

**请求体 (JSON)**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| username | string | 是 | 用户名，3-64 个字符 |
| password | string | 是 | 密码，至少 6 个字符 |
| confirm_password | string | 否 | 确认密码，需与 password 一致 |

**请求示例**
```json
{
  "username": "test_user",
  "password": "123456",
  "confirm_password": "123456"
}
```

**成功响应 201**
```json
{
  "id": 1,
  "message": "User registered successfully"
}
```

**错误响应**
| 状态码 | 错误信息 |
|--------|----------|
| 400 | `Invalid JSON` |
| 400 | `Missing or invalid 'username' field` |
| 400 | `Missing or invalid 'password' field` |
| 400 | `Username cannot be empty` |
| 400 | `Username must be between 3 and 64 characters` |
| 400 | `Password cannot be empty` |
| 400 | `Password must be at least 6 characters` |
| 400 | `Passwords do not match` |
| 409 | `Username already taken` |
| 500 | `Failed to create user` |

---

### 2. POST /api/login — 用户登录

- **认证**: 不需要

**请求体 (JSON)**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| username | string | 是 | 用户名 |
| password | string | 是 | 密码 |

**请求示例**
```json
{
  "username": "admin",
  "password": "admin123"
}
```

**成功响应 200**
```json
{
  "id": 1,
  "username": "admin",
  "role": "admin"
}
```
同时返回 `Set-Cookie: session_id=xxx; Path=/; HttpOnly; SameSite=Lax; Max-Age=86400`

**错误响应**
| 状态码 | 错误信息 |
|--------|----------|
| 400 | `Invalid JSON` |
| 400 | `Missing or invalid 'username' field` |
| 400 | `Missing or invalid 'password' field` |
| 400 | `Username cannot be empty` |
| 400 | `Password cannot be empty` |
| 401 | `Invalid username or password` |
| 500 | `Failed to create session` |

---

### 3. POST /api/logout — 用户登出

- **认证**: 不需要（自动读取 Cookie 中的 session_id）

**请求体**: 无

**成功响应 200**
```json
{
  "message": "Logged out successfully"
}
```
同时通过 `Set-Cookie` 清除 session cookie。

---

### 4. GET /api/problems — 题目列表

- **认证**: 不需要

**请求体**: 无

**成功响应 200**
```json
[
  {
    "id": 1,
    "title": "A+B Problem",
    "difficulty": "Easy"
  },
  {
    "id": 2,
    "title": "Quick Sort",
    "difficulty": "Medium"
  }
]
```

---

### 5. GET /api/problems/{id} — 题目详情

- **认证**: 不需要

**路径参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | int | 题目 ID |

**成功响应 200**
```json
{
  "id": 1,
  "title": "A+B Problem",
  "difficulty": "Easy",
  "content": "## Description\nGiven two integers A and B, output A + B.",
  "template": "#include <iostream>\nint main() { return 0; }",
  "created_at": "2025-01-01 12:00:00",
  "test_cases": [
    {
      "id": 1,
      "input": "1 2",
      "expected": "3",
      "position": 0
    }
  ]
}
```

**响应字段说明**

| 字段 | 类型 | 说明 |
|------|------|------|
| id | int | 题目 ID |
| title | string | 题目标题 |
| difficulty | string | 难度：Easy / Medium / Hard |
| content | string | 题目描述（Markdown 格式） |
| template | string | 代码模板 |
| created_at | string | 创建时间 |
| test_cases | array | 测试用例列表 |

**错误响应**
| 状态码 | 错误信息 |
|--------|----------|
| 404 | `Problem not found` |

---

### 6. POST /api/submit — 提交代码执行

- **认证**: 不需要

**请求体 (JSON)**

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| problem_id | int | 是 | 题目 ID |
| code | string | 是 | C++ 源代码 |

**请求示例**
```json
{
  "problem_id": 1,
  "code": "#include <iostream>\nint main() {\n  int a, b;\n  std::cin >> a >> b;\n  std::cout << a + b;\n  return 0;\n}"
}
```

**成功响应 200**

全部通过 (AC) 示例：
```json
{
  "status": "AC",
  "passed": 3,
  "total": 3,
  "results": [
    {
      "position": 0,
      "status": "AC",
      "input": "1 2",
      "expected": "3",
      "actual": "3"
    },
    {
      "position": 1,
      "status": "AC",
      "input": "10 20",
      "expected": "30",
      "actual": "30"
    },
    {
      "position": 2,
      "status": "AC",
      "input": "-1 1",
      "expected": "0",
      "actual": "0"
    }
  ]
}
```

编译错误 (CE) 示例：
```json
{
  "status": "CE",
  "passed": 0,
  "total": 0,
  "compile_error": "solution.cpp:1:1: error: 'include' does not name a type\n #include <iostream>\n ^~~~~~~\n",
  "results": []
}
```

部分通过 (WA) 示例：
```json
{
  "status": "WA",
  "passed": 2,
  "total": 3,
  "results": [
    {
      "position": 0,
      "status": "AC",
      "input": "1 2",
      "expected": "3",
      "actual": "3"
    },
    {
      "position": 1,
      "status": "WA",
      "input": "10 20",
      "expected": "30",
      "actual": "2010"
    },
    {
      "position": 2,
      "status": "AC",
      "input": "-1 1",
      "expected": "0",
      "actual": "0"
    }
  ]
}
```

**顶层响应字段**

| 字段 | 类型 | 说明 |
|------|------|------|
| status | string | 最终判定：AC / WA / CE / TLE / RE |
| passed | int | 通过的测试用例数 |
| total | int | 测试用例总数（CE 时为 0） |
| compile_error | string | 编译错误信息（仅 status=CE 时出现） |
| results | array | 每个测试用例的详细结果 |

**results 数组项字段**

| 字段 | 类型 | 说明 |
|------|------|------|
| position | int | 测试用例序号 |
| status | string | 该用例判定：AC / WA / RE / TLE |
| input | string | 输入数据 |
| expected | string | 期望输出 |
| actual | string | 实际输出 |
| error | string | 错误详情（仅非空时出现） |

**执行限制**
- 编译超时：默认系统限制
- 运行超时：5 秒
- 内存限制：256 MB
- 输出限制：65536 字节
- 编译器：g++ -std=c++17 -O2 -Wall

**错误响应**
| 状态码 | 错误信息 |
|--------|----------|
| 400 | `Invalid JSON` |
| 400 | `Missing or invalid 'problem_id'` |
| 400 | `Missing or invalid 'code'` |
| 400 | `Code cannot be empty` |

---

## 二、管理接口（需要管理员权限）

### 7. POST /api/admin/problems — 新增题目

- **认证**: 需要（需管理员角色）

**请求体 (JSON)**

| 字段 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| title | string | 是 | - | 题目标题 |
| content | string | 是 | - | 题目描述（Markdown 格式） |
| difficulty | string | 否 | `"Easy"` | 难度：Easy / Medium / Hard |
| template | string | 否 | `""` | 代码模板 |
| test_cases | array | 否 | `[]` | 测试用例列表 |

**test_cases 数组项字段**

| 字段 | 类型 | 必填 | 默认值 | 说明 |
|------|------|------|--------|------|
| input | string | 否 | `""` | 输入数据 |
| expected | string | 否 | `""` | 期望输出 |
| position | int | 否 | 自动递增 | 排序序号 |

**请求示例**
```json
{
  "title": "A+B Problem",
  "difficulty": "Easy",
  "content": "## Description\nGiven two integers A and B, output their sum.",
  "template": "#include <iostream>\nint main() {\n  return 0;\n}",
  "test_cases": [
    { "input": "1 2", "expected": "3", "position": 0 },
    { "input": "10 20", "expected": "30", "position": 1 }
  ]
}
```

**成功响应 201**
```json
{
  "id": 1,
  "message": "Problem created successfully"
}
```

**错误响应**
| 状态码 | 错误信息 |
|--------|----------|
| 400 | `Invalid JSON` |
| 400 | `Missing or invalid 'title' field` |
| 400 | `Missing or invalid 'content' field` |
| 401 | `Authentication required` |
| 401 | `Invalid or expired session` |
| 403 | `Admin privileges required` |
| 500 | `Failed to create problem` |

---

### 8. DELETE /api/admin/problems/{id} — 删除题目

- **认证**: 需要（需管理员角色）

**路径参数**

| 参数 | 类型 | 说明 |
|------|------|------|
| id | int | 要删除的题目 ID |

**请求体**: 无

**成功响应 200**
```json
{
  "message": "Problem deleted successfully"
}
```

**错误响应**
| 状态码 | 错误信息 |
|--------|----------|
| 401 | `Authentication required` |
| 401 | `Invalid or expired session` |
| 403 | `Admin privileges required` |
| 404 | `Problem not found` |

---

## 三、认证机制说明

系统使用 **Session + Cookie** 方式进行认证：

1. 用户登录成功后，服务端生成一个 24 小时有效期的 session，并通过 `Set-Cookie` 返回 `session_id`
2. Cookie 属性：`Path=/; HttpOnly; SameSite=Lax; Max-Age=86400`
3. 调用需认证的接口时，浏览器自动携带 Cookie，服务端通过 `extractSessionId()` 解析
4. 管理接口额外校验 `role == "admin"`
5. 登出时服务端销毁 session 并清除 Cookie

## 四、接口总览

| 方法 | 路径 | 认证 | 角色 | 说明 |
|------|------|------|------|------|
| POST | `/api/register` | 否 | 所有人 | 用户注册 |
| POST | `/api/login` | 否 | 所有人 | 用户登录 |
| POST | `/api/logout` | 否 | 所有人 | 用户登出 |
| GET | `/api/problems` | 否 | 所有人 | 获取题目列表 |
| GET | `/api/problems/:id` | 否 | 所有人 | 获取题目详情 |
| POST | `/api/submit` | 否 | 所有人 | 提交代码执行 |
| POST | `/api/admin/problems` | 是 | 管理员 | 新增题目 |
| DELETE | `/api/admin/problems/:id` | 是 | 管理员 | 删除题目 |
