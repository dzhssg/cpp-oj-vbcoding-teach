const API_BASE = '/api';

async function request(method, path, body) {
  const opts = {
    method,
    headers: { 'Content-Type': 'application/json' },
    credentials: 'same-origin'
  };

  if (body !== undefined) {
    opts.body = JSON.stringify(body);
  }

  const res = await fetch(API_BASE + path, opts);
  const data = await res.json().catch(() => null);

  if (!res.ok) {
    const msg = (data && data.error) ? data.error : ('Request failed (' + res.status + ')');
    const err = new Error(msg);
    err.status = res.status;
    err.data = data;
    throw err;
  }

  return data;
}

const api = {
  register(username, password, confirmPassword) {
    const body = { username, password };
    if (confirmPassword !== undefined) {
      body.confirm_password = confirmPassword;
    }
    return request('POST', '/register', body);
  },

  login(username, password) {
    return request('POST', '/login', { username, password });
  },

  logout() {
    return request('POST', '/logout');
  },

  getProblems() {
    return request('GET', '/problems');
  },

  getProblem(id) {
    return request('GET', '/problems/' + id);
  },

  submit(problemId, code) {
    return request('POST', '/submit', { problem_id: problemId, code });
  },

  createProblem(data) {
    return request('POST', '/admin/problems', data);
  },

  updateProblem(id, data) {
    return request('PUT', '/admin/problems/' + id, data);
  },

  deleteProblem(id) {
    return request('DELETE', '/admin/problems/' + id);
  }
};
