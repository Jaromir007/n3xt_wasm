document.addEventListener('DOMContentLoaded', () => {
    // Search bar
    const searchBar = document.querySelector('#searchBar');
    const searchIcon = document.querySelector('#searchIcon');
    const searchInput = document.querySelector('#searchInput');

    // hamburger toggle
    const hamburgerButton = document.querySelector('#hamburgerButton');
    const mobileMenu = document = document.querySelector('#headerNav');


    searchIcon.addEventListener('click', (e) => {
        searchBar.classList.toggle('active');
        searchInput.focus();
        e.stopPropagation();
    });

    hamburgerButton.addEventListener("click", (e) => {
        hamburgerButton.classList.toggle("active");
        mobileMenu.classList.toggle("active");
    });

    document.addEventListener('click', (e) => {
        if (!searchBar.contains(e.target)) {
            searchBar.classList.remove('active');
            searchInput.blur();
            searchInput.value = '';
        }

        if (!mobileMenu.contains(e.target) && !hamburgerButton.contains(e.target)) {
            mobileMenu.classList.remove('active');
            hamburgerButton.classList.remove('active')
        }
    });
});