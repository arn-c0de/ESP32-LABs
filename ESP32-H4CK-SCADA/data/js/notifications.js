// ============================================================
// notifications.js — Toast notifications
// ============================================================

const Notify = {
  container: null,

  init() {
    if (this.container) return;
    this.container = document.createElement('div');
    this.container.className = 'toast-container';
    document.body.appendChild(this.container);
  },

  show(message, type = 'info', duration = 4000) {
    this.init();
    const toast = document.createElement('div');
    toast.className = 'toast toast-' + type;
    toast.innerHTML = `<div style="font-weight:600;margin-bottom:2px">${type.toUpperCase()}</div>
                       <div style="font-size:0.85rem">${message}</div>`;
    this.container.appendChild(toast);
    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateX(100%)';
      toast.style.transition = 'all 0.3s';
      setTimeout(() => toast.remove(), 300);
    }, duration);
  },

  success(msg) { this.show(msg, 'success'); },
  error(msg)   { this.show(msg, 'error', 6000); },
  warning(msg) { this.show(msg, 'warning', 5000); },
  info(msg)    { this.show(msg, 'info'); }
};
