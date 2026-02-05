// ============================================================
// hints.js — Progressive hint disclosure UI
// ============================================================

const Hints = {
  async load(endpoint, level) {
    try {
      const data = await API.get(`/api/hints?endpoint=${encodeURIComponent(endpoint)}&level=${level}`);
      return data;
    } catch (err) {
      Notify.error('Failed to load hints');
      return null;
    }
  },

  async loadAll() {
    try {
      const data = await API.get('/api/hints?endpoint=&level=3');
      return data;
    } catch (err) {
      return null;
    }
  },

  renderHints(container, hints) {
    if (!container || !hints) return;
    container.innerHTML = hints.hints ? hints.hints.map(h => `
      <div class="card" style="margin-bottom:8px;opacity:${h.unlocked ? 1 : 0.5}">
        <div class="flex-between">
          <span class="badge badge-${h.unlocked ? 'blue' : 'gray'}">
            Level ${h.level} — ${h.path}
          </span>
          ${h.unlocked ? '' : '<span class="text-xs text-muted">Locked</span>'}
        </div>
        <div class="mt-md text-sm">${h.text}</div>
      </div>
    `).join('') : '<p class="text-muted">No hints available</p>';
  }
};
