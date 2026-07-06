# OJ API 接口自动化测试文档 (基于 curl)

## 前置条件

1. MySQL 已启动并可连接（socket: `/tmp/mysql.sock`）
2. 数据库 `cpp_oj` 已初始化，表结构就绪
3. OJ Server 已编译并启动，监听 `127.0.0.1:8080`

### 快速启动命令

```bash
# 启动 MySQL（如未启动）
/home/dzh/.local/mysql/sbin/mysqld --no-defaults \
  --basedir=/home/dzh/.local/mysql \
  --datadir=/home/dzh/.local/mysql/data \
  --socket=/tmp/mysql.sock --port=3306 --user=dzh &

# 编译并启动 OJ Server
cd /home/dzh/project/cpp-oj-vbcoding-teach
mkdir -p build && cd build && cmake .. && make -j$(nproc)
cd ..
pkill -9 oj_server 2>/dev/null
nohup ./build/oj_server > /tmp/oj_server.log 2>&1 &

# 验证服务就绪
curl -s http://127.0.0.1:8080/api/problems
# 应返回: []
```

---

## 接口测试总览

| # | 接口 | 测试项 | 预期 HTTP |
|---|------|--------|-----------|
| 1 | `POST /api/register` | 正常注册 + 异常场景 | 201/400/409 |
| 2 | `POST /api/login` | 登录 + 权限验证 | 200/400/401 |
| 3 | `POST /api/logout` | 登出 + session 失效 | 200/401 |
| 4 | `GET /api/problems` | 题目列表 | 200 |
| 5 | `GET /api/problems/:id` | 题目详情 | 200/404 |
| 6 | `POST /api/submit` | 代码提交 (AC/WA/CE) | 200/400 |
| 7 | `POST /api/admin/problems` | 管理员创建题目 | 201/400/401/403 |
| 8 | `DELETE /api/admin/problems/:id` | 管理员删除题目 | 200/401/403/404 |

---

## 完整测试脚本

将以下内容保存为 `api_test.sh` 并执行 `bash api_test.sh`：

