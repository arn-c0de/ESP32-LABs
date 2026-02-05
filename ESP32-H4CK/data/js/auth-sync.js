// Auth Cookie Sync - Must load BEFORE any auth checks
// Syncs auth cookies to localStorage
(function(){
    function getCookie(n){
        var m=document.cookie.match('(^|;)\\s*'+n+'=([^;]*)');
        return m?decodeURIComponent(m[2]):null;
    }
    
    var ct=getCookie('auth_token');
    var cu=getCookie('auth_user');
    var cr=getCookie('auth_role');
    
    if(ct&&cu){
        localStorage.setItem('token',ct);
        localStorage.setItem('username',cu);
        localStorage.setItem('role',cr||'guest');
    }
})();
