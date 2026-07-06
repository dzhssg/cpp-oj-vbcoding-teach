const AUTH_CHECK_INTERVAL = 30 * 60 * 1000;

const AUTH_KEY = 'oj_user';

const auth = {
  saveUser(user) {
    sessionStorage.setItem(AUTH_KEY, JSON.stringify(user));
  },

  getUser() {
    var raw = sessionStorage.getItem(AUTH_KEY);
    if (!raw) return null;
    try { return JSON.parse(raw); } catch (e) { return null; }
  },

  isLoggedIn() {
    return this.getUser() !== null;
  },

  requireAuth() {
    if (!this.isLoggedIn()) {
      window.location.href = '/login.html';
      return false;
    }
    return true;
  },

  redirectIfLoggedIn(target) {
    if (this.isLoggedIn()) {
      window.location.href = target || '/index.html';
    }
  },

  logout() {
    sessionStorage.removeItem(AUTH_KEY);
  },

  checkAuthPeriodic() {
    setInterval(() => {
      if (!this.isLoggedIn()) {
        var current = window.location.pathname.split('/').pop();
        if (current !== 'login.html' && current !== 'register.html' && current !== '') {
          window.location.href = '/login.html';
        }
      }
    }, AUTH_CHECK_INTERVAL);
  }
};

auth.checkAuthPeriodic();