```bash
#!/bin/bash
BASE="http://127.0.0.1:8080"
OK=0; FAIL=0
COOKIE_FILE=$(mktemp)

# ── 工具函数 ──
check_http() {
  local label="$1"; local expect_code="$2"; shift 2
  local code=$(curl -s -o /tmp/oj_resp.txt -w "%{http_code}" "$@")
  if [ "$code" = "$expect_code" ]; then
    echo "  [PASS] $label (HTTP $code)"
    OK=$((OK+1))
  else
    echo "  [FAIL] $label (expected $expect_code, got HTTP $code)"
    echo "  Body: $(cat /tmp/oj_resp.txt)"
    FAIL=$((FAIL+1))
  fi
}

check_body() {
  local label="$1"; local expect="$2"; shift 2
  local code=$(curl -s -o /tmp/oj_resp.txt -w "%{http_code}" "$@")
  local body=$(cat /tmp/oj_resp.txt)
  if echo "$body" | grep -q "$expect"; then
    echo "  [PASS] $label (HTTP $code)"
    OK=$((OK+1))
  else
    echo "  [FAIL] $label (body does NOT contain '$expect')"
    echo "  HTTP $code Body: $body"
    FAIL=$((FAIL+1))
  fi
}

# ── 环境准备 ──
echo ">>> 清理数据库并重置自增 ID ..."
mysql -u root cpp_oj -e "
  DELETE FROM sessions;
  DELETE FROM test_cases;
  DELETE FROM problems;
  DELETE FROM users;
  ALTER TABLE problems AUTO_INCREMENT = 1;
  ALTER TABLE test_cases AUTO_INCREMENT = 1;
  ALTER TABLE users AUTO_INCREMENT = 1;
" 2>/dev/null

echo ">>> 验证服务器连通性 ..."
if curl -s -o /dev/null -w "%{http_code}" "$BASE/api/problems" | grep -q 200; then
  echo "  Server OK"
else
  echo "  ERROR: Server not reachable at $BASE"
  exit 1
fi

echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║              OJ API 接口自动化测试                       ║"
echo "╚══════════════════════════════════════════════════════════╝"

# ═══════════════════════════════════════════
# 1. 用户注册 POST /api/register
# ═══════════════════════════════════════════
echo ""; echo "━━━ 1. POST /api/register ━━━"
check_http "1.1 正常注册普通用户" "201" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"pass123","confirm_password":"pass123"}'
check_http "1.2 正常注册管理员账号" "201" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123","confirm_password":"admin123"}'
check_http "1.3 重复用户名 (409)" "409" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"pass123"}'
check_http "1.4 密码太短 (400)" "400" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"u1","password":"12"}'
check_http "1.5 缺失 password (400)" "400" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"bob"}'
check_http "1.6 密码不匹配 (400)" "400" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"charlie","password":"pass123","confirm_password":"diff"}'
check_http "1.7 用户名为空 (400)" "400" \
  -X POST $BASE/api/register -H "Content-Type: application/json" \
  -d '{"username":"","password":"pass123"}'

# 升级 admin 为管理员角色
mysql -u root cpp_oj -e "UPDATE users SET role='admin' WHERE username='admin';" 2>/dev/null
echo "  >> admin 已升级为管理员角色"

# ═══════════════════════════════════════════
# 2. 用户登录 POST /api/login
# ═══════════════════════════════════════════
echo ""; echo "━━━ 2. POST /api/login ━━━"

# 登录并保存 cookie
echo -n "  >> 登录 alice (普通用户)... "
curl -s -c /tmp/oj_student_cookie -X POST $BASE/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"pass123"}' | python3 -c "import sys,json;d=json.load(sys.stdin);print(f'id={d[\"id\"]} role={d[\"role\"]}')"

echo -n "  >> 登录 admin (管理员)... "
curl -s -c /tmp/oj_admin_cookie -X POST $BASE/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' | python3 -c "import sys,json;d=json.load(sys.stdin);print(f'id={d[\"id\"]} role={d[\"role\"]}')"

check_http "2.1 错误密码 (401)" "401" \
  -X POST $BASE/api/login -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"bad_pass"}'
check_http "2.2 不存在用户 (401)" "401" \
  -X POST $BASE/api/login -H "Content-Type: application/json" \
  -d '{"username":"ghost_user","password":"x"}'
check_http "2.3 缺失 password (400)" "400" \
  -X POST $BASE/api/login -H "Content-Type: application/json" \
  -d '{"username":"admin"}'
check_http "2.4 缺失 username (400)" "400" \
  -X POST $BASE/api/login -H "Content-Type: application/json" \
  -d '{"password":"admin123"}'

# ═══════════════════════════════════════════
# 3. 管理员创建题目 POST /api/admin/problems
# ═══════════════════════════════════════════
echo ""; echo "━━━ 3. POST /api/admin/problems (管理员创建题目) ━━━"

check_http "3.1 未登录创建 (401)" "401" \
  -X POST $BASE/api/admin/problems -H "Content-Type: application/json" \
  -d '{"title":"x","content":"y"}'
check_http "3.2 普通用户创建 (403)" "403" \
  -b /tmp/oj_student_cookie -X POST $BASE/api/admin/problems \
  -H "Content-Type: application/json" -d '{"title":"x","content":"y"}'

# 创建题目 A+B Problem (捕获 ID)
PID1=$(curl -s -b /tmp/oj_admin_cookie -X POST $BASE/api/admin/problems \
  -H "Content-Type: application/json" -d '{
    "title": "A+B Problem",
    "difficulty": "Easy",
    "content": "## Description\nGiven two integers A and B, output their sum.\n\n## Input\nTwo space-separated integers.\n\n## Output\nA single integer, the sum.",
    "template": "#include <iostream>\nint main() {\n  int a, b;\n  std::cin >> a >> b;\n  std::cout << a + b;\n  return 0;\n}",
    "test_cases": [
      {"input": "1 2", "expected": "3", "position": 0},
      {"input": "100 200", "expected": "300", "position": 1},
      {"input": "-5 5", "expected": "0", "position": 2}
    ]
  }' | grep -oP '"id":\d+' | grep -oP '\d+')
echo "  >> A+B Problem created, id=$PID1"
check_http "3.3 管理员创建 (已有PID1)" "201" \
  -b /tmp/oj_admin_cookie -X POST $BASE/api/admin/problems \
  -H "Content-Type: application/json" \
  -d '{"title":"dummy","content":"verify 201 status code"}'

# 创建题目 Fibonacci (捕获 ID)
PID2=$(curl -s -b /tmp/oj_admin_cookie -X POST $BASE/api/admin/problems \
  -H "Content-Type: application/json" -d '{
    "title": "Fibonacci Number",
    "difficulty": "Medium",
    "content": "## Description\nOutput the N-th Fibonacci number (0-indexed).\nF(0)=0, F(1)=1.",
    "template": "#include <iostream>\nint main() {\n  int n;\n  std::cin >> n;\n  std::cout << 0;\n  return 0;\n}",
    "test_cases": [
      {"input": "0", "expected": "0", "position": 0},
      {"input": "1", "expected": "1", "position": 1},
      {"input": "10", "expected": "55", "position": 2}
    ]
  }' | grep -oP '"id":\d+' | grep -oP '\d+')
echo "  >> Fibonacci created, id=$PID2"

check_http "3.4 缺少 title (400)" "400" \
  -b /tmp/oj_admin_cookie -X POST $BASE/api/admin/problems \
  -H "Content-Type: application/json" -d '{"content":"x"}'
check_http "3.5 缺少 content (400)" "400" \
  -b /tmp/oj_admin_cookie -X POST $BASE/api/admin/problems \
  -H "Content-Type: application/json" -d '{"title":"x"}'

# ═══════════════════════════════════════════
# 4. 题目列表与详情
# ═══════════════════════════════════════════
echo ""; echo "━━━ 4. GET /api/problems & GET /api/problems/:id ━━━"

check_body "4.1 列表包含 A+B" "A+B Problem" $BASE/api/problems
check_body "4.2 列表包含 Fib" "Fibonacci" $BASE/api/problems
check_body "4.3 A+B详情含 test_cases" '"test_cases"' $BASE/api/problems/$PID1
check_http "4.4 不存在题目 (404)" "404" $BASE/api/problems/99999

# ═══════════════════════════════════════════
# 5. 代码提交 POST /api/submit
# ═══════════════════════════════════════════
echo ""; echo "━━━ 5. POST /api/submit (代码提交执行) ━━━"

check_body "5.1 AC-全部通过" '"status":"AC"' -X POST $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PID1,\"code\":\"#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<a+b;return 0;}\"}"

check_body "5.2 WA-答案错误" '"status":"WA"' -X POST $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PID1,\"code\":\"#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<0;return 0;}\"}"

check_body "5.3 CE-编译错误" '"status":"CE"' -X POST $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PID1,\"code\":\"this is not valid C++ code !!! @@@\"}"

check_body "5.4 Fib-AC" '"status":"AC"' -X POST $BASE/api/submit \
  -H "Content-Type: application/json" \
  -d "{\"problem_id\":$PID2,\"code\":\"#include <iostream>\nint main(){int n;std::cin>>n;if(n==0){std::cout<<0;return 0;}if(n==1){std::cout<<1;return 0;}long long a=0,b=1,t;for(int i=2;i<=n;i++){t=a+b;a=b;b=t;}std::cout<<b;return 0;}\"}"

check_http "5.5 缺 problem_id (400)" "400" -X POST $BASE/api/submit \
  -H "Content-Type: application/json" -d '{"code":"int main(){return 0;}"}'
check_http "5.6 缺 code (400)" "400" -X POST $BASE/api/submit \
  -H "Content-Type: application/json" -d "{\"problem_id\":$PID1}"
check_http "5.7 code 为空 (400)" "400" -X POST $BASE/api/submit \
  -H "Content-Type: application/json" -d "{\"problem_id\":$PID1,\"code\":\"\"}"

# ═══════════════════════════════════════════
# 6. 管理员删除题目 DELETE /api/admin/problems/:id
# ═══════════════════════════════════════════
echo ""; echo "━━━ 6. DELETE /api/admin/problems/:id (管理员删除) ━━━"

check_http "6.1 未认证删除 (401)" "401" \
  -X DELETE $BASE/api/admin/problems/$PID1
check_http "6.2 普通用户删除 (403)" "403" \
  -b /tmp/oj_student_cookie -X DELETE $BASE/api/admin/problems/$PID1
check_http "6.3 删除不存在题目 (404)" "404" \
  -b /tmp/oj_admin_cookie -X DELETE $BASE/api/admin/problems/99999

# 正常删除 Fibonacci
check_body "6.4 管理员删除 Fib" "deleted successfully" \
  -b /tmp/oj_admin_cookie -X DELETE $BASE/api/admin/problems/$PID2

# 验证删除结果
echo -n "  >> 验证删除结果... "
LIST=$(curl -s $BASE/api/problems)
echo "$LIST"
if echo "$LIST" | grep -q "Fibonacci"; then
  echo "  [FAIL] 6.5 Fibonacci 仍存在于列表中"
  FAIL=$((FAIL+1))
else
  echo "  [PASS] 6.5 Fibonacci 已被删除(列表不含Fib)"
  OK=$((OK+1))
fi
check_http "6.6 Fib详情返回 404" "404" $BASE/api/problems/$PID2
check_body "6.7 A+B 仍然存在" "A+B Problem" $BASE/api/problems

# ═══════════════════════════════════════════
# 7. 登出 POST /api/logout
# ═══════════════════════════════════════════
echo ""; echo "━━━ 7. POST /api/logout ━━━"

check_body "7.1 管理员登出" "Logged out successfully" \
  -b /tmp/oj_admin_cookie -X POST $BASE/api/logout \
  -H "Content-Type: application/json" -d '{}'
check_http "7.2 登出后无法删除 (401)" "401" \
  -b /tmp/oj_admin_cookie -X DELETE $BASE/api/admin/problems/$PID1

# ═══════════════════════════════════════════
# 清理
# ═══════════════════════════════════════════
echo ""; echo ">>> 清理 dummy 数据 ..."
curl -s -c /tmp/oj_adm_clean -X POST $BASE/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}' > /dev/null
# 找出所有非 A+B 的题目 ID 并删除
curl -s $BASE/api/problems | python3 -c "
import sys,json
ids=[str(p['id']) for p in json.load(sys.stdin) if p['title']!='A+B Problem']
for i in ids: print(i)
" | while read id; do
  curl -s -b /tmp/oj_adm_clean -X DELETE $BASE/api/admin/problems/$id > /dev/null
  echo "  deleted id=$id"
done

# ═══════════════════════════════════════════
# 报告
# ═══════════════════════════════════════════
echo ""
echo "╔══════════════════════════════════════════════════════════╗"
echo "║  测试完成: $OK 通过, $FAIL 失败  (共 $((OK+FAIL)) 项)  ║"
echo "╚══════════════════════════════════════════════════════════╝"

[ $FAIL -eq 0 ] && exit 0 || exit 1
```

