// ============================================================
// incidents.js — Incident UI + RCA interface
// ============================================================

const Incidents = {
  async loadActive() {
    try { return await API.get('/api/incidents/list'); }
    catch (e) { return { incidents: [], count: 0 }; }
  },

  async submitRCA(incidentId, diagnosis) {
    try {
      const result = await API.post('/api/incidents/report', {
        incident_id: incidentId,
        diagnosis: diagnosis
      });
      if (result.success) {
        Notify.success('Incident resolved! ' + (result.sub_flag || ''));
      }
      return result;
    } catch (err) {
      Notify.error('Failed to submit RCA');
      return null;
    }
  },

  renderList(container, data) {
    if (!container) return;
    if (!data || !data.incidents || data.incidents.length === 0) {
      container.innerHTML = '<p class="text-muted text-center">No active incidents</p>';
      return;
    }

    container.innerHTML = data.incidents.map(inc => `
      <div class="card" data-incident="${inc.id}">
        <div class="flex-between">
          <div>
            <span class="text-mono text-sm">${inc.id}</span>
            <span class="badge badge-${this.sevColor(inc.severity)} ml-sm">${inc.severity}</span>
          </div>
          <span class="text-muted text-xs">${inc.age_sec}s ago</span>
        </div>
        <div class="mt-md">
          <div><strong>${inc.type}</strong> — Line ${inc.line}</div>
          <div class="text-sm text-secondary">${inc.description}</div>
          <div class="text-xs text-muted mt-md">Equipment: ${inc.equipment}</div>
        </div>
        <div class="mt-md">
          <button class="btn btn-sm btn-primary" onclick="Incidents.showRCA('${inc.id}')">
            Submit RCA
          </button>
        </div>
      </div>
    `).join('');
  },

  showRCA(incidentId) {
    const overlay = document.getElementById('modal-overlay');
    const modal = document.getElementById('rca-modal');
    if (!overlay || !modal) return;

    modal.querySelector('.modal-title').textContent = 'Root Cause Analysis: ' + incidentId;
    modal.querySelector('#rca-incident-id').value = incidentId;
    modal.querySelector('#rca-text').value = '';

    overlay.classList.add('active');
    modal.classList.add('active');
  },

  async handleRCASubmit() {
    const incId = document.getElementById('rca-incident-id').value;
    const text = document.getElementById('rca-text').value;
    if (!text.trim()) { Notify.warning('Please enter a diagnosis'); return; }

    const result = await this.submitRCA(incId, text);
    if (result && result.success) {
      document.getElementById('modal-overlay').classList.remove('active');
      document.getElementById('rca-modal').classList.remove('active');
      // Reload incidents
      const data = await this.loadActive();
      this.renderList(document.getElementById('incident-list'), data);
    }
  },

  sevColor(sev) {
    return { LOW: 'blue', MEDIUM: 'yellow', HIGH: 'orange', CRITICAL: 'red' }[sev] || 'gray';
  }
};
