// Auth Sync for SCADA - Syncs JWT token from cookies to localStorage
(function(){
    function getCookie(name){
        var match = document.cookie.match('(^|;)\\s*' + name + '=([^;]*)');
        return match ? decodeURIComponent(match[2]) : null;
    }
    
    // Support multiple cookie names set by server: auth_token (form submit) or jwt_token (older client)
    var token = getCookie('auth_token') || getCookie('jwt_token') || getCookie('auth_token');
    var username = getCookie('auth_user') || getCookie('username');
    var role = getCookie('auth_role') || getCookie('role');
    
    if(token && username){
        // Only overwrite localStorage if token exists
        localStorage.setItem('jwt_token', token);
        localStorage.setItem('username', username);
        localStorage.setItem('role', role || 'viewer');
        console.log('[AUTH-SYNC] Synced auth token from cookies (user:', username, 'role:', role, ')');
    }
})();

// Global auth helpers
function requireAuth() {
    const token = localStorage.getItem('jwt_token');
    if (!token) {
        window.location.href = '/';
    }
}

function logout() {
    fetch('/api/logout', {
        method: 'GET',
        headers: { 'Authorization': 'Bearer ' + localStorage.getItem('jwt_token') }
    }).finally(function() {
        localStorage.clear();
        window.location.href = '/';
    });
}

