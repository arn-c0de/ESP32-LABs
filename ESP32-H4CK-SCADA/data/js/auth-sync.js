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

// Get auth token with fallback from localStorage -> cookies
// Critical for mobile browsers where localStorage may fail
function getAuthToken() {
    // Try localStorage first
    let token = localStorage.getItem('jwt_token');
    if (token) return token;
    
    // Fallback: read from cookie (for mobile browsers)
    const cookies = document.cookie.split(';');
    for (let cookie of cookies) {
        const [name, value] = cookie.trim().split('=');
        if (name === 'auth_token') {
            token = value;
            // Cache it in localStorage for next time
            try { 
                localStorage.setItem('jwt_token', token);
                console.log('[AUTH-SYNC] Retrieved token from cookie fallback');
            } catch(e) {
                console.warn('[AUTH-SYNC] localStorage unavailable:', e);
            }
            return token;
        }
    }
    
    console.error('[AUTH-SYNC] No authentication token found in localStorage or cookies');
    return null;
}

function requireAuth() {
    const token = getAuthToken();
    if (!token) {
        console.warn('[AUTH-SYNC] No auth token found, redirecting to login');
        window.location.href = '/';
    }
}

function logout() {
    const token = getAuthToken();
    fetch('/api/logout', {
        method: 'GET',
        headers: { 'Authorization': 'Bearer ' + (token || '') }
    }).finally(function() {
        localStorage.clear();
        // Also clear auth cookies
        document.cookie = 'auth_token=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;';
        document.cookie = 'session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:01 GMT;';
        window.location.href = '/';
    });
}