---

## 手动逐接口测试命令

以下是各接口的手动 curl 命令，可按需单独执行。假设管理员 cookie 已保存至 `/tmp/oj_admin_cookie`。

### 1. 用户注册

```bash
# 正常注册
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"pass123","confirm_password":"pass123"}'
# → 201 {"id":1,"message":"User registered successfully"}

# 重复用户名
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"pass123"}'
# → 409 {"error":"Username already taken"}

# 密码太短
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"u1","password":"12"}'
# → 400 {"error":"Password must be at least 6 characters"}

# 密码不匹配
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"bob","password":"pass123","confirm_password":"diff123"}'
# → 400 {"error":"Passwords do not match"}

# 缺失字段
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"charlie"}'
# → 400 {"error":"Missing or invalid 'password' field"}
```

### 2. 用户登录

```bash
# 管理员登录（保存 cookie）
curl -c /tmp/oj_admin_cookie -X POST http://127.0.0.1:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
# → 200 {"id":2,"username":"admin","role":"admin"}

# 普通用户登录
curl -c /tmp/oj_student_cookie -X POST http://127.0.0.1:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"alice","password":"pass123"}'
# → 200 {"id":1,"username":"alice","role":"user"}

# 错误密码
curl -X POST http://127.0.0.1:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"wrong"}'
# → 401 {"error":"Invalid username or password"}

# 不存在用户
curl -X POST http://127.0.0.1:8080/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"ghost","password":"x"}'
# → 401 {"error":"Invalid username or password"}
```

