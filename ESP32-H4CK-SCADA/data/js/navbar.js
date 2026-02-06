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
