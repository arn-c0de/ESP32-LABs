// ============================================================
// flags.js — Flag submission + progress
// ============================================================

const Flags = {
  async loadProgress() {
    try { return await API.get('/api/flags/progress'); }
    catch (e) { return null; }
  },

  async submit(flag) {
    try {
      const result = await API.post('/api/flags/submit', { flag });
      if (result.correct && result.new_discovery) {
        Notify.success('New exploit path discovered: ' + result.path);
      } else if (result.correct) {
        Notify.info('Already discovered: ' + result.path);
      } else {
        Notify.warning(result.message);
      }
      return result;
    } catch (err) {
      Notify.error('Failed to submit flag');
      return null;
    }
  },

  renderProgress(container, data) {
    if (!container || !data) return;

    const paths = data.paths || {};
    const pathInfo = {
      IDOR: { desc: 'Insecure Direct Object Reference', icon: 'ID' },
      INJECTION: { desc: 'Command Injection', icon: 'CI' },
      RACE: { desc: 'Race Condition', icon: 'RC' },
      PHYSICS: { desc: 'Physics-Based Analysis', icon: 'PH' },
      FORENSICS: { desc: 'Log Forensics', icon: 'FR' },
      WEAK_AUTH: { desc: 'Weak Authentication', icon: 'WA' }
    };

    container.innerHTML = `
      <div class="flex-between mb-md">
        <div>
          <span class="metric-value text-green">${data.paths_found}</span>
          <span class="metric-unit">/ ${data.paths_total}</span>
          <span class="text-sm text-muted"> exploit paths found</span>
        </div>
        <span class="badge badge-${data.game_complete ? 'green' : 'blue'}">
          ${data.game_complete ? 'COMPLETE' : 'In Progress'}
        </span>
      </div>
      <div class="progress mb-md">
        <div class="progress-bar green" style="width:${(data.paths_found/data.paths_total*100)}%"></div>
      </div>
      <div class="flag-progress">
        ${Object.entries(pathInfo).map(([key, info]) => `
          <div class="flag-path ${paths[key] ? 'found' : ''}">
            <div class="path-icon text-mono">${info.icon}</div>
            <div>
              <div class="path-name">${key}</div>
              <div class="text-xs text-muted">${info.desc}</div>
            </div>
            <div class="path-status">
              ${paths[key]
                ? '<span class="text-green">Found</span>'
                : '<span class="text-muted">Hidden</span>'}
            </div>
          </div>
        `).join('')}
      </div>
      <div class="card mt-md">
        <div class="grid-3 text-center">
          <div class="metric">
            <div class="metric-value text-blue">${data.score || 0}</div>
            <div class="metric-label">Score</div>
          </div>
          <div class="metric">
            <div class="metric-value">${Math.floor((data.time_elapsed_sec||0)/60)}</div>
            <div class="metric-label">Minutes</div>
          </div>
          <div class="metric">
            <div class="metric-value">${data.hints_requested || 0}</div>
            <div class="metric-label">Hints Used</div>
          </div>
        </div>
      </div>
    `;
  }
};
