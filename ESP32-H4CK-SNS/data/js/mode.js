// Lab Mode Visibility Control
// Handles hiding/showing content based on lab mode (testing/pentest/realism)

(function(){
    var labMode = 'testing'; // default
    
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
        var pentestHide = document.querySelectorAll('.vuln-info, .testing-only, .hint-box, .vuln-hint, .exploit-hint');
        
        // Elements to hide in realism mode (everything pentestHide + more)
        var realismHide = document.querySelectorAll('.vuln-info, .testing-only, .hint-box, .vuln-hint, .exploit-hint, .shell-link, .debug-info, .api-docs');
        
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
})();
