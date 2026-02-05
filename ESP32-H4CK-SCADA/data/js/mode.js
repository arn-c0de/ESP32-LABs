// ============================================================
// mode.js — Dark/Light mode toggle
// ============================================================

const Mode = {
  get current() { return localStorage.getItem('theme') || 'dark'; },

  init() {
    document.documentElement.setAttribute('data-theme', this.current);
    document.querySelectorAll('.mode-toggle').forEach(btn => {
      btn.addEventListener('click', () => this.toggle());
      btn.textContent = this.current === 'dark' ? 'Light' : 'Dark';
    });
  },

  toggle() {
    const next = this.current === 'dark' ? 'light' : 'dark';
    localStorage.setItem('theme', next);
    document.documentElement.setAttribute('data-theme', next);
    document.querySelectorAll('.mode-toggle').forEach(btn => {
      btn.textContent = next === 'dark' ? 'Light' : 'Dark';
    });
  }
};

document.addEventListener('DOMContentLoaded', () => Mode.init());
