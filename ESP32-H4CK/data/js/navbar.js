// Unified Navbar with User Dropdown
(function(){
    function getCookie(n){
        var m=document.cookie.match('(^|;)\\s*'+n+'=([^;]*)');
        return m?decodeURIComponent(m[2]):null;
    }
    
    // Sync cookies to localStorage
    var ct=getCookie('auth_token'),cu=getCookie('auth_user'),cr=getCookie('auth_role');
    if(ct&&cu){
        localStorage.setItem('token',ct);
        localStorage.setItem('username',cu);
        localStorage.setItem('role',cr||'guest');
    }
    
    var token=localStorage.getItem('token');
    var username=localStorage.getItem('username');
    var role=localStorage.getItem('role');
    var navUser=document.getElementById('navUser');
    
    if(!navUser)return;
    
    function doLogout(){
        fetch('/api/logout').finally(function(){
            localStorage.clear();
            window.location.href='/login';
        });
    }
    
    window.navbarLogout=doLogout;
    
    function updateNavbar(balance){
        var roleClass=role==='admin'?' admin':'';
        var balanceText=balance!==null?' | '+balance.toFixed(0)+' credits':'';
        navUser.innerHTML='<div class="user-dropdown">' +
            '<button class="user-dropdown-toggle">' +
                '<span class="user-name">'+username+'</span>' +
                '<span class="user-balance">'+balanceText+'</span>' +
                '<span class="user-role'+roleClass+'">'+role+'</span>' +
                '<span class="dropdown-arrow">▼</span>' +
            '</button>' +
            '<div class="user-dropdown-menu">' +
                '<div class="dropdown-header">' +
                    '<div class="dropdown-name">'+username+'</div>' +
                    '<div class="dropdown-role">'+role.toUpperCase()+'</div>' +
                '</div>' +
                (balance!==null?'<div class="dropdown-balance">'+balance.toFixed(2)+' Credits</div>':'') +
                '<div class="dropdown-divider"></div>' +
                '<a href="/dashboard" class="dropdown-item">Dashboard</a>' +
                '<a href="/profile" class="dropdown-item">Profil</a>' +
                '<a href="/transactions" class="dropdown-item">Transaktionen</a>' +
                '<a href="/shop" class="dropdown-item">Shop</a>' +
                '<a href="/cart" class="dropdown-item">Warenkorb</a>' +
                '<a href="/orders" class="dropdown-item">Bestellungen</a>' +
                (role==='admin'?'<div class="dropdown-divider"></div><a href="/admin" class="dropdown-item admin-link">Admin Panel</a>':'') +
                '<div class="dropdown-divider"></div>' +
                '<button class="dropdown-item danger" onclick="window.navbarLogout()">Abmelden</button>' +
            '</div>' +
        '</div>';
    }
    
    if(token && username){
        fetch('/api/wallet/balance',{headers:{'Authorization':'Bearer '+token}})
            .then(function(r){return r.json()})
            .then(function(d){updateNavbar(d.balance)})
            .catch(function(){updateNavbar(null)});
    } else {
        navUser.innerHTML='<a href="/login" class="btn-nav solid">Anmelden</a>';
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
