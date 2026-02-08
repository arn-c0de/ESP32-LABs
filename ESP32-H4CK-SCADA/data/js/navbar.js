// SCADA Navbar with User Info & Dropdown
(function(){
    var username = localStorage.getItem('username');
    var role = localStorage.getItem('role');
    var navUser = document.getElementById('nav-user');
    var navToggle = document.getElementById('nav-toggle');
    var navMenu = document.getElementById('nav-menu');
    
    if(!navUser) return;
    
    if(username || role){
        // Set user info in dropdown (support missing username or role)
        var displayName = username || (role ? role : 'User');
        var displayRole = role || 'viewer';

        var userName = document.getElementById('user-name');
        var userRole = document.getElementById('user-role');
        var dropdownName = document.getElementById('dropdown-name');
        var dropdownRole = document.getElementById('dropdown-role');

        if(userName) userName.textContent = displayName;
        if(userRole) {
            userRole.textContent = displayRole.toUpperCase();
            if(displayRole === 'admin') {
                userRole.style.background = '#dc2626';
                userRole.style.color = '#fff';
            } else if(displayRole === 'operator') {
                userRole.style.color = '#059669';
            } else if(displayRole === 'viewer' || displayRole === 'maintenance') {
                userRole.style.color = '#3b82f6';
            } else {
                userRole.style.color = '#3b82f6';
            }
        }
        if(dropdownName) dropdownName.textContent = displayName;
        if(dropdownRole) dropdownRole.textContent = displayRole.toUpperCase();

        // Ensure Logout button exists in menu (fallback)
        var menu = navUser.querySelector('.user-dropdown-menu');
        if(menu && !menu.querySelector('.logout-item')){
            var logoutLink = document.createElement('a');
            logoutLink.href = '#';
            logoutLink.className = 'dropdown-item danger logout-item';
            logoutLink.textContent = 'Logout';
            logoutLink.onclick = function(e){ e.preventDefault(); logout(); return false; };
            menu.appendChild(logoutLink);
        }

        // Setup dropdown toggle
        var toggle = document.getElementById('user-dropdown-toggle');
        if(toggle) {
            toggle.addEventListener('click', function(e) {
                e.preventDefault();
                e.stopPropagation();
                navUser.classList.toggle('open');
            });
        }

        // Close dropdown when clicking outside
        document.addEventListener('click', function(e) {
            if(!navUser.contains(e.target)) {
                navUser.classList.remove('open');
            }
        });
    } else {
        // Not logged in - show sign in button
        var btn = document.createElement('button');
        btn.className = 'btn-nav solid';
        btn.innerHTML = '🔓 Sign In';
        btn.style.cursor = 'pointer';
        btn.onclick = function() { location.href = '/login'; };
        navUser.innerHTML = '';
        navUser.appendChild(btn);
    }

    // Mobile nav toggle
    if(navToggle && navMenu) {
        navToggle.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            var isOpen = navMenu.classList.toggle('open');
            navToggle.setAttribute('aria-expanded', isOpen ? 'true' : 'false');
        });

        navMenu.querySelectorAll('a').forEach(function(link) {
            link.addEventListener('click', function() {
                navMenu.classList.remove('open');
                navToggle.setAttribute('aria-expanded', 'false');
            });
        });

        document.addEventListener('click', function(e) {
            if(!navMenu.contains(e.target) && !navToggle.contains(e.target)) {
                navMenu.classList.remove('open');
                navToggle.setAttribute('aria-expanded', 'false');
            }
        });

        window.addEventListener('resize', function() {
            if(window.innerWidth > 900) {
                navMenu.classList.remove('open');
                navToggle.setAttribute('aria-expanded', 'false');
            }
        });
    }
})();

// Theme toggle is handled by mode.js — navbar.js only needs to ensure
// the toggle button exists (mode.js creates it on DOMContentLoaded).

// Ensure mode.js is loaded on every page
(function(){
    if(!document.querySelector('script[src="/js/mode.js"]')){
        var s=document.createElement('script');
        s.src='/js/mode.js';
        s.defer=true;
        document.head.appendChild(s);
    }
})();
