// ============================================================
// leaderboard.js — Score display
// ============================================================

const Leaderboard = {
  async load(sort = 'score', limit = 20) {
    try {
      return await API.get(`/api/admin/leaderboard?sort=${sort}&limit=${limit}`);
    } catch (e) {
      return { enabled: false };
    }
  },

  render(container, data) {
    if (!container) return;
    if (!data || !data.enabled) {
      container.innerHTML = '<p class="text-muted text-center">Leaderboard is disabled</p>';
      return;
    }

    const entries = data.leaderboard || [];
    container.innerHTML = `
      <div class="table-wrapper">
        <table class="leaderboard-table">
          <thead>
            <tr>
              <th>Rank</th>
              <th>Player</th>
              <th>Difficulty</th>
              <th>Exploits</th>
              <th>Time</th>
              <th>Score</th>
              <th>Status</th>
            </tr>
          </thead>
          <tbody>
            ${entries.map(e => `
              <tr>
                <td class="leaderboard-rank rank-${e.rank}">${e.rank <= 3 ? ['','1st','2nd','3rd'][e.rank] : e.rank}</td>
                <td><strong>${e.username}</strong></td>
                <td><span class="badge badge-${e.difficulty==='HARD'?'red':e.difficulty==='NORMAL'?'yellow':'green'}">${e.difficulty}</span></td>
                <td class="text-mono">${e.exploits}</td>
                <td class="text-mono">${e.time_min}m</td>
                <td class="text-mono"><strong>${e.score}</strong></td>
                <td>${e.complete ? '<span class="badge badge-green">Complete</span>' : '<span class="badge badge-blue">Active</span>'}</td>
              </tr>
            `).join('')}
          </tbody>
        </table>
      </div>
      ${data.scoring_formula ? `
        <div class="card mt-md text-sm">
          <div class="card-title">Scoring Formula</div>
          <div class="text-mono mt-md" style="white-space:pre-line">
base = ${data.scoring_formula.base}
+ (exploit_paths x ${data.scoring_formula.per_exploit})
+ max(0, ${data.scoring_formula.time_bonus_max} - minutes x ${data.scoring_formula.time_penalty_rate})
+ (incidents_resolved x ${data.scoring_formula.per_incident})
+ (defense_evasions x ${data.scoring_formula.per_evasion})
- (hints_requested x ${Math.abs(data.scoring_formula.per_hint_penalty)})
          </div>
        </div>
      ` : ''}
    `;
  }
};
