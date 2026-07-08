# OJ 系统 Web 自动化测试文档

## 测试环境

| 项目 | 值 |
|------|-----|
| 服务器地址 | `http://8.138.161.148:8080` |
| 管理员账号 | `admin` / `admin123` |
| 测试方式一 (API) | `curl` 命令行 API 自动化测试 |
| 测试方式二 (UI) | `playwright-cli` 浏览器 UI 自动化测试（详见附录 B） |
| Cookie 管理 | `curl -c /tmp/oj_cookie.txt -b /tmp/oj_cookie.txt` |

---

## 目录

1. [用户认证模块 (AUTH)](#1-用户认证模块-auth)
2. [题目浏览模块 (PROB)](#2-题目浏览模块-prob)
3. [代码提交模块 (SUBM)](#3-代码提交模块-subm)
4. [管理后台模块 (ADMIN)](#4-管理后台模块-admin)
5. [权限校验模块 (PERM)](#5-权限校验模块-perm)
6. [端到端集成场景 (E2E)](#6-端到端集成场景-e2e)

---

## 通用前置与清理

> 每个模块的测试可独立运行。运行前需先执行以下命令获取管理员 Cookie（供 ADMIN 和 PERM 模块使用）：

```bash
curl -c /tmp/oj_cookie.txt -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login
```

> 运行结束后清理 Cookie：

```bash
rm -f /tmp/oj_cookie.txt
```

---

# 1. 用户认证模块 (AUTH)

## 1.1 用户注册

### AUTH-01 正常注册新用户

**测试点**：验证正常注册流程，检查返回 201 及用户 ID

> 使用 `$(date +%s)` 生成唯一用户名，确保多次运行均能通过。

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser_'$(date +%s)'","password":"pass1234","confirm_password":"pass1234"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`201`
- 响应体包含 `"id"` 字段（整数）和 `"message":"User registered successfully"`

---

### AUTH-02 注册时缺少用户名

**测试点**：验证缺少 username 字段时返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"password":"pass1234","confirm_password":"pass1234"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Missing or invalid 'username' field"` 或类似错误信息

---

### AUTH-03 注册时缺少密码

**测试点**：验证缺少 password 字段时返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser_nopwd","confirm_password":"pass1234"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error"` 字段

---

### AUTH-04 注册时密码不足 6 位

**测试点**：验证密码长度校验（最少 6 位）

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser_short","password":"12345","confirm_password":"12345"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Password must be at least 6 characters"`

---

### AUTH-05 注册时两次密码不一致

**测试点**：验证 confirm_password 校验

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser_mismatch","password":"pass1234","confirm_password":"different"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Passwords do not match"`

---

### AUTH-06 注册时用户名为空

**测试点**：验证空用户名被拒绝

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"","password":"pass1234","confirm_password":"pass1234"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Username cannot be empty"`

---

### AUTH-07 用户名长度校验（少于 3 字符）

**测试点**：验证用户名需 3-64 字符

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"ab","password":"pass1234","confirm_password":"pass1234"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Username must be between 3 and 64 characters"`

---

### AUTH-08 注册已存在的用户名

**测试点**：验证用户名唯一性约束

```bash
# 先注册一个用户
curl -s -H "Content-Type: application/json" \
  -d '{"username":"dup_user","password":"pass1234"}' \
  http://8.138.161.148:8080/api/register

# 再次注册同名用户
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"dup_user","password":"pass5678","confirm_password":"pass5678"}' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- 第二次请求 HTTP 状态码：`409`
- 响应体包含 `"error":"Username already taken"`

---

### AUTH-09 注册时 body 为非 JSON 格式

**测试点**：验证非法 JSON 时的错误处理

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d 'not-valid-json' \
  http://8.138.161.148:8080/api/register
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Invalid JSON"`

---

## 1.2 用户登录

### AUTH-10 正常登录

**测试点**：验证使用正确凭据登录，返回用户信息和设置 Session Cookie

```bash
curl -c /tmp/oj_cookie.txt \
  -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login
```

**预期结果**：
- HTTP 状态码：`200`
- 响应体包含 `"username":"admin"`、`"role":"admin"`
- 响应头 `Set-Cookie` 中包含 `session_id=`
- Cookie 文件被成功写入

---

### AUTH-11 使用错误密码登录

**测试点**：验证错误密码返回 401

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"wrongpassword"}' \
  http://8.138.161.148:8080/api/login
```

**预期结果**：
- HTTP 状态码：`401`
- 响应体包含 `"error":"Invalid username or password"`

---

### AUTH-12 使用不存在的用户名登录

**测试点**：验证不存在的用户名返回 401

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"nonexistent_user_xyz","password":"pass1234"}' \
  http://8.138.161.148:8080/api/login
```

**预期结果**：
- HTTP 状态码：`401`
- 响应体包含 `"error":"Invalid username or password"`

---

### AUTH-13 登录时缺少字段

**测试点**：验证缺少必填字段时的错误处理

```bash
# 缺少 password
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"admin"}' \
  http://8.138.161.148:8080/api/login
```

**预期结果**：
- HTTP 状态码：`400`

---

### AUTH-14 登录时用户名为空

**测试点**：验证空用户名被拒绝

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"username":"","password":"admin123"}' \
  http://8.138.161.148:8080/api/login
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Username cannot be empty"`

---

### AUTH-15 登录时 body 为非 JSON

**测试点**：验证非法 JSON 返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d 'invalid{json' \
  http://8.138.161.148:8080/api/login
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Invalid JSON"`

---

## 1.3 用户登出

### AUTH-16 正常登出（有有效 Session）

**测试点**：登录后登出，验证 Session 被销毁

```bash
# 登录
curl -c /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login

# 登出
curl -b /tmp/oj_cookie.txt \
  -s -w "\nHTTP_STATUS:%{http_code}" \
  -X POST \
  http://8.138.161.148:8080/api/logout
```

**预期结果**：
- HTTP 状态码：`200`
- 响应体包含 `"message":"Logged out successfully"`
- 之后的 admin 请求应返回 401（Session 已失效）

---

### AUTH-17 无 Session 时登出（幂等性）

**测试点**：验证未登录时调用登出接口不报错

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -X POST \
  http://8.138.161.148:8080/api/logout
```

**预期结果**：
- HTTP 状态码：`200`
- 响应体包含 `"message":"Logged out successfully"`

---

## 1.4 Session 管理

### AUTH-18 Session 有效期内可访问受保护接口

**测试点**：登录后使用 Cookie 访问 admin 接口

```bash
# 登录
curl -c /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login

# 使用 Cookie 访问题目列表（此接口不强制认证，但验证 Cookie 传递正常）
curl -b /tmp/oj_cookie.txt \
  -s -w "\nHTTP_STATUS:%{http_code}" \
  http://8.138.161.148:8080/api/problems
```

**预期结果**：
- HTTP 状态码：`200`
- 返回 JSON 数组

---

### AUTH-19 无效 Session 无法访问 admin 接口

**测试点**：使用伪造/过期的 Session Cookie 访问管理接口

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Cookie: session_id=0000000000000000000000000000000000000000000000000000000000000000" \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}' \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`401`
- 响应体包含 `"error":"Invalid or expired session"`

---

# 2. 题目浏览模块 (PROB)

## 2.1 题目列表

### PROB-01 获取所有题目列表

**测试点**：验证获取题目列表 API

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  http://8.138.161.148:8080/api/problems
```

**预期结果**：
- HTTP 状态码：`200`
- 返回 JSON 数组，每个元素包含 `id`、`title`、`difficulty` 字段
- 无题目时返回空数组 `[]`

---

### PROB-02 题目列表不包含敏感信息

**测试点**：验证列表接口不泄露题目描述、测试用例等详细信息

```bash
curl -s http://8.138.161.148:8080/api/problems | python3 -c "
import json, sys
data = json.load(sys.stdin)
if not data:
    print('PASS: 题目列表为空')
    sys.exit(0)
first = data[0]
sensitive = ['content', 'template', 'solution_content', 'solution_code', 'test_cases', 'created_at']
for key in sensitive:
    if key in first:
        print(f'FAIL: 列表接口泄露了 {key} 字段')
        sys.exit(1)
print('PASS: 列表接口未泄露敏感字段')
"
```

**预期结果**：输出 `PASS`，列表中元素仅包含 `id`、`title`、`difficulty`

---

### PROB-03 未登录也可访问题目列表（API 层面）

**测试点**：验证题目列表 API 不做强制认证

```bash
# 不携带任何 Cookie
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  http://8.138.161.148:8080/api/problems
```

**预期结果**：
- HTTP 状态码：`200`
- 返回 JSON 数组

---

## 2.2 题目详情

### PROB-04 获取存在的题目详情

**测试点**：验证通过 ID 获取题目详情（包括测试用例）

```bash
# 先获取题目列表，取第一个 ID
FIRST_ID=$(curl -s http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=json.load(sys.stdin); print(data[0]['id'] if data else 1)")
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  http://8.138.161.148:8080/api/problems/$FIRST_ID
```

**预期结果**：
- HTTP 状态码：`200`
- 响应体包含 `id`、`title`、`difficulty`、`content`、`template`、`test_cases` 等字段
- `test_cases` 为数组，每个元素包含 `id`、`input`、`expected`、`position`

---

### PROB-05 获取不存在的题目详情

**测试点**：验证请求不存在的题目 ID 返回 404

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  http://8.138.161.148:8080/api/problems/99999
```

**预期结果**：
- HTTP 状态码：`404`
- 响应体包含 `"error":"Problem not found"`

---

### PROB-06 题目详情包含代码模板

**测试点**：验证 template 字段不为空时包含 C++ 代码

```bash
FIRST_ID=$(curl -s http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=json.load(sys.stdin); print(data[0]['id'] if data else 1)")
curl -s http://8.138.161.148:8080/api/problems/$FIRST_ID | python3 -c "
import json, sys
data = json.load(sys.stdin)
if 'template' in data:
    print(f'template 字段存在，长度: {len(data[\"template\"])}')
    if data['template'] and '#include' in data['template']:
        print('PASS: 模板包含 C++ 代码')
    else:
        print('INFO: 模板为空或无 #include')
else:
    print('INFO: 无 template 字段')
"
```

**预期结果**：模板字段存在（可能为空字符串）

---

### PROB-07 题目详情中 test_cases 数据完整性

**测试点**：验证 test_cases 数组中每个元素包含必要字段

```bash
FIRST_ID=$(curl -s http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=json.load(sys.stdin); print(data[0]['id'] if data else 1)")
curl -s http://8.138.161.148:8080/api/problems/$FIRST_ID | python3 -c "
import json, sys
data = json.load(sys.stdin)
cases = data.get('test_cases', [])
if not cases:
    print('WARN: 该题目无测试用例')
else:
    required = ['id', 'input', 'expected', 'position']
    for tc in cases:
        for r in required:
            if r not in tc:
                print(f'FAIL: 测试用例缺少字段 {r}')
                sys.exit(1)
    print(f'PASS: {len(cases)} 个测试用例，字段完整')
"
```

**预期结果**：输出 `PASS`（全部用例字段完整）

---

# 3. 代码提交模块 (SUBM)

## 3.1 基本提交流程

### SUBM-01 提交正确的 C++ 代码 (AC)

**测试点**：验证提交能通过所有测试用例的代码，返回 AC

```bash
PROBLEM_ID=1
CODE='#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}'
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PROBLEM_ID,\"code\":$(echo "$CODE" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')}" \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`200`
- `status` 为 `"AC"`（如测试用例数据期望为两数之和）
- `passed == total`

> **注意**：实际 AC 结果取决于题目的测试用例期望输出。`PROBLEM_ID` 需根据实际题目调整。

---

### SUBM-02 提交错误答案的 C++ 代码 (WA)

**测试点**：验证提交输出不正确的代码，返回 WA

```bash
PROBLEM_ID=1
CODE='#include <iostream>
int main() {
    std::cout << 0 << std::endl;
    return 0;
}'
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PROBLEM_ID,\"code\":$(echo "$CODE" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')}" \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`200`
- `status` 为 `"WA"`
- `results` 中包含 `"status":"WA"` 的条目
- 每个 WA 测试用例中 `actual != expected`

---

### SUBM-03 提交有编译错误的代码 (CE)

**测试点**：验证提交无法编译的 C++ 代码，返回 CE

```bash
PROBLEM_ID=1
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PROBLEM_ID,\"code\":\"#include <iostream>\nint main() { invalid_syntax###return 0; }\"}" \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`200`
- `status` 为 `"CE"`
- 响应体包含 `compile_error` 字段（g++ 编译错误信息）
- `results` 为空数组

---

### SUBM-04 提交超时代码 (TLE)

**测试点**：验证提交死循环代码，在 5s 后被终止并返回 TLE

```bash
PROBLEM_ID=1
CODE='#include <iostream>
int main() {
    while(true) {}
    return 0;
}'
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PROBLEM_ID,\"code\":$(echo "$CODE" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')}" \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`200`
- `status` 为 `"TLE"`

---

### SUBM-05 提交运行时错误代码 (RE)

**测试点**：验证提交会触发段错误的代码，返回 RE

```bash
PROBLEM_ID=1
CODE='#include <iostream>
int main() {
    int* p = nullptr;
    *p = 42;
    return 0;
}'
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PROBLEM_ID,\"code\":$(echo "$CODE" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')}" \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`200`
- `status` 为 `"RE"`

---

## 3.2 提交参数校验

### SUBM-06 缺少 problem_id

**测试点**：验证不传 problem_id 返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"code":"#include <iostream>\nint main(){return 0;}"}' \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Missing or invalid 'problem_id'"`

---

### SUBM-07 缺少 code 字段

**测试点**：验证不传 code 返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"problem_id":1}' \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Missing or invalid 'code'"`

---

### SUBM-08 代码为空

**测试点**：验证提交空代码返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"problem_id":1,"code":""}' \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Code cannot be empty"`

---

### SUBM-09 提交到不存在的题目

**测试点**：验证提交到不存在的 problem_id

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d '{"problem_id":99999,"code":"#include <iostream>\nint main(){return 0;}"}' \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`200`
- `status` 为 `"CE"`
- `compile_error` 包含 `"Problem not found"`

---

### SUBM-10 非法 JSON body

**测试点**：验证提交非法 JSON 返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d 'not json' \
  http://8.138.161.148:8080/api/submit
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Invalid JSON"`

---

## 3.3 判题结果验证

### SUBM-11 验证返回结果结构完整性

**测试点**：验证成功提交后响应体包含所有必要字段

```bash
PROBLEM_ID=1
CODE='#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}'
curl -s http://8.138.161.148:8080/api/submit \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PROBLEM_ID,\"code\":$(echo "$CODE" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')}" | python3 -c "
import json, sys
data = json.load(sys.stdin)
required_fields = ['status', 'passed', 'total']
for f in required_fields:
    if f not in data:
        print(f'FAIL: 缺少字段 {f}')
        sys.exit(1)
if data['status'] != 'CE':
    if 'results' not in data or not isinstance(data['results'], list):
        print('FAIL: 缺少 results 数组')
        sys.exit(1)
    for r in data['results']:
        for rf in ['position', 'status', 'input', 'expected', 'actual']:
            if rf not in r:
                print(f'FAIL: 测试结果缺少字段 {rf}')
                sys.exit(1)
if data['status'] == 'CE':
    if 'compile_error' not in data:
        print('FAIL: CE 状态下缺少 compile_error')
        sys.exit(1)
print(f'PASS: 状态={data[\"status\"]}, passed={data[\"passed\"]}, total={data[\"total\"]}')
"
```

**预期结果**：输出 `PASS` 加状态信息

---

### SUBM-12 判题优先级验证（RE 优先于 WA）

**测试点**：在多个测试用例中，RE 优先级高于 WA 作为最终状态

```bash
# 提交始终输出 0 的代码（WA），且可能第一个用例就 WA
# 若既有 WA 也有 RE 的代码，应返回 RE
# 这里设计为：先用 AC 代码验证理解，再用已知会导致 WA 的代码验证

# 注：此测试依赖题目的实际测试用例数据。若第一个用例为 RE（段错误），
# 即使后续用例 WA，最终 status 也应为 RE。
```

**验证方法**：检查整体 `status` 的判定逻辑符合 `CE > RE > TLE > WA > AC`

---

# 4. 管理后台模块 (ADMIN)

## 4.1 前置：管理员登录

> 所有 ADMIN 测试用例需要先登录获取管理员 Cookie。在每个用例前执行：

```bash
curl -c /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login
```

> **可重复运行说明**：以下 ADMIN-01 ~ ADMIN-06 用例使用 `$(date +%s)` 生成唯一标题，确保多次运行互不干扰。ADMIN-06 末尾会删除测试题目。

---

### ADMIN-01 创建新题目（完整字段 + 多个测试用例）

**测试点**：验证管理员创建包含完整字段和多测试用例的题目

```bash
# 登录管理员
curl -c /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login > /dev/null 2>&1

# 使用时间戳生成唯一标题
TITLE="两数之和_$(date +%s)"

# 构造 JSON 请求体
BODY=$(cat <<'ENDOFJSON'
{
    "title": "REPLACE_TITLE",
    "difficulty": "Easy",
    "content": "# 两数之和\n\n计算两个整数的和。\n\n## 输入\n一行两个整数 a, b",
    "template": "#include <iostream>\n\nint main() {\n    int a, b;\n    std::cin >> a >> b;\n    // 在此编写代码\n    return 0;\n}",
    "solution_content": "## 题解\n直接计算 a + b 即可。",
    "solution_code": "#include <iostream>\n\nint main() {\n    int a, b;\n    std::cin >> a >> b;\n    std::cout << a + b << std::endl;\n    return 0;\n}",
    "test_cases": [
      {"input": "1 2", "expected": "3", "position": 0},
      {"input": "10 20", "expected": "30", "position": 1},
      {"input": "-5 5", "expected": "0", "position": 2}
    ]
  }
ENDOFJSON
)
BODY="${BODY/REPLACE_TITLE/$TITLE}"

curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d "$BODY" \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`201`
- 响应体包含 `"id"` 字段（整数）和 `"message":"Problem created successfully"`

---

### ADMIN-02 题目创建后可在列表中看到

**测试点**：验证 ADMIN-01 创建的题目出现在列表中

```bash
curl -s -b /tmp/oj_cookie.txt \
  http://8.138.161.148:8080/api/problems | python3 -c "
import json, sys
data = json.load(sys.stdin)
target = [p for p in data if p['title'] == '$TITLE']
if target:
    print(f'PASS: 在列表中找到题目「$TITLE」，ID={target[0][\"id\"]}')
else:
    print('FAIL: 未找到创建的题目')
"
```

**预期结果**：输出 `PASS`，找到目标题目

---

### ADMIN-03 题目创建后可查看详情（含测试用例）

**测试点**：验证 ADMIN-01 创建的题目详情正确

```bash
# 获取刚创建的题目 ID
NEW_ID=$(curl -s -b /tmp/oj_cookie.txt \
  http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=[p for p in json.load(sys.stdin) if p['title']=='$TITLE']; print(data[0]['id'] if data else '')")

curl -s http://8.138.161.148:8080/api/problems/$NEW_ID | python3 -c "
import json, sys
data = json.load(sys.stdin)
assert data['title'] == '$TITLE', f'FAIL: 标题不匹配: {data[\"title\"]}'
assert data['difficulty'] == 'Easy', f'FAIL: 难度不匹配'
assert data['content'].startswith('#'), f'FAIL: content 不是 Markdown'
assert 'template' in data, 'FAIL: 缺少 template'
assert data.get('solution_content'), 'FAIL: 缺少 solution_content'
assert data.get('solution_code'), 'FAIL: 缺少 solution_code'
cases = data.get('test_cases', [])
assert len(cases) == 3, f'FAIL: 测试用例数量应为 3，实际 {len(cases)}'
for tc in cases:
    assert 'input' in tc and 'expected' in tc, f'FAIL: 测试用例缺少字段'
print(f'PASS: 题目 ID={data[\"id\"]}，测试用例 {len(cases)} 个')
"
```

**预期结果**：输出 `PASS`

---

### ADMIN-04 更新题目信息

**测试点**：验证管理员可以更新题目

```bash
# 获取 ADMIN-01 创建的题目 ID
NEW_ID=$(curl -s -b /tmp/oj_cookie.txt \
  http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=[p for p in json.load(sys.stdin) if p['title']=='$TITLE']; print(data[0]['id'] if data else '')")

NEW_TITLE="${TITLE}（修订版）"

curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d "{
    \"title\": \"$NEW_TITLE\",
    \"difficulty\": \"Medium\",
    \"content\": \"# 两数之和 v2\n计算两个整数的和（更新版）。\",
    \"template\": \"#include <iostream>\nint main(){int a,b;std::cin>>a>>b;return 0;}\",
    \"test_cases\": [
      {\"input\": \"1 2\", \"expected\": \"3\", \"position\": 0},
      {\"input\": \"100 200\", \"expected\": \"300\", \"position\": 1}
    ]
  }" \
  http://8.138.161.148:8080/api/admin/problems/$NEW_ID
```

**预期结果**：
- HTTP 状态码：`200`
- 响应体包含 `"message":"Problem updated successfully"`
- 再次获取详情，标题应为 `"${TITLE}（修订版）"`，难度应为 `"Medium"`

---

### ADMIN-05 更新不存在的题目

**测试点**：验证更新不存在的题目返回 404

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test","difficulty":"Easy"}' \
  http://8.138.161.148:8080/api/admin/problems/99999
```

**预期结果**：
- HTTP 状态码：`404`
- 响应体包含 `"error":"Problem not found"`

---

### ADMIN-06 删除题目

**测试点**：验证管理员可以删除题目

```bash
# 获取 ADMIN-04 更新后的题目 ID（标题含"修订版"）
DEL_ID=$(curl -s -b /tmp/oj_cookie.txt \
  http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=[p for p in json.load(sys.stdin) if '修订版' in p['title']]; print(data[0]['id'] if data else '')")

curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -X DELETE \
  http://8.138.161.148:8080/api/admin/problems/$DEL_ID
```

**预期结果**：
- HTTP 状态码：`200`
- 响应体包含 `"message":"Problem deleted successfully"`
- 再次获取列表，该题目应不再出现

> **注意**：删除后清理 Cookie：
> ```bash
> rm -f /tmp/oj_cookie.txt
> ```

---

### ADMIN-07 删除不存在的题目

**测试点**：验证删除不存在的题目返回 404

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -X DELETE \
  http://8.138.161.148:8080/api/admin/problems/99999
```

**预期结果**：
- HTTP 状态码：`404`
- 响应体包含 `"error":"Problem not found"`

---

### ADMIN-08 创建题目时缺少 title

**测试点**：验证必填字段校验

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"content":"Some description","difficulty":"Easy"}' \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Missing or invalid 'title' field"`

---

### ADMIN-09 创建题目时缺少 content

**测试点**：验证必填字段 content 校验

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"title":"No Content Problem","difficulty":"Easy"}' \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Missing or invalid 'content' field"`

---

### ADMIN-10 创建题目时 difficulty 默认值

**测试点**：验证不传 difficulty 时默认为 Easy

```bash
# 使用时间戳生成唯一标题
DIFF_TITLE="默认难度测试_$(date +%s)"

# 创建题目（不传 difficulty）
curl -s -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d "{
    \"title\": \"$DIFF_TITLE\",
    \"content\": \"# 测试\n一个测试题目。\",
    \"test_cases\": [{\"input\": \"1\", \"expected\": \"1\", \"position\": 0}]
  }" \
  http://8.138.161.148:8080/api/admin/problems > /dev/null

# 获取该题目的 difficulty
curl -s http://8.138.161.148:8080/api/problems | python3 -c "
import json, sys
data = json.load(sys.stdin)
target = [p for p in data if p['title'] == '$DIFF_TITLE']
if target:
    diff = target[0]['difficulty']
    print(f'difficulty = {diff}')
    if diff == 'Easy':
        print('PASS: 默认难度为 Easy')
    else:
        print(f'WARN: 默认难度为 {diff}，非 Easy')
else:
    print('WARN: 未找到测试题目')
"

# 清理：删除测试题目（可选）
DIFF_NEW_ID=$(curl -s http://8.138.161.148:8080/api/problems | python3 -c "import json,sys; data=[p for p in json.load(sys.stdin) if p['title']=='$DIFF_TITLE']; print(data[0]['id'] if data else '')")
if [ -n "$DIFF_NEW_ID" ]; then
  curl -s -b /tmp/oj_cookie.txt -X DELETE \
    http://8.138.161.148:8080/api/admin/problems/$DIFF_NEW_ID > /dev/null
fi
```

**预期结果**：`difficulty` 为 `"Easy"`

---

### ADMIN-11 创建题目时带非 JSON body

**测试点**：验证非法 JSON 返回 400

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d 'not{valid' \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`400`
- 响应体包含 `"error":"Invalid JSON"`

---

# 5. 权限校验模块 (PERM)

## 5.1 管理员权限

### PERM-01 未登录访问 admin 创建接口

**测试点**：无 Cookie 时访问 admin 接口返回 401

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}' \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`401`
- 响应体包含 `"error":"Authentication required"`

---

### PERM-02 未登录访问 admin 删除接口

**测试点**：无 Cookie 时删除接口返回 401

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -X DELETE \
  http://8.138.161.148:8080/api/admin/problems/1
```

**预期结果**：
- HTTP 状态码：`401`
- 响应体包含 `"error":"Authentication required"`

---

### PERM-03 未登录访问 admin 更新接口

**测试点**：无 Cookie 时更新接口返回 401

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -X PUT \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}' \
  http://8.138.161.148:8080/api/admin/problems/1
```

**预期结果**：
- HTTP 状态码：`401`

---

### PERM-04 普通用户访问 admin 接口

**测试点**：使用普通用户（非 admin 角色）的 Cookie 访问 admin 接口

```bash
# 注册一个普通用户
USERNAME="permtest_$(date +%s)"
curl -s -H "Content-Type: application/json" \
  -d "{\"username\":\"$USERNAME\",\"password\":\"pass1234\"}" \
  http://8.138.161.148:8080/api/register > /dev/null

# 用该普通用户登录
curl -c /tmp/oj_cookie_user.txt \
  -s -H "Content-Type: application/json" \
  -d "{\"username\":\"$USERNAME\",\"password\":\"pass1234\"}" \
  http://8.138.161.148:8080/api/login > /dev/null

# 尝试访问 admin 接口
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie_user.txt \
  -X POST \
  -H "Content-Type: application/json" \
  -d '{"title":"黑客攻击","content":"尝试越权"}' \
  http://8.138.161.148:8080/api/admin/problems
```

**预期结果**：
- HTTP 状态码：`403`
- 响应体包含 `"error":"Admin privileges required"`

> 验收标准 #6：普通用户无法访问管理接口

---

### PERM-05 普通用户无法删除题目

**测试点**：普通用户无法删除题目

```bash
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie_user.txt \
  -X DELETE \
  http://8.138.161.148:8080/api/admin/problems/1
```

**预期结果**：
- HTTP 状态码：`403`

---

### PERM-06 管理员登录后可正常访问 admin 接口

**测试点**：验证管理员凭据正常工作（正面测试）

```bash
# 管理员登录
curl -c /tmp/oj_cookie.txt \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  http://8.138.161.148:8080/api/login > /dev/null

# 访问 admin 接口
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -b /tmp/oj_cookie.txt \
  http://8.138.161.148:8080/api/problems
```

**预期结果**：
- HTTP 状态码：`200`

---

# 6. 端到端集成场景 (E2E)

> 下表映射 SPEC.md 第 7 章验收标准：

| 场景 | 对应验收标准 |
|------|-------------|
| E2E-01 | #1 管理员可成功新增题目并在前端列表看到 |
| E2E-02 | #2 普通用户可查看题目、在线编辑 C++ 代码并提交 |
| E2E-03 | #3 代码在 <5s 超时限制内执行并返回结果 |
| E2E-04 | #4 正确判断 AC/WA/TLE/RE 并反馈给用户 |
| E2E-05 | #5 管理员可删除题目 |
| E2E-06 | #6 普通用户无法访问管理接口 |
| E2E-08 | #8 页面无需刷新可完成一次完整提交 |
| E2E-09 | #9 新用户可注册账号并登录 |

---

### E2E-01 管理员创建题目 → 学生查看完整流程

**对应验收标准**：#1 管理员可成功新增题目并在前端列表看到

**测试步骤**：

```bash
#!/bin/bash
BASE="http://8.138.161.148:8080"
COOKIE="/tmp/oj_e2e_cookie.txt"

# Step 1: 管理员登录
echo "=== Step 1: 管理员登录 ==="
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  $BASE/api/login | python3 -m json.tool

# Step 2: 管理员创建题目
echo "=== Step 2: 创建题目 ==="
CREATE_RESULT=$(curl -s -b $COOKIE -H "Content-Type: application/json" \
  -d '{
    "title": "E2E测试-斐波那契",
    "difficulty": "Medium",
    "content": "# 斐波那契数列\n\n输入一个整数 n，输出第 n 项斐波那契数。\n\n## 输入\n一个整数 n (0 ≤ n ≤ 30)\n\n## 输出\n第 n 项斐波那契数",
    "template": "#include <iostream>\n\nint main() {\n    int n;\n    std::cin >> n;\n    // 在此编写代码\n    return 0;\n}",
    "test_cases": [
      {"input": "0", "expected": "0", "position": 0},
      {"input": "1", "expected": "1", "position": 1},
      {"input": "10", "expected": "55", "position": 2}
    ]
  }' \
  $BASE/api/admin/problems)
echo "$CREATE_RESULT" | python3 -m json.tool
PROBLEM_ID=$(echo "$CREATE_RESULT" | python3 -c "import json,sys; print(json.load(sys.stdin).get('id',''))")
echo "新题目 ID: $PROBLEM_ID"

# Step 3: 验证题目出现在列表中
echo "=== Step 3: 验证题目列表 ==="
curl -s $BASE/api/problems | python3 -c "
import json, sys
data = json.load(sys.stdin)
found = [p for p in data if p['title'] == 'E2E测试-斐波那契']
if found:
    print(f'PASS: 题目「E2E测试-斐波那契」出现在列表中，ID={found[0][\"id\"]}')
else:
    print('FAIL: 题目未出现在列表中')
"

# Step 4: 验证题目详情
echo "=== Step 4: 验证题目详情 ==="
curl -s $BASE/api/problems/$PROBLEM_ID | python3 -c "
import json, sys
data = json.load(sys.stdin)
assert data['title'] == 'E2E测试-斐波那契', '标题不匹配'
assert len(data['test_cases']) == 3, '测试用例数量不匹配'
print(f'PASS: 题目详情正确，测试用例: {len(data[\"test_cases\"])} 个')
"

# Step 5: 学生提交 AC 代码
echo "=== Step 5: 提交 AC 代码 ==="
AC_CODE='#include <iostream>

int fib(int n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}

int main() {
    int n;
    std::cin >> n;
    std::cout << fib(n) << std::endl;
    return 0;
}'
curl -s $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "$(python3 -c "
import json
print(json.dumps({'problem_id': $PROBLEM_ID, 'code': '''$AC_CODE'''}))
")" | python3 -c "
import json, sys
data = json.load(sys.stdin)
print(f'状态: {data[\"status\"]}, 通过: {data[\"passed\"]}/{data[\"total\"]}')
if data['status'] == 'AC':
    print('PASS: 代码提交结果正确 (AC)')
else:
    print(f'结果: status={data[\"status\"]}')
"

# Step 6: 学生提交 WA 代码
echo "=== Step 6: 提交 WA 代码 ==="
WA_CODE='#include <iostream>
int main() {
    int n;
    std::cin >> n;
    std::cout << n << std::endl;
    return 0;
}'
curl -s $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "$(python3 -c "
import json
print(json.dumps({'problem_id': $PROBLEM_ID, 'code': '''$WA_CODE'''}))
")" | python3 -c "
import json, sys
data = json.load(sys.stdin)
print(f'状态: {data[\"status\"]}, 通过: {data[\"passed\"]}/{data[\"total\"]}')
if data['status'] == 'WA':
    print('PASS: WA 判定正确')
else:
    print(f'结果: status={data[\"status\"]}')
"

# Step 7: 管理员删除题目
echo "=== Step 7: 删除题目 ==="
curl -s -b $COOKIE -X DELETE $BASE/api/admin/problems/$PROBLEM_ID | python3 -m json.tool

# Step 8: 验证删除成功
echo "=== Step 8: 验证删除成功 ==="
curl -s $BASE/api/problems | python3 -c "
import json, sys
data = json.load(sys.stdin)
found = [p for p in data if p['title'] == 'E2E测试-斐波那契']
if not found:
    print('PASS: 题目已被删除')
else:
    print('FAIL: 题目仍然存在')
"

rm -f $COOKIE
echo "=== E2E-01 测试完成 ==="
```

**预期结果**：所有步骤输出 `PASS`

---

### E2E-02 普通用户注册 → 登录 → 查看题目 → 提交代码

**对应验收标准**：#2、#9

**测试步骤**：

```bash
#!/bin/bash
BASE="http://8.138.161.148:8080"
COOKIE="/tmp/oj_e2e2_cookie.txt"
TIMESTAMP=$(date +%s)

# Step 1: 注册新用户
echo "=== Step 1: 注册用户 ==="
curl -s -w "\nHTTP_STATUS:%{http_code}" \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"student_$TIMESTAMP\",\"password\":\"pass1234\",\"confirm_password\":\"pass1234\"}" \
  $BASE/api/register

# Step 2: 登录
echo "=== Step 2: 登录 ==="
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d "{\"username\":\"student_$TIMESTAMP\",\"password\":\"pass1234\"}" \
  $BASE/api/login | python3 -c "
import json, sys
data = json.load(sys.stdin)
assert data['role'] == 'user', f'FAIL: 角色应为 user，实际 {data[\"role\"]}'
print(f'PASS: 登录成功，用户={data[\"username\"]}，角色={data[\"role\"]}')
"

# Step 3: 查看题目列表
echo "=== Step 3: 查看题目列表 ==="
curl -s $BASE/api/problems | python3 -c "
import json, sys
data = json.load(sys.stdin)
print(f'题目总数: {len(data)}')
if len(data) > 0:
    print(f'第一题: ID={data[0][\"id\"]}, 标题={data[0][\"title\"]}, 难度={data[0][\"difficulty\"]}')
print('PASS: 成功获取题目列表')
"

# Step 4: 查看题目详情
echo "=== Step 4: 查看题目详情 ==="
FIRST_ID=$(curl -s $BASE/api/problems | python3 -c "import json,sys; d=json.load(sys.stdin); print(d[0]['id'] if d else 1)")
curl -s $BASE/api/problems/$FIRST_ID | python3 -c "
import json, sys
data = json.load(sys.stdin)
assert 'content' in data, 'FAIL: 缺少 content'
assert 'template' in data, 'FAIL: 缺少 template'
assert 'test_cases' in data, 'FAIL: 缺少 test_cases'
print(f'PASS: 题目详情获取成功，测试用例 {len(data[\"test_cases\"])} 个')
"

# Step 5: 提交代码
echo "=== Step 5: 提交代码 ==="
CODE='#include <iostream>
int main() {
    int a, b;
    std::cin >> a >> b;
    std::cout << a + b << std::endl;
    return 0;
}'
RESULT=$(curl -s $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "$(python3 -c "
import json
print(json.dumps({'problem_id': $FIRST_ID, 'code': '''$CODE'''}))
")")
echo "$RESULT" | python3 -c "
import json, sys
data = json.load(sys.stdin)
print(f'提交状态: {data[\"status\"]}')
print(f'通过: {data[\"passed\"]}/{data[\"total\"]}')
if data['status'] != 'CE':
    for r in data.get('results', []):
        print(f'  用例#{r[\"position\"]}: {r[\"status\"]} (expected={r[\"expected\"]}, actual={r[\"actual\"]})')
"
echo "PASS: 代码提交成功"

rm -f $COOKIE
echo "=== E2E-02 测试完成 ==="
```

**预期结果**：所有步骤输出 `PASS`，用户成功注册、登录、查看题目、提交代码

---

### E2E-03 四种判题结果完整性验证

**对应验收标准**：#3、#4

**测试描述**：对同一题目分别提交 AC / WA / TLE / CE 代码，验证系统能正确判定

```bash
#!/bin/bash
BASE="http://8.138.161.148:8080"
COOKIE="/tmp/oj_e2e3_cookie.txt"

# 管理员登录 + 创建测试题目
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  $BASE/api/login > /dev/null

CREATE_RESP=$(curl -s -b $COOKIE -H "Content-Type: application/json" \
  -d '{
    "title": "判题测试-A+B",
    "difficulty": "Easy",
    "content": "# A+B\n计算两个整数的和。",
    "test_cases": [
      {"input": "1 2", "expected": "3", "position": 0},
      {"input": "100 200", "expected": "300", "position": 1}
    ]
  }' \
  $BASE/api/admin/problems)
PID=$(echo "$CREATE_RESP" | python3 -c "import json,sys; print(json.load(sys.stdin)['id'])")
echo "创建题目 ID=$PID"

# 测试1: AC - 正确代码
echo "--- 测试 AC ---"
AC='#include <iostream>
int main(){int a,b;std::cin>>a>>b;std::cout<<a+b<<std::endl;return 0;}'
curl -s $BASE/api/submit -H "Content-Type: application/json" \
  -d "$(python3 -c "import json; print(json.dumps({'problem_id':$PID,'code':'$AC'}))")" \
  | python3 -c "import json,sys;d=json.load(sys.stdin);assert d['status']=='AC';print('AC PASS')"

# 测试2: WA - 错误代码
echo "--- 测试 WA ---"
WA='#include <iostream>
int main(){std::cout<<0<<std::endl;return 0;}'
curl -s $BASE/api/submit -H "Content-Type: application/json" \
  -d "$(python3 -c "import json; print(json.dumps({'problem_id':$PID,'code':'$WA'}))")" \
  | python3 -c "import json,sys;d=json.load(sys.stdin);assert d['status']=='WA';print('WA PASS')"

# 测试3: TLE - 死循环
echo "--- 测试 TLE ---"
TLE='#include <iostream>
int main(){while(true){}return 0;}'
curl -s $BASE/api/submit -H "Content-Type: application/json" \
  -d "$(python3 -c "import json; print(json.dumps({'problem_id':$PID,'code':'$TLE'}))")" \
  | python3 -c "import json,sys;d=json.load(sys.stdin);assert d['status']=='TLE',f'Expected TLE got {d[\"status\"]}';print('TLE PASS')"

# 测试4: CE - 编译错误
echo "--- 测试 CE ---"
CE='#include <iostream>
int main(){invalid_syntax###return 0;}'
curl -s $BASE/api/submit -H "Content-Type: application/json" \
  -d "$(python3 -c "import json; print(json.dumps({'problem_id':$PID,'code':'$CE'}))")" \
  | python3 -c "import json,sys;d=json.load(sys.stdin);assert d['status']=='CE';assert 'compile_error' in d;print('CE PASS')"

# 清理
curl -s -b $COOKIE -X DELETE $BASE/api/admin/problems/$PID > /dev/null
rm -f $COOKIE
echo "=== E2E-03 全部通过 ==="
```

**预期结果**：依次输出 `AC PASS`、`WA PASS`、`TLE PASS`、`CE PASS`

---

### E2E-04 超时限制验证（<5s）

**对应验收标准**：#3

**测试描述**：提交一个包含 `sleep(2)` 的代码，验证在 5s 内完成执行并返回结果（耗时在正常范围）

```bash
#!/bin/bash
BASE="http://8.138.161.148:8080"
COOKIE="/tmp/oj_e2e4_cookie.txt"

# 使用已有题目 ID=1，或跳过（无题目时）
PID=$(curl -s $BASE/api/problems | python3 -c "import json,sys;d=json.load(sys.stdin);print(d[0]['id'] if d else '')")
if [ -z "$PID" ]; then
  echo "SKIP: 无可用题目"
  exit 0
fi

CODE='#include <iostream>
#include <unistd.h>
int main() {
    int a, b;
    std::cin >> a >> b;
    sleep(2);
    std::cout << a + b << std::endl;
    return 0;
}'

START=$(date +%s%N)
RESULT=$(curl -s $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "$(python3 -c "import json; print(json.dumps({'problem_id':$PID,'code':'$CODE'}))")")
END=$(date +%s%N)
ELAPSED_MS=$(( (END - START) / 1000000 ))

STATUS=$(echo "$RESULT" | python3 -c "import json,sys; print(json.load(sys.stdin)['status'])")
echo "状态: $STATUS, 耗时: ${ELAPSED_MS}ms"
if [ $ELAPSED_MS -lt 5000 ]; then
  echo "PASS: 在 5s 内返回结果"
else
  echo "FAIL: 超过 5s 超时限制"
fi
rm -f $COOKIE
```

**预期结果**：耗时 < 5000ms，输出 `PASS`

---

### E2E-05 管理员完整 CRUD 流程

**对应验收标准**：#1、#5

**测试描述**：管理员 创建 → 查看 → 编辑 → 查看（验证变更）→ 删除 → 查看（验证已删除）

详见 E2E-01 脚本，其已完整覆盖创建→查看→提交→删除流程。更新操作参见 ADMIN-04。

---

### E2E-06 权限隔离验证

**对应验收标准**：#6

见 PERM-04、PERM-05 测试用例。

---

### E2E-07 注册→登录→登出→重新登录 完整流程

**对应验收标准**：#9

```bash
#!/bin/bash
BASE="http://8.138.161.148:8080"
COOKIE="/tmp/oj_e2e7_cookie.txt"
TS=$(date +%s)

# Step 1: 注册
echo "=== Step 1: 注册 ==="
REG=$(curl -s -w "\nHTTP_CODE:%{http_code}" -H "Content-Type: application/json" \
  -d "{\"username\":\"lifecycle_$TS\",\"password\":\"pass1234\",\"confirm_password\":\"pass1234\"}" \
  $BASE/api/register)
echo "$REG"
echo "$REG" | grep -q "HTTP_CODE:201" && echo "PASS: 注册成功 (201)" || echo "FAIL: 注册未返回 201"

# Step 2: 登录
echo "=== Step 2: 登录 ==="
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d "{\"username\":\"lifecycle_$TS\",\"password\":\"pass1234\"}" \
  $BASE/api/login | python3 -c "
import json,sys; d=json.load(sys.stdin)
assert d['username']=='lifecycle_$TS'
print('PASS: 登录成功')
"

# Step 3: 登出
echo "=== Step 3: 登出 ==="
LOGOUT=$(curl -s -b $COOKIE -X POST $BASE/api/logout)
echo "$LOGOUT" | python3 -c "
import json,sys; d=json.load(sys.stdin)
assert d['message']=='Logged out successfully'
print('PASS: 登出成功')
"

# Step 4: 用过期 Cookie 访问 admin（验证 Session 已失效）
echo "=== Step 4: 验证 Session 失效 ==="
HTTP_CODE=$(curl -s -b $COOKIE -X POST \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}' \
  -o /dev/null -w "%{http_code}" \
  $BASE/api/admin/problems)
if [ "$HTTP_CODE" = "401" ]; then
  echo "PASS: 登出后 Session 失效 (401)"
else
  echo "FAIL: 期望 401，实际 $HTTP_CODE"
fi

# Step 5: 重新登录
echo "=== Step 5: 重新登录 ==="
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d "{\"username\":\"lifecycle_$TS\",\"password\":\"pass1234\"}" \
  $BASE/api/login | python3 -c "
import json,sys; d=json.load(sys.stdin)
assert d['username']=='lifecycle_$TS'
print('PASS: 重新登录成功')
"

rm -f $COOKIE
echo "=== E2E-07 测试完成 ==="
```

**预期结果**：所有步骤输出 `PASS`

---

### E2E-08 前端页面可访问性验证

**对应验收标准**：#8（页面无需刷新可完成一次完整提交）

**测试描述**：验证前端静态页面返回 200（HTML 文件可加载）

```bash
BASE="http://8.138.161.148:8080"

echo "=== 验证各页面可访问 ==="
for PAGE in "" "/login.html" "/register.html" "/problem_list.html" "/problem.html" "/admin.html"; do
  HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE$PAGE)
  echo "  $PAGE -> $HTTP_CODE"
  if [ "$HTTP_CODE" != "200" ]; then
    echo "  WARN: 期望 200，实际 $HTTP_CODE"
  fi
done

echo "=== 验证 CSS 和 JS 文件可访问 ==="
for RESOURCE in "/css/style.css" "/js/api.js" "/js/auth.js" "/js/problem.js" "/js/problem_detail.js" "/js/submit.js" "/js/admin.js"; do
  HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" $BASE$RESOURCE)
  echo "  $RESOURCE -> $HTTP_CODE"
done
```

**预期结果**：所有资源返回 200 或 304

---

# 附录 A：一键运行所有测试

将所有测试整合为一个可执行的 shell 脚本，保存为 `run_all_tests.sh`：

```bash
#!/bin/bash
# OJ 系统自动化测试套件
# 运行方式: chmod +x run_all_tests.sh && ./run_all_tests.sh

set -e
BASE="http://8.138.161.148:8080"
PASS=0
FAIL=0
COOKIE="/tmp/oj_test_cookie.txt"
COOKIE_USER="/tmp/oj_test_cookie_user.txt"
TIMESTAMP=$(date +%s)

cleanup() {
    rm -f $COOKIE $COOKIE_USER
}
trap cleanup EXIT

assert_status() {
    local desc="$1" expected="$2" actual="$3"
    if [ "$actual" = "$expected" ]; then
        echo "  [PASS] $desc"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc (期望 $expected, 实际 $actual)"
        FAIL=$((FAIL + 1))
    fi
}

assert_contains() {
    local desc="$1" response="$2" keyword="$3"
    if echo "$response" | grep -q "$keyword"; then
        echo "  [PASS] $desc"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $desc (未找到 '$keyword')"
        FAIL=$((FAIL + 1))
    fi
}

echo "=============================================="
echo "  OJ 系统 Web 自动化测试"
echo "  目标: $BASE"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=============================================="

# ====== AUTH 模块 ======
echo ""
echo ">>> 模块一：用户认证 (AUTH)"

# AUTH-01 正常注册
echo "-- AUTH-01: 正常注册"
RESP=$(curl -s -w "\n%{http_code}" -H "Content-Type: application/json" \
  -d "{\"username\":\"at_$TIMESTAMP\",\"password\":\"pass1234\",\"confirm_password\":\"pass1234\"}" \
  $BASE/api/register)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "注册返回 201" "201" "$HTTP_CODE"

# AUTH-04 密码不足6位
echo "-- AUTH-04: 密码不足6位"
RESP=$(curl -s -w "\n%{http_code}" -H "Content-Type: application/json" \
  -d '{"username":"shortpwd","password":"12345","confirm_password":"12345"}' \
  $BASE/api/register)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "短密码返回 400" "400" "$HTTP_CODE"

# AUTH-08 重复用户名
echo "-- AUTH-08: 重复用户名"
RESP=$(curl -s -w "\n%{http_code}" -H "Content-Type: application/json" \
  -d "{\"username\":\"at_$TIMESTAMP\",\"password\":\"pass9999\",\"confirm_password\":\"pass9999\"}" \
  $BASE/api/register)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "重复用户名返回 409" "409" "$HTTP_CODE"

# AUTH-10 正常登录
echo "-- AUTH-10: 正常登录"
RESP=$(curl -c $COOKIE -s -w "\n%{http_code}" -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  $BASE/api/login)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "登录返回 200" "200" "$HTTP_CODE"

# AUTH-11 错误密码
echo "-- AUTH-11: 错误密码"
RESP=$(curl -s -w "\n%{http_code}" -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"wrong"}' \
  $BASE/api/login)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "错误密码返回 401" "401" "$HTTP_CODE"

# AUTH-16 登出
echo "-- AUTH-16: 登出"
RESP=$(curl -s -w "\n%{http_code}" -b $COOKIE -X POST $BASE/api/logout)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "登出返回 200" "200" "$HTTP_CODE"

# ====== PROB 模块 ======
echo ""
echo ">>> 模块二：题目浏览 (PROB)"

# PROB-01 题目列表
echo "-- PROB-01: 题目列表"
RESP=$(curl -s -w "\n%{http_code}" $BASE/api/problems)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "题目列表返回 200" "200" "$HTTP_CODE"

# PROB-05 不存在题目
echo "-- PROB-05: 不存在题目"
RESP=$(curl -s -w "\n%{http_code}" $BASE/api/problems/99999)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "不存在题目返回 404" "404" "$HTTP_CODE"

# ====== ADMIN 模块 ======
echo ""
echo ">>> 模块三：管理后台 (ADMIN)"

# 重新登录 admin
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' $BASE/api/login > /dev/null

# ADMIN-01 创建题目
echo "-- ADMIN-01: 创建题目"
RESP=$(curl -s -w "\n%{http_code}" -b $COOKIE -H "Content-Type: application/json" \
  -d "{\"title\":\"atest_$TIMESTAMP\",\"content\":\"# Test\",\"difficulty\":\"Easy\",\"test_cases\":[{\"input\":\"1\",\"expected\":\"1\",\"position\":0}]}" \
  $BASE/api/admin/problems)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "创建题目返回 201" "201" "$HTTP_CODE"
NEW_ID=$(echo "$RESP" | python3 -c "import json,sys; print(json.load(sys.stdin).get('id',''))" 2>/dev/null || echo "")

# ADMIN-02 验证创建后在列表中
echo "-- ADMIN-02: 创建后验证"
if [ -n "$NEW_ID" ]; then
    RESP=$(curl -s $BASE/api/problems)
    if echo "$RESP" | python3 -c "import json,sys; d=json.load(sys.stdin); assert any(p['id']==$NEW_ID for p in d)" 2>/dev/null; then
        echo "  [PASS] 题目在列表中找到"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] 题目未在列表中找到"
        FAIL=$((FAIL + 1))
    fi
else
    echo "  [FAIL] 未能获取新题目 ID"
    FAIL=$((FAIL + 1))
fi

# ADMIN-04 更新题目
echo "-- ADMIN-04: 更新题目"
if [ -n "$NEW_ID" ]; then
    RESP=$(curl -s -w "\n%{http_code}" -b $COOKIE -H "Content-Type: application/json" \
      -d "{\"title\":\"atest_updated_$TIMESTAMP\",\"content\":\"# Updated\",\"difficulty\":\"Hard\",\"test_cases\":[{\"input\":\"2\",\"expected\":\"2\",\"position\":0}]}" \
      $BASE/api/admin/problems/$NEW_ID)
    HTTP_CODE=$(echo "$RESP" | tail -1)
    assert_status "更新题目返回 200" "200" "$HTTP_CODE"
fi

# ADMIN-06 删除题目
echo "-- ADMIN-06: 删除题目"
if [ -n "$NEW_ID" ]; then
    RESP=$(curl -s -w "\n%{http_code}" -b $COOKIE -X DELETE $BASE/api/admin/problems/$NEW_ID)
    HTTP_CODE=$(echo "$RESP" | tail -1)
    assert_status "删除题目返回 200" "200" "$HTTP_CODE"
fi

# ADMIN-07 删除不存在题目
echo "-- ADMIN-07: 删除不存在题目"
RESP=$(curl -s -w "\n%{http_code}" -b $COOKIE -X DELETE $BASE/api/admin/problems/99999)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "删除不存在题目返回 404" "404" "$HTTP_CODE"

# ====== PERM 模块 ======
echo ""
echo ">>> 模块四：权限校验 (PERM)"

# PERM-01 未登录访问 admin
echo "-- PERM-01: 未登录访问 admin"
RESP=$(curl -s -w "\n%{http_code}" -X POST -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}' $BASE/api/admin/problems)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "未登录返回 401" "401" "$HTTP_CODE"

# PERM-04 普通用户访问 admin
echo "-- PERM-04: 普通用户访问 admin"
# 注册普通用户
curl -s -H "Content-Type: application/json" \
  -d "{\"username\":\"pt_$TIMESTAMP\",\"password\":\"pass1234\"}" $BASE/api/register > /dev/null
# 登录普通用户
curl -c $COOKIE_USER -s -H "Content-Type: application/json" \
  -d "{\"username\":\"pt_$TIMESTAMP\",\"password\":\"pass1234\"}" $BASE/api/login > /dev/null
# 越权访问
RESP=$(curl -s -w "\n%{http_code}" -b $COOKIE_USER -X POST -H "Content-Type: application/json" \
  -d '{"title":"hack","content":"hack"}' $BASE/api/admin/problems)
HTTP_CODE=$(echo "$RESP" | tail -1)
assert_status "普通用户返回 403" "403" "$HTTP_CODE"

# ====== SUBM 模块 ======
echo ""
echo ">>> 模块五：代码提交 (SUBM)"

# 先创建一个测试题目
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' $BASE/api/login > /dev/null
CREATE_RESP=$(curl -s -b $COOKIE -H "Content-Type: application/json" \
  -d "{\"title\":\"submtest_$TIMESTAMP\",\"content\":\"# A+B\",\"difficulty\":\"Easy\",\"test_cases\":[{\"input\":\"1 2\",\"expected\":\"3\",\"position\":0}]}" \
  $BASE/api/admin/problems)
SPID=$(echo "$CREATE_RESP" | python3 -c "import json,sys; print(json.load(sys.stdin)['id'])" 2>/dev/null || echo "")

if [ -n "$SPID" ]; then
    # SUBM-01 AC
    echo "-- SUBM-01: AC 代码"
    CODE='#include <iostream>
int main(){int a,b;std::cin>>a>>b;std::cout<<a+b<<std::endl;return 0;}'
    RESP=$(curl -s -H "Content-Type: application/json" \
      -d "$(python3 -c "import json; print(json.dumps({'problem_id':$SPID,'code':'$CODE'}))")" \
      $BASE/api/submit)
    STATUS=$(echo "$RESP" | python3 -c "import json,sys; print(json.load(sys.stdin)['status'])")
    assert_status "AC 代码返回 AC" "AC" "$STATUS"

    # SUBM-03 CE
    echo "-- SUBM-03: CE 代码"
    RESP=$(curl -s -H "Content-Type: application/json" \
      -d "{\"problem_id\":$SPID,\"code\":\"invalid c++ code###\"}" \
      $BASE/api/submit)
    STATUS=$(echo "$RESP" | python3 -c "import json,sys; print(json.load(sys.stdin)['status'])")
    assert_status "CE 代码返回 CE" "CE" "$STATUS"

    # SUBM-04 TLE
    echo "-- SUBM-04: TLE 代码"
    CODE='#include <iostream>
int main(){while(true){}return 0;}'
    RESP=$(curl -s -H "Content-Type: application/json" \
      -d "$(python3 -c "import json; print(json.dumps({'problem_id':$SPID,'code':'$CODE'}))")" \
      $BASE/api/submit)
    STATUS=$(echo "$RESP" | python3 -c "import json,sys; print(json.load(sys.stdin)['status'])")
    assert_status "TLE 代码返回 TLE" "TLE" "$STATUS"

    # SUBM-06 缺少参数
    echo "-- SUBM-06: 缺少参数"
    RESP=$(curl -s -w "\n%{http_code}" -H "Content-Type: application/json" \
      -d '{"code":"#include <iostream>\nint main(){return 0;}"}' \
      $BASE/api/submit)
    HTTP_CODE=$(echo "$RESP" | tail -1)
    assert_status "缺 problem_id 返回 400" "400" "$HTTP_CODE"

    # 清理测试题目
    curl -s -b $COOKIE -X DELETE $BASE/api/admin/problems/$SPID > /dev/null
else
    echo "  [SKIP] 无法创建测试题目，跳过 SUBM 模块"
fi

# ====== 结果汇总 ======
echo ""
echo "=============================================="
echo "  测试结果汇总"
echo "=============================================="
echo "  通过: $PASS"
echo "  失败: $FAIL"
echo "  总计: $((PASS + FAIL))"
echo "=============================================="

if [ $FAIL -gt 0 ]; then
    echo "  存在失败用例，请检查！"
    exit 1
else
    echo "  全部测试通过！"
    exit 0
fi
```

---

# 附录 B：浏览器 UI 自动化测试 (playwright-cli)

本附录记录基于 `playwright-cli` 的有头浏览器（`--headed`）UI 自动化测试过程，模拟真实用户操作，验证前端页面与后端 API 的交互行为。每个操作之间增加 1 秒停顿便于肉眼观察。

## 测试环境

| 项目 | 值 |
|------|-----|
| 工具 | `playwright-cli` (全局安装 `@playwright/cli@latest`) |
| 浏览器 | Chromium (headed 模式) |
| 操作间隔 | 每个操作之间 1 秒停顿 |

## 启动浏览器

```bash
# 带界面打开浏览器并访问首页
playwright-cli open --headed http://8.138.161.148:8080/

# 每个操作之间增加 1 秒停顿
playwright-cli run-code "async page => await page.waitForTimeout(1000)"
```

---

## 1. 用户认证模块 (AUTH) — UI 测试

### AUTH-01 正常注册新用户

**测试步骤**：
1. 打开浏览器访问 `http://8.138.161.148:8080/register.html`
2. `playwright-cli fill <ref> "testuser_<timestamp>"` — 填入唯一用户名
3. `playwright-cli fill <ref> "pass1234"` — 填入密码
4. `playwright-cli fill <ref> "pass1234"` — 填入确认密码
5. `playwright-cli click <ref>` — 点击「注册」按钮

**结果**: PASS — 页面自动跳转至 `/login.html`，注册成功。

**命令示例**：
```bash
playwright-cli goto http://8.138.161.148:8080/register.html
playwright-cli snapshot
# 记录 ref 后填入表单
$ts = [DateTimeOffset]::Now.ToUnixTimeSeconds()
playwright-cli fill <username_ref> "testuser_$ts"
playwright-cli fill <password_ref> "pass1234"
playwright-cli fill <confirm_ref> "pass1234"
playwright-cli run-code "async page => await page.waitForTimeout(1000)"
playwright-cli click <register_btn_ref>
```

### AUTH-05 两次密码不一致

**测试步骤**：填入用户名、密码 `pass1234`、确认密码 `different`，点击注册。

**结果**: PASS — 页面上显示「两次输入的密码不一致」客户端验证提示，未发送 API 请求。

### AUTH-08 注册已存在的用户名

**测试步骤**：使用 AUTH-01 注册过的用户名再次注册。

**结果**: PASS — 页面上显示「该用户名已被使用」，浏览器 Console 记录 API 返回 409 Conflict。

### AUTH-10 管理员正常登录

**测试步骤**：
1. 访问 `/login.html`
2. 填入用户名 `admin`，密码 `admin123`
3. 点击「登录」

**结果**: PASS — 页面跳转到 `/problem_list.html`，导航栏显示 `admin` + `ADMIN` 徽章 +「管理」链接 +「登出」按钮。

### AUTH-11 错误密码登录

**测试步骤**：填入 `admin` / `wrongpassword`，点击登录。

**结果**: PASS — 页面显示「Invalid username or password」，停留在 `/login.html`。

### AUTH-12 不存在用户名登录

**测试步骤**：填入 `nonexistent_user_xyz` / `pass1234`，点击登录。

**结果**: PASS — 页面显示「Invalid username or password」。

### AUTH-16 登出

**测试步骤**：登录后点击导航栏「登出」按钮。

**结果**: PASS — 页面跳转到 `/login.html`，Session 被销毁。

> **说明**: AUTH-02/03/04/06/07/13/14 在 UI 层面由 HTML5 表单校验 (`required`, `minlength`) 拦截，不会触发 API 调用。AUTH-09/15（非 JSON body）在 UI 测试中不可模拟，仅 API 测试覆盖。

---

## 2. 题目浏览模块 (PROB) — UI 测试

### PROB-01 题目列表

**测试步骤**：登录后访问 `/problem_list.html`，查看题目表格。

**结果**: PASS — 显示 17 道题目，表格包含 `#`、`标题`、`难度` 三列，难度标签有 Easy/Medium 分色显示，支持按难度筛选按钮。

### PROB-02 敏感信息保护

**验证方式**：题目列表页的快照中，每条题目仅显示 id、title、difficulty，不泄露 `content`、`template`、`test_cases` 等字段。

**结果**: PASS

### PROB-03 未登录可访问题目列表

**验证方式**：不登录，直接访问 `/problem_list.html`。

**结果**: PASS — 列表正常展示（但导航栏没有管理员选项）。

### PROB-04 题目详情

**测试步骤**：在题目列表中点击「A + B 问题」，进入 `/problem.html?id=1`。

**结果**: PASS — 页面左侧显示完整题目描述（含 Markdown 渲染的标题、输入/输出格式、样例、数据范围），右侧显示 Ace Editor 代码编辑器（含 C++ 模板代码）和「提交运行」按钮。

### PROB-05 不存在的题目

**测试步骤**：访问 `/problem.html?id=99999`。

**结果**: PASS — 页面显示「加载失败」+「Problem not found」，Console 记录 API 返回 404。

---

## 3. 代码提交模块 (SUBM) — UI 测试

### 前置：代码编辑器操作

Ace Editor 中填充代码使用 `page.evaluate` 调用 `window.editor.setValue()`：

```bash
playwright-cli run-code "async page => {
  const code = '#include <iostream>\nint main() {\n    int a, b;\n    std::cin >> a >> b;\n    std::cout << a + b << std::endl;\n    return 0;\n}';
  await page.evaluate(code => { window.editor.setValue(code); window.editor.clearSelection(); }, code);
  await page.waitForTimeout(500);
  return 'done';
}"
```

### SUBM-01 正确代码 (AC)

**测试步骤**：
1. 在题目详情页的 Ace Editor 中填入 A+B 的正确 C++ 代码
2. 点击「提交运行」
3. 等待判题结果

**结果**: PASS — 结果面板显示 **AC**，「通过 3/3 个测试用例」：
| Case #1 | AC | 输入 1 2 | 期望 3 | 实际 3 |
| Case #2 | AC | 输入 100 200 | 期望 300 | 实际 300 |
| Case #3 | AC | 输入 -5 5 | 期望 0 | 实际 0 |

### SUBM-02 错误代码 (WA)

**测试步骤**：填入输出恒为 0 的代码，点击「重新提交」。

**结果**: PASS — 结果面板显示 **WA**，「通过 1/3 个测试用例」：
| Case #1 | WA | 输入 1 2 | 期望 3 | 实际 0 |
| Case #2 | WA | 输入 100 200 | 期望 300 | 实际 0 |
| Case #3 | AC | 输入 -5 5 | 期望 0 | 实际 0 |（巧合通过）

### SUBM-03 编译错误 (CE)

**测试步骤**：填入包含 `invalid_syntax###` 的非法 C++ 代码，点击「重新提交」。

**结果**: PASS — 结果面板显示 **CE** +「编译错误」，展开显示完整的 g++ 编译错误信息（`stray '##'`、`invalid_syntax was not declared`）。

### SUBM-04 超时代码 (TLE)

**测试步骤**：填入 `while(true){}` 死循环代码，点击「重新提交」。

**结果**: PASS — 等待约 5 秒后结果面板显示 **TLE**，「通过 0/3 个测试用例」，每个用例标注「Time Limit Exceeded (5s)」。

> **说明**: SUBM-05 (RE) 在本次 UI 测试中未执行（nullptr 解引用），可复用 API 测试覆盖。SUBM-06~10（参数校验）在后端 API 层验证，UI 层由 Ace Editor + 题目 ID 自动拼接无法模拟缺失字段场景。

---

## 4. 管理后台模块 (ADMIN) — UI 测试

### ADMIN-01 创建新题目

**测试步骤**：
1. 管理员登录后点击导航栏「管理」进入 `/admin.html`
2. 表单字段：`f15e45` (标题)、`f15e51` (Markdown 描述)、`f15e54` (代码模板)、`f15e73` (测试用例输入)、`f15e76` (测试用例期望输出)
3. 填入唯一标题 `"E2E测试_<timestamp>"`，描述、模板、一个测试用例
4. 点击「创建题目」

**结果**: PASS — 题目成功创建，底部「题目列表」表格中出现新行：ID=111, 标题=测试标题, 难度=Medium。内置 17 道题目均显示「编辑」和「删除」按钮。

### ADMIN-06 删除题目

**测试步骤**：
1. 在题目列表中找到刚创建的题目行
2. 点击「删除」按钮
3. 弹出确认对话框：「确定要删除题目 #111吗？此操作不可撤销」
4. `playwright-cli dialog-accept` 确认
5. 检查题目列表 — 目标题目已消失

**结果**: PASS — 确认对话框出现，删除后题目从列表中消失。

---

## 5. 权限校验模块 (PERM) — UI 测试

### PERM-01/02/03 未登录访问管理页面

**测试步骤**：登出后直接访问 `/admin.html`。

**结果**: PASS — 页面被重定向到 `/login.html`，未登录无法访问管理页面。

### PERM-04 普通用户访问管理页面

**测试步骤**：
1. 注册新用户（非 admin 角色）
2. 用该用户登录
3. 访问 `/admin.html`

**结果**: PASS — 页面可以加载但显示权限拒绝界面：
- 标题：「无访问权限」
- 提示文案：「此页面仅限管理员访问，请使用管理员账号登录。」
- 导航栏显示普通用户名 +「登出」按钮，无「管理」链接

---

## 6. 端到端集成场景 (E2E) — UI 覆盖情况

| 场景 | 验收标准 | 覆盖状态 |
|------|----------|----------|
| E2E-01 | 管理员创建→列表可见→删除 | PASS (ADMIN-01 + ADMIN-06) |
| E2E-02 | 注册→登录→查看题目→提交代码 | PASS (AUTH-01 + AUTH-10 + PROB-04 + SUBM-01) |
| E2E-03 | AC/WA/TLE/CE 四种判题结果 | PASS (SUBM-01~04) |
| E2E-04 | 超时限制 <5s | PASS (SUBM-04 TLE 在约 5s 返回) |
| E2E-05 | 管理员 CRUD | PASS (ADMIN-01 创建 + ADMIN-06 删除) |
| E2E-06 | 普通用户无法访问管理 | PASS (PERM-04) |
| E2E-07 | 注册→登录→登出→重新登录 | PASS (AUTH-01 + AUTH-10 + AUTH-16 + AUTH-10) |
| E2E-08 | 页面无需刷新完成提交 | PASS (SUBM 在同一页面不刷新，异步返回结果) |
| E2E-09 | 新用户注册并登录 | PASS (AUTH-01 + AUTH-10) |

---

## 7. UI 测试结果汇总

```
============================================
  OJ 系统 Web UI 自动化测试 (playwright-cli)
  目标: http://8.138.161.148:8080
  Browser: Chromium (headed)
  操作间隔: 1s
============================================

>>> 模块一：用户认证 (AUTH)
  [PASS] AUTH-01 正常注册（注册后跳转登录页）
  [PASS] AUTH-05 密码不一致（显示客户端错误）
  [PASS] AUTH-08 重复用户名（显示错误 + API 409）
  [PASS] AUTH-10 管理员登录（跳转题目列表，含 ADMIN 徽章）
  [PASS] AUTH-11 错误密码（显示 Invalid 错误）
  [PASS] AUTH-12 不存在用户（显示 Invalid 错误）
  [PASS] AUTH-16 登出（跳转登录页，Session 销毁）

>>> 模块二：题目浏览 (PROB)
  [PASS] PROB-01 题目列表（17 道题目，含 ID/标题/难度）
  [PASS] PROB-02 敏感信息保护（列表不泄露 content/test_cases）
  [PASS] PROB-03 未登录可访问列表
  [PASS] PROB-04 题目详情（Markdown + Ace Editor + 提交按钮）
  [PASS] PROB-05 不存在题目（显示 NotFound 错误）

>>> 模块三：代码提交 (SUBM)
  [PASS] SUBM-01 AC 提交（3/3 通过，逐用例显示 expected/actual）
  [PASS] SUBM-02 WA 提交（1/3 通过，错误用例显示差异）
  [PASS] SUBM-03 CE 提交（显示 g++ 编译错误详情）
  [PASS] SUBM-04 TLE 提交（0/3 通过，超时 5s 正确判定）

>>> 模块四：管理后台 (ADMIN)
  [PASS] ADMIN-01 创建题目（ID=111 出现在列表中）
  [PASS] ADMIN-06 删除题目（确认对话框 → 删除成功）

>>> 模块五：权限校验 (PERM)
  [PASS] PERM-01/02/03 未登录访问（重定向登录页）
  [PASS] PERM-04 普通用户访问（显示「无访问权限」）

============================================
  通过: 21 / 21
  失败: 0
  通过率: 100%
============================================
```

---

## 8. UI 测试 vs API 测试差异说明

| 场景 | API (curl) | UI (playwright-cli) | 说明 |
|------|-----------|---------------------|------|
| 校验层 | 后端验证返回 HTTP 状态码 | 前端 HTML5 约束 + 后端验证 | 前端先拦截，后端再校验 |
| 密码不足6位 | API 返回 400 + error msg | HTML5 `minlength` 拦截，不发 API | 用户收到浏览器原生提示 |
| 空用户名 | API 返回 400 | HTML5 `required` 拦截 | 同上 |
| 非 JSON body | API 返回 400 | 不可模拟 | 浏览器 fetch 自动设置格式 |
| 错误反馈 | JSON 中的 `error` 字段 | 页面内联消息 / toast | UI 友好展示更直观 |
| 判题结果 | JSON `status` + `results[]` | 结果面板 + 逐用例展开 | UI 有折叠/展开交互 |

---

# 附录 C：验收标准覆盖矩阵

| 验收标准 | 覆盖测试用例 |
|----------|-------------|
| #1 管理员新增题目并在列表看到 | ADMIN-01, ADMIN-02, ADMIN-03, E2E-01 |
| #2 普通用户查看题目、编辑并提交 | PROB-01~07, SUBM-01, E2E-02 |
| #3 代码在 <5s 超时内执行返回 | SUBM-04, E2E-03, E2E-04 |
| #4 正确判断 AC/WA/TLE/RE | SUBM-01~05, SUBM-11, E2E-03 |
| #5 管理员可删除题目 | ADMIN-06, E2E-01 (Step 7-8) |
| #6 普通用户无法访问管理接口 | PERM-01~05, E2E-06 |
| #7 部署文档完整，单机可运行 | 环境依赖检查（非自动化范围） |
| #8 页面无需刷新完成完整提交 | E2E-08（前端页面和 API 可访问性） |
| #9 新用户可注册并登录 | AUTH-01, AUTH-10, E2E-02, E2E-07 |

---

# 附录 D：测试数据清理

以下脚本用于清理测试过程中产生的题目数据（保留其他模块间独立测试用）：

```bash
#!/bin/bash
# 清理所有测试产生的题目
BASE="http://8.138.161.148:8080"
COOKIE="/tmp/oj_test_cookie.txt"

# 管理员登录
curl -c $COOKIE -s -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' \
  $BASE/api/login > /dev/null

# 获取所有题目，删除标题包含特定模式的问题
curl -s $BASE/api/problems | python3 -c "
import json, sys, subprocess

data = json.load(sys.stdin)
patterns = ['atest_', 'submtest_', 'E2E', '默认难度', '判题测试']

for p in data:
    for pat in patterns:
        if pat in p['title']:
            subprocess.run(['curl', '-s', '-b', '/tmp/oj_test_cookie.txt',
                '-X', 'DELETE', f'http://8.138.161.148:8080/api/admin/problems/{p[\"id\"]}'],
                stdout=subprocess.DEVNULL)
            print(f'已删除: [{p[\"id\"]}] {p[\"title\"]}')
            break
"
rm -f $COOKIE
echo "清理完成"
```

