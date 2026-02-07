// SCADA Navbar with User Info
(function(){
    var username = localStorage.getItem('username');
    var role = localStorage.getItem('role');
    var navUser = document.getElementById('nav-user');
    
    if(!navUser) return;
    
    if(username && role){
        var roleColor = role === 'admin' ? '#dc2626' : (role === 'operator' ? '#059669' : '#3b82f6');
        navUser.innerHTML = '<span style="color:rgba(255,255,255,.9);">' + username + 
            '</span> <span style="background:rgba(255,255,255,.2);padding:2px 8px;border-radius:4px;font-size:.8em;color:' + roleColor + ';">' + 
            role.toUpperCase() + '</span>';
    }
})();

            .then(function(r){return r.json()})
            .then(function(d){updateNavbar(d.balance)})
            .catch(function(){updateNavbar(null)});
    } else {
        navUser.innerHTML='<a href="/login" class="btn-nav solid">Sign In</a>';
    }
    
    // Toggle dropdown on button click
    document.addEventListener('click',function(e){
        var toggle=e.target.closest('.user-dropdown-toggle');
        var dropdown=document.querySelector('.user-dropdown');
        if(toggle){
            e.preventDefault();
            if(dropdown)dropdown.classList.toggle('open');
        } else if(dropdown && !e.target.closest('.user-dropdown')){
            dropdown.classList.remove('open');
        }
    });
})();

// Ensure mode.js is available on every page (load if not present)
(function(){
    if(!document.querySelector('script[src="/js/mode.js"]')){
        var s=document.createElement('script');
        s.src='/js/mode.js';
        s.defer=true;
        document.head.appendChild(s);
    }

    // Provide a minimal fallback theme toggle while mode.js initializes
    document.addEventListener('DOMContentLoaded', function(){
        var navMenu=document.querySelector('.nav-menu');
        if(!navMenu) return;
        if(!document.getElementById('theme-toggle')){
            var btn=document.createElement('button');
            btn.id='theme-toggle';
            btn.className='btn-nav';
            btn.title='Toggle theme';

            function updateFallbackBtn(choice, effective){
                var icon = choice === 'dark' ? '🌙' : (choice === 'light' ? '☀️' : '🖥️');
                btn.textContent = icon;
                btn.setAttribute('data-theme-mode', choice);
                btn.title = choice + ' (effective: ' + effective + ')';
            }

            var cur = localStorage.getItem('theme') || (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
            var effective = document.documentElement.getAttribute('data-theme') || (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
            updateFallbackBtn(cur, effective);

            btn.addEventListener('click', function(){
                var curChoice = window.getTheme ? window.getTheme() : localStorage.getItem('theme') || 'system';
                var next = curChoice === 'light' ? 'dark' : (curChoice === 'dark' ? 'system' : 'light');

                if (window.setTheme) {
                    window.setTheme(next);
                } else {
                    // Apply immediately and persist
                    var eff = next === 'system' ? (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light') : next;
                    document.documentElement.setAttribute('data-theme', eff);
                    localStorage.setItem('theme', next);
                    // Emit themechange for any listeners
                    window.dispatchEvent(new CustomEvent('themechange', { detail: { choice: next, effective: eff } }));
                }

                // Update UI
                var effAfter = next === 'system' ? (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light') : next;
                updateFallbackBtn(next, effAfter);
            });

            var logout = navMenu.querySelector('a[onclick^="logout"], a[href="#"]');
            if(logout) navMenu.insertBefore(btn, logout);
            else navMenu.appendChild(btn);

            // Keep the fallback in sync with real changes
            window.addEventListener('themechange', function(e){
                var info = e.detail || {};
                updateFallbackBtn(window.getTheme ? window.getTheme() : localStorage.getItem('theme') || 'system', info.effective || document.documentElement.getAttribute('data-theme'));
            });
        }
    });
})();