### 3. 管理员创建题目

```bash
# 创建题目（带测试用例）
curl -b /tmp/oj_admin_cookie -X POST http://127.0.0.1:8080/api/admin/problems \
  -H "Content-Type: application/json" \
  -d '{
    "title": "A+B Problem",
    "difficulty": "Easy",
    "content": "## Description\nGiven two integers A and B, output their sum.",
    "template": "#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<a+b;return 0;}",
    "test_cases": [
      {"input": "1 2", "expected": "3", "position": 0},
      {"input": "10 20", "expected": "30", "position": 1}
    ]
  }'
# → 201 {"id":1,"message":"Problem created successfully"}

# 普通用户创建（应被拒）
curl -b /tmp/oj_student_cookie -X POST http://127.0.0.1:8080/api/admin/problems \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}'
# → 403 {"error":"Admin privileges required"}

# 未登录创建（应被拒）
curl -X POST http://127.0.0.1:8080/api/admin/problems \
  -H "Content-Type: application/json" \
  -d '{"title":"test","content":"test"}'
# → 401 {"error":"Authentication required"}
```

### 4. 题目列表与详情

```bash
# 题目列表
curl http://127.0.0.1:8080/api/problems
# → 200 [{"id":1,"title":"A+B Problem","difficulty":"Easy"},...]

# 题目详情
curl http://127.0.0.1:8080/api/problems/1
# → 200 {"id":1,"title":"A+B Problem","content":"...","test_cases":[...]}

# 不存在的题目
curl http://127.0.0.1:8080/api/problems/99999
# → 404 {"error":"Problem not found"}
```

