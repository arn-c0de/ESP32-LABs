// Auth Sync for SCADA - Syncs JWT token from cookies to localStorage
(function(){
    function getCookie(name){
        var match = document.cookie.match('(^|;)\\s*' + name + '=([^;]*)');
        return match ? decodeURIComponent(match[2]) : null;
    }
    
    var token = getCookie('jwt_token');
    var username = getCookie('username');
    var role = getCookie('role');
    
    if(token && username){
        localStorage.setItem('jwt_token', token);
        localStorage.setItem('username', username);
        localStorage.setItem('role', role || 'viewer');
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

