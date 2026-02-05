// ============================================================
// api.js — Fetch wrapper + auto-retry + WebSocket
// ============================================================

const API = {
  baseUrl: '',
  session: null,
  token: null,
  ws: null,
  wsCallbacks: {},
  pollInterval: null,

  async request(method, path, body = null, retries = 2) {
    const headers = { 'Content-Type': 'application/json' };
    if (this.token) headers['Authorization'] = 'Bearer ' + this.token;
    if (this.session) headers['X-Session'] = this.session;

    const opts = { method, headers };
    if (body) opts.body = JSON.stringify(body);

    for (let i = 0; i <= retries; i++) {
      try {
        const res = await fetch(this.baseUrl + path, opts);
        const data = await res.json();
        if (!res.ok && data.error) {
          if (res.status === 401) this.onAuthError();
          if (res.status === 429) {
            Notify.warning('Rate limited. Waiting...');
            await new Promise(r => setTimeout(r, 5000));
            continue;
          }
          throw new Error(data.message || 'Request failed');
        }
        return data;
      } catch (err) {
        if (i === retries) throw err;
        await new Promise(r => setTimeout(r, 1000 * (i + 1)));
      }
    }
  },

  get(path)       { return this.request('GET', path); },
  post(path, body) { return this.request('POST', path, body); },

  // WebSocket connection
  connectWS(onMessage) {
    const wsUrl = 'ws://' + location.host + '/ws';
    this.ws = new WebSocket(wsUrl);
    this.ws.onopen = () => console.log('[WS] Connected');
    this.ws.onclose = () => {
      console.log('[WS] Disconnected, reconnecting in 5s...');
      setTimeout(() => this.connectWS(onMessage), 5000);
    };
    this.ws.onerror = () => console.warn('[WS] Error');
    this.ws.onmessage = (e) => {
      try {
        const data = JSON.parse(e.data);
        if (onMessage) onMessage(data);
        if (data.type && this.wsCallbacks[data.type]) {
          this.wsCallbacks[data.type](data);
        }
      } catch (err) { console.warn('[WS] Parse error', err); }
    };
  },

  onWS(type, callback) { this.wsCallbacks[type] = callback; },

  // Polling fallback
  startPolling(callback, intervalMs = 3000) {
    this.pollInterval = setInterval(async () => {
      try {
        const data = await this.get('/api/dashboard/status');
        if (callback) callback(data);
      } catch (err) { /* silent */ }
    }, intervalMs);
  },

  stopPolling() {
    if (this.pollInterval) clearInterval(this.pollInterval);
  },

  // Auth error handler
  onAuthError() {
    this.session = null;
    this.token = null;
    localStorage.removeItem('session');
    localStorage.removeItem('token');
    if (!location.pathname.includes('index.html') && location.pathname !== '/') {
      location.href = '/';
    }
  },

  // Restore session
  restore() {
    this.session = localStorage.getItem('session');
    this.token = localStorage.getItem('token');
  },

  // Login
  async login(username, password) {
    const data = await this.post('/api/auth/login', { username, password });
    if (data.success) {
      this.session = data.session_id;
      this.token = data.token;
      localStorage.setItem('session', data.session_id);
      localStorage.setItem('token', data.token);
      localStorage.setItem('username', data.username);
      localStorage.setItem('role', data.role);
    }
    return data;
  },

  // Logout
  async logout() {
    try { await this.post('/api/auth/logout'); } catch (e) {}
    this.session = null;
    this.token = null;
    localStorage.clear();
    location.href = '/';
  }
};

// Auto-restore on load
API.restore();
