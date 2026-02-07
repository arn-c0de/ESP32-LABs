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

        // === Vulnerability pages visibility ===
        // Non-admin users should only see vulnerability sections when labMode === 'testing'
        var vulnElements = document.querySelectorAll('.vuln-section, .vuln-card, .vuln-category, .tools-section, .tools-grid, .tools-card, .tools-item');
        if (labMode !== 'testing' && role !== 'admin') {
            vulnElements.forEach(el => el.style.display = 'none');
        } else {
            // show for testing mode or admin users
            vulnElements.forEach(el => el.style.display = '');
        }

        // === Incidents visibility ===
        // Incidents pages and widgets are visible to admin and operator only
        var incidentsEls = document.querySelectorAll('.incidents-section, .incidents-list, .incident-placeholder, .incidents-placeholder');
        var incLinks = document.querySelectorAll('a[href="/incidents"], .nav-menu a[href="/incidents"]');
        if (role !== 'admin' && role !== 'operator') {
            incidentsEls.forEach(el => el.style.display = 'none');
            incLinks.forEach(a => a.style.display = 'none');
        } else {
            incidentsEls.forEach(el => el.style.display = '');
            incLinks.forEach(a => a.style.display = '');
        }

        // === Viewer restrictions ===
        // Viewers should only see the Dashboard. Hide nav links and sections and redirect if needed.
        if (role === 'viewer') {
            // Hide most nav links except Dashboard and Logout (keep user menu & dropdown)
            document.querySelectorAll('.nav-menu > a').forEach(a => {
                var href = a.getAttribute('href') || '';
                if (href !== '/dashboard' && href !== '/login' && href !== '/') {
                    a.style.display = 'none';
                }
            });

            // Hide content sections that viewers should not access
            var restrictedSelectors = [
                '.sensors-grid', '.actuators-grid', '.alarms-table-container', '.vuln-container',
                '.defense-resources', '.defense-actions', '.incidents-section', '.rules-table', '.active-rules'
            ];
            restrictedSelectors.forEach(sel => {
                document.querySelectorAll(sel).forEach(el => el.style.display = 'none');
            });

            // If the viewer is on any page other than dashboard or index, redirect to dashboard
            var p = window.location.pathname || '/';
            if (!(p.startsWith('/dashboard') || p === '/' || p.startsWith('/index') || p.startsWith('/login'))) {
                console.log('[MODE] Viewer redirect to /dashboard from', p);
                window.location.replace('/dashboard');
            }
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
    function detectSystemTheme() {
        return window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
    }

    function applyTheme(choice) {
        choice = choice || localStorage.getItem('theme') || 'light';
        var effective = (choice === 'system') ? detectSystemTheme() : choice;
        if (effective === 'dark') {
            document.documentElement.setAttribute('data-theme', 'dark');
        } else {
            document.documentElement.removeAttribute('data-theme');
        }
        localStorage.setItem('theme', choice);

        var btn = document.getElementById('theme-toggle');
        if (btn) {
            var icon = choice === 'dark' ? '🌙' : (choice === 'light' ? '☀️' : '🖥️');
            btn.textContent = icon;
            btn.title = choice.charAt(0).toUpperCase() + choice.slice(1) + ' mode (effective: ' + effective + ')';
            btn.setAttribute('data-theme-mode', choice);
        }
        console.log('[THEME] Applied:', { choice: choice, effective: effective });
    }

    window.setTheme = function(t) {
        localStorage.setItem('theme', t);
        applyTheme(t);
    };

    window.getTheme = function() {
        return localStorage.getItem('theme') || 'light';
    };

    // Apply theme immediately before DOM rendering
    applyTheme();

    // Listen to system theme changes
    if (window.matchMedia) {
        window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function() {
            if (window.getTheme() === 'system') applyTheme('system');
        });
    }

    // Create theme toggle button in navbar (runs after DOM is ready)
    document.addEventListener('DOMContentLoaded', function(){
        applyModeVisibility();

        var navMenu = document.querySelector('.nav-menu');
        if (!navMenu || document.getElementById('theme-toggle')) return;

        var btn = document.createElement('button');
        btn.id = 'theme-toggle';
        btn.className = 'btn-nav';
        btn.style.marginLeft = '8px';
        btn.setAttribute('type', 'button');
        btn.setAttribute('aria-label', 'Toggle theme');

        var choice = window.getTheme();
        var icon = choice === 'dark' ? '🌙' : (choice === 'light' ? '☀️' : '🖥️');
        btn.textContent = icon;
        btn.title = choice.charAt(0).toUpperCase() + choice.slice(1) + ' mode';

        btn.addEventListener('click', function(e){
            e.preventDefault();
            var cur = window.getTheme();
            var next = cur === 'light' ? 'dark' : (cur === 'dark' ? 'system' : 'light');
            window.setTheme(next);
            applyTheme(next);
        });

        // Insert before the user dropdown (direct child of nav-menu)
        var userDropdown = navMenu.querySelector('#nav-user');
        if (userDropdown) {
            navMenu.insertBefore(btn, userDropdown);
        } else {
            navMenu.appendChild(btn);
        }
    });
})();
