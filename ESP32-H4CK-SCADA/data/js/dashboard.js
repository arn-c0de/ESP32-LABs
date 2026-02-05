// ============================================================
// dashboard.js — SCADA Dashboard + Canvas P&ID
// ============================================================

class SCADADashboard {
  constructor() {
    this.data = null;
    this.canvas = null;
    this.ctx = null;
    this.animFrame = null;
  }

  async init() {
    // Connect WebSocket
    API.connectWS((data) => this.onWSMessage(data));

    // WS handlers
    API.onWS('sensors', (d) => this.updateSensors(d.data));
    API.onWS('alarms', (d) => this.updateAlarms(d.data));
    API.onWS('incident', (d) => this.onIncident(d.incident));
    API.onWS('emergency', (d) => Notify.error(d.message));

    // Initial data load
    await this.refresh();

    // Canvas P&ID
    this.canvas = document.getElementById('pid-canvas');
    if (this.canvas) {
      this.ctx = this.canvas.getContext('2d');
      this.resizeCanvas();
      window.addEventListener('resize', () => this.resizeCanvas());
      this.animate();
    }

    // Polling fallback
    API.startPolling((d) => this.onDashboardData(d), 3000);

    // Update clock
    setInterval(() => this.updateClock(), 1000);
  }

  async refresh() {
    try {
      this.data = await API.get('/api/dashboard/status');
      this.render();
    } catch (err) {
      console.warn('Dashboard refresh failed:', err);
    }
  }

  onDashboardData(data) {
    this.data = data;
    this.render();
  }

  onWSMessage(data) {
    // Generic WS message handling
  }

  render() {
    if (!this.data) return;
    this.renderLineCards();
    this.renderStats();
  }

  renderLineCards() {
    const container = document.getElementById('line-cards');
    if (!container || !this.data.lines) return;

    container.innerHTML = this.data.lines.map(line => `
      <div class="process-card" data-status="${line.status}">
        <div class="card-header">
          <div>
            <div class="card-title">${line.name}</div>
            <div class="text-sm text-muted">ID: ${line.id}</div>
          </div>
          <span class="badge badge-${this.statusColor(line.status)}">
            <span class="status-dot ${line.status}"></span>
            ${line.status}
          </span>
        </div>
        <div class="card-metrics">
          <div class="metric">
            <div class="metric-value">${line.metrics.output}<span class="metric-unit">%</span></div>
            <div class="metric-label">Output</div>
          </div>
          <div class="metric">
            <div class="metric-value">${line.metrics.efficiency}<span class="metric-unit">%</span></div>
            <div class="metric-label">Efficiency</div>
          </div>
        </div>
        <div class="card-sensors">
          ${line.sensors.map(s => `
            <div class="sensor-mini" data-status="${s.status}">
              <span>${this.sensorIcon(s.type)}</span>
              <span>${s.value}${s.unit}</span>
            </div>
          `).join('')}
        </div>
      </div>
    `).join('');
  }

  renderStats() {
    const alarmEl = document.getElementById('alarm-count');
    const incidentEl = document.getElementById('incident-count');
    const uptimeEl = document.getElementById('uptime');

    if (alarmEl) alarmEl.textContent = this.data.active_alarms || 0;
    if (incidentEl) incidentEl.textContent = this.data.active_incidents || 0;
    if (uptimeEl) {
      const sec = this.data.uptime_sec || 0;
      const h = Math.floor(sec / 3600);
      const m = Math.floor((sec % 3600) / 60);
      uptimeEl.textContent = `${h}h ${m}m`;
    }
  }

  updateSensors(sensors) {
    if (!sensors) return;
    // Update mini sensor displays
    sensors.forEach(s => {
      const el = document.querySelector(`[data-sensor-id="${s.id}"]`);
      if (el) {
        el.querySelector('.sensor-value').textContent = s.value + s.unit;
        el.setAttribute('data-status', s.status);
      }
    });
  }

  updateAlarms(alarms) {
    const el = document.getElementById('alarm-count');
    if (el && alarms) el.textContent = alarms.length;

    // Flash alarm notification
    if (alarms && alarms.length > 0) {
      const latest = alarms[alarms.length - 1];
      if (latest.severity === 'CRITICAL') {
        Notify.error(`ALARM: ${latest.message} (Line ${latest.line})`);
      }
    }
  }

  onIncident(incident) {
    Notify.warning(`Incident: ${incident.type} on Line ${incident.line}`);
    this.refresh();
  }

  // Canvas P&ID rendering
  resizeCanvas() {
    if (!this.canvas) return;
    const parent = this.canvas.parentElement;
    this.canvas.width = parent.clientWidth;
    this.canvas.height = 400;
  }

  animate() {
    if (!this.ctx) return;
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    this.drawPID();
    this.animFrame = requestAnimationFrame(() => this.animate());
  }