### 5. 代码提交执行

```bash
# AC - 正确代码
curl -X POST http://127.0.0.1:8080/api/submit \
  -H "Content-Type: application/json" \
  -d '{
    "problem_id": 1,
    "code": "#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<a+b;return 0;}"
  }'
# → 200 {"status":"AC","passed":2,"total":2,"results":[...]}

# WA - 错误答案
curl -X POST http://127.0.0.1:8080/api/submit \
  -H "Content-Type: application/json" \
  -d '{
    "problem_id": 1,
    "code": "#include <iostream>\nint main(){int a,b;std::cin>>a>>b;std::cout<<0;return 0;}"
  }'
# → 200 {"status":"WA","passed":0,"total":2,"results":[...]}

# CE - 编译错误
curl -X POST http://127.0.0.1:8080/api/submit \
  -H "Content-Type: application/json" \
  -d '{
    "problem_id": 1,
    "code": "not valid c++"
  }'
# → 200 {"status":"CE","passed":0,"total":0,"compile_error":"..."}
```

### 6. 管理员删除题目

```bash
# 删除题目
curl -b /tmp/oj_admin_cookie -X DELETE http://127.0.0.1:8080/api/admin/problems/1
# → 200 {"message":"Problem deleted successfully"}

# 验证已删除
curl http://127.0.0.1:8080/api/problems/1
# → 404 {"error":"Problem not found"}

# 普通用户删除（应被拒）
curl -b /tmp/oj_student_cookie -X DELETE http://127.0.0.1:8080/api/admin/problems/1
# → 403 {"error":"Admin privileges required"}
```

### 7. 登出

```bash
# 登出
curl -b /tmp/oj_admin_cookie -X POST http://127.0.0.1:8080/api/logout \
  -H "Content-Type: application/json" -d '{}'
# → 200 {"message":"Logged out successfully"}

# 登出后无法操作
curl -b /tmp/oj_admin_cookie -X DELETE http://127.0.0.1:8080/api/admin/problems/1
# → 401 {"error":"Authentication required"}
```

---

## 数据库重置命令

当需要完全重置测试环境时：

```bash
mysql -u root cpp_oj -e "
  DELETE FROM sessions;
  DELETE FROM test_cases;
  DELETE FROM problems;
  DELETE FROM users;
  ALTER TABLE problems AUTO_INCREMENT = 1;
  ALTER TABLE test_cases AUTO_INCREMENT = 1;
  ALTER TABLE users AUTO_INCREMENT = 1;
"
```

## 快速手动创建 admin 账号

如果数据库中没有 admin 账号，可通过以下方式快速创建：

```bash
# 通过 API 注册
curl -X POST http://127.0.0.1:8080/api/register \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123","confirm_password":"admin123"}'

# 升级为管理员
mysql -u root cpp_oj -e "UPDATE users SET role='admin' WHERE username='admin';"
```
