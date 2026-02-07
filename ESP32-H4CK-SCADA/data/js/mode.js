// Lab Mode Visibility Control
// Handles hiding/showing content based on lab mode (testing/pentest/realism)

(function(){
    var labMode = 'testing'; // default
    var originalSecurityHtml = null; // will store original login warning content
    
    // Fetch lab mode from API
    fetch('/api/info')
        .then(r => r.json())
        .then(data => {
            labMode = data.lab_mode || 'testing';
            applyModeVisibility();
        })
        .catch(() => {
            // Default to testing mode if API fails
            applyModeVisibility();
        });
    
    function applyModeVisibility() {
        var role = localStorage.getItem('role');
        
        // Always hide admin-only elements for non-admin users
        if (role !== 'admin') {
            var adminOnlyElements = document.querySelectorAll('.admin-only');
            adminOnlyElements.forEach(el => {
                if (el.tagName === 'LI') {
                    el.style.display = 'none';
                } else if (el.tagName === 'A') {
                    el.style.display = 'none';
                }
            });
        }
        
        // Elements to hide in pentest mode
        var pentestHide = document.querySelectorAll('.vuln-info, .testing-only, .hint-box, .vuln-hint, .exploit-hint, .default-creds');
        
        // Elements to hide in realism mode (everything pentestHide + more)
        var realismHide = document.querySelectorAll('.vuln-info, .testing-only, .hint-box, .vuln-hint, .exploit-hint, .default-creds, .shell-link, .debug-info, .api-docs');
        
        if (labMode === 'pentest') {
            // Hide vuln hints and testing info
            pentestHide.forEach(el => el.style.display = 'none');
            
            // Hide shell link in navbar
            var shellLinks = document.querySelectorAll('.shell-link');
            shellLinks.forEach(link => {
                if (link.tagName === 'LI') {
                    link.style.display = 'none';
                } else if (link.tagName === 'A') {
                    link.style.display = 'none';
                }
            });
        } else if (labMode === 'realism') {
            // Hide everything
            realismHide.forEach(el => el.style.display = 'none');
            
            // Hide shell link
            var shellLinks = document.querySelectorAll('.shell-link');
            shellLinks.forEach(link => {
                if (link.tagName === 'LI') {
                    link.style.display = 'none';
                } else if (link.tagName === 'A') {
                    link.style.display = 'none';
                }
            });
        }

        // Update the login security warning depending on mode
        var sec = document.querySelector('.security-warning');
        if (sec) {
            // Keep original HTML so we can restore in testing mode
            if (originalSecurityHtml === null) originalSecurityHtml = sec.innerHTML;

            if (labMode === 'pentest') {
                sec.innerHTML = '<p><strong>⚠️ TRAINING LAB (Pentest mode)</strong></p>' +
                                '<p>Default test accounts and hints are hidden for this mode.</p>';
                sec.style.display = 'block';
            } else if (labMode === 'realism') {
                // In realism mode minimize exposure and hide the notice entirely
                sec.style.display = 'none';
            } else {
                // testing: restore original message
                sec.innerHTML = originalSecurityHtml;
                sec.style.display = '';
            }
        }

        // testing mode: show everything (default) but still respect admin-only
    }
    
    // Export function for dynamic use
    window.getLabMode = function() {
        return labMode;
    };
    
    window.isTestingMode = function() {
        return labMode === 'testing';
    };
    
    window.isPentestMode = function() {
        return labMode === 'pentest';
    };
    
    window.isRealismMode = function() {
        return labMode === 'realism';
    };

    /* ===== Theme / Dark Mode Support ===== */
    var savedTheme = localStorage.getItem('theme');

    function detectSystemTheme() {
        return window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    }

    function applyTheme(t) {
        var choice = t || (localStorage.getItem('theme') || 'system');
        var effective = (choice === 'system') ? detectSystemTheme() : choice;

        // Set the effective theme on the document (used by CSS variables)
        document.documentElement.setAttribute('data-theme', effective);

        // Update toggle UI if present
        var btn = document.getElementById('theme-toggle');
        if (btn) {
            var icon = choice === 'dark' ? '🌙' : (choice === 'light' ? '☀️' : '🖥️');
            var label = choice === 'dark' ? 'Dark' : (choice === 'light' ? 'Light' : 'System');
            btn.textContent = icon;
            btn.title = label + ' (effective: ' + effective + ')';
            btn.setAttribute('data-theme-mode', choice);
            btn.setAttribute('aria-label', 'Theme: ' + label + ' — effective: ' + effective);
            btn.setAttribute('aria-pressed', effective === 'dark');
        }

        // Broadcast theme change for other listeners
        window.dispatchEvent(new CustomEvent('themechange', { detail: { choice: choice, effective: effective } }));

        // Log for debugging and verification
        if (window.console) console.log('[THEME] applied', { choice: choice, effective: effective });
    }

    window.setTheme = function(t) {
        if (!t) return;
        localStorage.setItem('theme', t);
        applyTheme(t);
    };

    window.getTheme = function() {
        return localStorage.getItem('theme') || 'system';
    };

    // Initialize theme on load
    (function initTheme(){
        var t = savedTheme || 'system';
        applyTheme(t);

        // Listen to system changes when in 'system' mode
        if (window.matchMedia) {
            window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function() {
                if (window.getTheme() === 'system') applyTheme('system');
            });
        }

        // Ensure applyModeVisibility runs again once DOM is ready (so elements like .security-warning exist)
        document.addEventListener('DOMContentLoaded', function(){
            applyModeVisibility();

            // Small verification helper for debugging: logs current stored choice and effective theme
            window.verifyTheme = function(){
                var stored = window.getTheme();
                var eff = document.documentElement.getAttribute('data-theme');
                console.log('[THEME] verify - stored:', stored, 'effective:', eff);
                return { stored: stored, effective: eff };
            };
            // Run verification automatically on page load
            window.verifyTheme();

            // Inject a theme toggle into navbar if present (cycles Light -> Dark -> System)
            var navMenu = document.querySelector('.nav-menu');
            if (!navMenu) return;
            if (!document.getElementById('theme-toggle')) {
                var btn = document.createElement('button');
            btn.id = 'theme-toggle';
            btn.className = 'btn-nav';
            btn.style.marginLeft = '8px';

            function updateBtnForMode(choice, effective) {
                var icon = choice === 'dark' ? '🌙' : (choice === 'light' ? '☀️' : '🖥️');
                var label = choice === 'dark' ? 'Dark' : (choice === 'light' ? 'Light' : 'System');
                btn.textContent = icon;
                btn.title = label + ' (effective: ' + effective + ')';
                btn.setAttribute('data-theme-mode', choice);
                btn.setAttribute('aria-label', 'Theme: ' + label + ' — effective: ' + effective);
                btn.setAttribute('aria-pressed', effective === 'dark');
            }

            // Reflect current stored choice (or system) and effective theme
            var chosen = window.getTheme();
            var effective = document.documentElement.getAttribute('data-theme') || detectSystemTheme();
            updateBtnForMode(chosen, effective);

            btn.addEventListener('click', function(){
                var cur = window.getTheme();
                var next = cur === 'light' ? 'dark' : (cur === 'dark' ? 'system' : 'light');
                window.setTheme(next);
                var eff = (next === 'system') ? (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light') : next;
                updateBtnForMode(next, eff);
            });

            // Try to insert before the Logout link, otherwise append
            var logoutLink = navMenu.querySelector('a[onclick^="logout"], a[href="#"]');
            if (logoutLink) navMenu.insertBefore(btn, logoutLink);
            else navMenu.appendChild(btn);

            // Listen to external theme changes (dispatched by applyTheme)
            window.addEventListener('themechange', function(e){
                var info = e.detail || {};
                updateBtnForMode(window.getTheme(), info.effective || document.documentElement.getAttribute('data-theme'));
            });
        }
    });
    })();
})();
