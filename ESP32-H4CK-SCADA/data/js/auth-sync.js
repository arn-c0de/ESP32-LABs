// ============================================================
// auth-sync.js — Session management + auth state
// ============================================================

const Auth = {
  get isLoggedIn() { return !!localStorage.getItem('session'); },
  get username()   { return localStorage.getItem('username') || ''; },
  get role()       { return localStorage.getItem('role') || 'none'; },
  get session()    { return localStorage.getItem('session') || ''; },
  get isAdmin()    { return this.role === 'admin'; },

  async check() {
    try {
      const data = await API.get('/api/auth/status');
      if (!data.authenticated) {
        this.clear();
        return false;
      }
      return true;
    } catch (e) {
      return false;
    }
  },

  clear() {
    localStorage.removeItem('session');
    localStorage.removeItem('token');
    localStorage.removeItem('username');
    localStorage.removeItem('role');
  },

  requireAuth() {
    if (!this.isLoggedIn) {
      location.href = '/';
      return false;
    }
    return true;
  },

  updateUI() {
    // Update username display
    document.querySelectorAll('.user-name').forEach(el => {
      el.textContent = this.username || 'Guest';
    });
    document.querySelectorAll('.user-role').forEach(el => {
      el.textContent = this.role;
    });

    // Show/hide admin elements
    document.querySelectorAll('.admin-only').forEach(el => {
      el.style.display = this.isAdmin ? '' : 'none';
    });

    // Show/hide auth-dependent elements
    document.querySelectorAll('.auth-only').forEach(el => {
      el.style.display = this.isLoggedIn ? '' : 'none';
    });
    document.querySelectorAll('.guest-only').forEach(el => {
      el.style.display = this.isLoggedIn ? 'none' : '';
    });
  }
};
