// ============================================================
// defense.js — Defense controls + IDS alerts
// ============================================================

const Defense = {
  async loadStatus() {
    try { return await API.get('/api/defense/status'); }
    catch (e) { return { defense_enabled: false }; }
  },

  async loadAlerts() {
    try { return await API.get('/api/defense/alerts'); }
    catch (e) { return { alerts: [] }; }
  },

  async blockIP(ip, duration) {
    try {
      const result = await API.post('/api/defense/block-ip', { ip, duration });
      Notify.success(result.message);
      return result;
    } catch (err) {
      Notify.error('Failed to block IP');
      return null;
    }
  },

  renderStatus(container, data) {
    if (!container || !data) return;

    if (!data.defense_enabled) {
      container.innerHTML = '<div class="card text-center"><p class="text-muted">Defense system is disabled</p></div>';
      return;
    }

    const r = data.resources || {};
    container.innerHTML = `
      <div class="defense-resources">
        <div class="defense-resource">
          <div class="resource-value text-blue">${r.dp || 0}</div>
          <div class="resource-label">Defense Points</div>
          <div class="progress mt-md"><div class="progress-bar blue" style="width:${(r.dp/r.dp_max*100)||0}%"></div></div>
        </div>
        <div class="defense-resource">
          <div class="resource-value text-green">${r.ap || 0}</div>
          <div class="resource-label">Action Points</div>
          <div class="progress mt-md"><div class="progress-bar green" style="width:${(r.ap/r.ap_max*100)||0}%"></div></div>
        </div>
        <div class="defense-resource">
          <div class="resource-value text-yellow">${r.stability || 0}</div>
          <div class="resource-label">Stability</div>
          <div class="progress mt-md"><div class="progress-bar yellow" style="width:${r.stability||0}%"></div></div>
        </div>
      </div>

      <div class="card mt-md">
        <div class="card-title">Active Modules</div>
        <table class="mt-md">
          <tr><td>IDS</td><td><span class="badge badge-${data.modules.ids ? 'green' : 'gray'}">${data.modules.ids ? 'Active' : 'Off'}</span></td></tr>
          <tr><td>WAF</td><td><span class="badge badge-${data.modules.waf ? 'green' : 'gray'}">${data.modules.waf ? 'Active' : 'Off'}</span></td></tr>
          <tr><td>Rate Limit</td><td><span class="badge badge-${data.modules.rate_limit ? 'green' : 'gray'}">${data.modules.rate_limit ? 'Active' : 'Off'}</span></td></tr>
          <tr><td>IP Blocking</td><td><span class="badge badge-${data.modules.ip_blocking ? 'green' : 'gray'}">${data.modules.ip_blocking ? 'Active' : 'Off'}</span></td></tr>
          <tr><td>Honeypots</td><td><span class="badge badge-${data.modules.honeypot ? 'green' : 'gray'}">${data.modules.honeypot ? 'Active' : 'Off'}</span></td></tr>
        </table>
      </div>

      ${data.blocked_ips && data.blocked_ips.length > 0 ? `
        <div class="card mt-md">
          <div class="card-title">Blocked IPs</div>
          <table class="mt-md">
            <tr><th>IP</th><th>Remaining</th><th>Reason</th></tr>
            ${data.blocked_ips.map(b => `
              <tr><td class="text-mono">${b.ip}</td><td>${b.remaining}s</td><td>${b.reason}</td></tr>
            `).join('')}
          </table>
        </div>
      ` : ''}
    `;
  },

  renderAlerts(container, data) {
    if (!container) return;
    if (!data || !data.alerts || data.alerts.length === 0) {
      container.innerHTML = '<p class="text-muted">No alerts</p>';
      return;
    }

    const alerts = typeof data.alerts === 'string' ? JSON.parse(data.alerts) : data.alerts;
    container.innerHTML = alerts.slice(-30).reverse().map(a => `
      <div class="alarm-item alarm-${a.type === 'WAF_BLOCK' ? 'critical' : 'medium'}">
        <div class="alarm-indicator"></div>
        <div class="alarm-content">
          <div class="alarm-title">${a.type}</div>
          <div class="alarm-details">${a.details}</div>
          <div class="alarm-meta">
            <span>${a.timestamp}</span>
            <span class="text-mono">${a.ip || ''}</span>
          </div>
        </div>
      </div>
    `).join('');
  }
};