  drawPID() {
    const w = this.canvas.width;
    const h = this.canvas.height;
    const ctx = this.ctx;

    // Background
    ctx.fillStyle = getComputedStyle(document.documentElement).getPropertyValue('--bg-surface').trim() || '#1e242e';
    ctx.fillRect(0, 0, w, h);

    if (!this.data || !this.data.lines) return;

    const lineCount = this.data.lines.length;
    const lineHeight = h / lineCount;

    this.data.lines.forEach((line, i) => {
      const y = i * lineHeight + lineHeight / 2;
      const xStart = 60;
      const xEnd = w - 60;

      // Line label
      ctx.fillStyle = '#8b95a5';
      ctx.font = '12px monospace';
      ctx.fillText(line.id, 10, y + 4);

      // Main pipe
      ctx.strokeStyle = line.status === 'running' ? '#45aaf2' : '#636e72';
      ctx.lineWidth = 4;
      ctx.beginPath();
      ctx.moveTo(xStart, y);
      ctx.lineTo(xEnd, y);
      ctx.stroke();

      // Flow arrows
      if (line.status === 'running') {
        const arrowCount = 5;
        const offset = (Date.now() / 20) % ((xEnd - xStart) / arrowCount);
        for (let a = 0; a < arrowCount; a++) {
          const ax = xStart + a * ((xEnd - xStart) / arrowCount) + offset;
          if (ax < xEnd - 20) {
            ctx.fillStyle = '#45aaf2';
            ctx.beginPath();
            ctx.moveTo(ax, y - 4);
            ctx.lineTo(ax + 8, y);
            ctx.lineTo(ax, y + 4);
            ctx.fill();
          }
        }
      }

      // Equipment positions
      const positions = [0.15, 0.35, 0.55, 0.75];

      // Motor (circle)
      const motorX = xStart + (xEnd - xStart) * positions[0];
      this.drawMotor(ctx, motorX, y, line.status === 'running');

      // Valve (diamond)
      const valveX = xStart + (xEnd - xStart) * positions[1];
      this.drawValve(ctx, valveX, y, line.status !== 'stopped' ? 'open' : 'closed');

      // Sensor (square)
      const sensorX = xStart + (xEnd - xStart) * positions[2];
      const sStatus = line.sensors && line.sensors[0] ? line.sensors[0].status : 'normal';
      this.drawSensor(ctx, sensorX, y, sStatus,
        line.sensors && line.sensors[0] ? line.sensors[0].value + line.sensors[0].unit : '');

      // Pump (triangle)
      const pumpX = xStart + (xEnd - xStart) * positions[3];
      this.drawPump(ctx, pumpX, y, line.status === 'running');
    });
  }

  drawMotor(ctx, x, y, running) {
    const r = 16;
    ctx.beginPath();
    ctx.arc(x, y, r, 0, Math.PI * 2);
    ctx.fillStyle = running ? '#26de81' : '#636e72';
    ctx.fill();
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 1.5;
    ctx.stroke();

    // Rotation line
    if (running) {
      const angle = (Date.now() / 100) % (Math.PI * 2);
      ctx.save();
      ctx.translate(x, y);
      ctx.rotate(angle);
      ctx.beginPath();
      ctx.moveTo(0, -r + 4);
      ctx.lineTo(0, r - 4);
      ctx.strokeStyle = '#fff';
      ctx.lineWidth = 2;
      ctx.stroke();
      ctx.restore();
    }

    ctx.fillStyle = '#fff';
    ctx.font = '9px monospace';
    ctx.textAlign = 'center';
    ctx.fillText('M', x, y + 3);
    ctx.textAlign = 'left';
  }

  drawValve(ctx, x, y, state) {
    const s = 14;
    ctx.beginPath();
    ctx.moveTo(x - s, y);
    ctx.lineTo(x, y - s);
    ctx.lineTo(x + s, y);
    ctx.lineTo(x, y + s);
    ctx.closePath();
    ctx.fillStyle = state === 'open' ? '#26de81' : state === 'stuck' ? '#ff4757' : '#636e72';
    ctx.fill();
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 1.5;
    ctx.stroke();
  }

  drawSensor(ctx, x, y, status, label) {
    const s = 10;
    ctx.fillStyle = status === 'critical' ? '#ff4757' : status === 'high' ? '#fed330' :
                    status === 'fault' ? '#a55eea' : '#26de81';
    ctx.fillRect(x - s, y - s, s * 2, s * 2);
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 1;
    ctx.strokeRect(x - s, y - s, s * 2, s * 2);

    if (label) {
      ctx.fillStyle = '#e6e8eb';
      ctx.font = '9px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(label, x, y - s - 4);
      ctx.textAlign = 'left';
    }
  }

  drawPump(ctx, x, y, running) {
    const s = 14;
    ctx.beginPath();
    ctx.moveTo(x, y - s);
    ctx.lineTo(x + s, y + s);
    ctx.lineTo(x - s, y + s);
    ctx.closePath();
    ctx.fillStyle = running ? '#45aaf2' : '#636e72';
    ctx.fill();
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 1.5;
    ctx.stroke();
  }

  statusColor(status) {
    switch (status) {
      case 'running': return 'green';
      case 'stopped': return 'gray';
      case 'warning': return 'yellow';
      case 'alarm':   return 'red';
      default:        return 'gray';
    }
  }

  sensorIcon(type) {
    const icons = { temperature: 'T', pressure: 'P', vibration: 'V', flow: 'F', current: 'A' };
    return icons[type] || '?';
  }

  updateClock() {
    const el = document.getElementById('clock');
    if (el) el.textContent = new Date().toLocaleTimeString();
  }
}

// Init on page load
document.addEventListener('DOMContentLoaded', () => {
  if (document.getElementById('line-cards') || document.getElementById('pid-canvas')) {
    window.dashboard = new SCADADashboard();
    window.dashboard.init();
  }
});
