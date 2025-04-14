let allResults = [];
let currentPage = 0;
const pageSize = 10;

// Set up an array of GIFs for the loading animation
const gifs = [
  'https://tenor.com/view/evan-peters-ip-ip-address-google-computer-gif-16211954443149729959.gif', // IP
  'https://media4.giphy.com/media/v1.Y2lkPTc5MGI3NjExMzZ4ZnpjdDVoOHlueW1kbmRtdG55NDIzcjA1aHdzaXlnam9tanp4MSZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/FArgGzk7KO14k/giphy.gif', // Bears
  'https://tenor.com/view/kerfuffle-fox-gif-18270783.gif', // fox
  'https://media0.giphy.com/media/v1.Y2lkPTc5MGI3NjExeWcwM3U1dGl1aGIyamgzbWNqcDE1MXRkenM0OTBudHRuNnF5NGl0NiZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/E6jscXfv3AkWQ/giphy.gif', // Cat
  'https://media3.giphy.com/media/v1.Y2lkPTc5MGI3NjExbjBuMHI2b3dpZWpzbzF1eGF0enhvcWwzM2duZHh2b3FnYzZhbXNhNiZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/42wQXwITfQbDGKqUP7/giphy.gif', // Pikachu
  'https://media2.giphy.com/media/v1.Y2lkPTc5MGI3NjExbTY0MmUxMHlpN3dycmt0cWl4aWJ2NTJ6ZG50c3B5Zzk3eWNyZHdydyZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/A53vF9xNk7AKnQPLDs/giphy.gif' // Cookie monster
];

// DOM References
const searchForm = document.getElementById('searchForm');
const queryInput = document.getElementById('queryInput');
const searchResultsContainer = document.getElementById('searchResults');
const loadingIndicator = document.getElementById('loading');
const loadingGif = document.getElementById('loadingGif');
const paginationControls = document.getElementById('paginationControls');
const prevButton = document.getElementById('prevPage');
const nextButton = document.getElementById('nextPage');
const pageNumberSpan = document.getElementById('pageNumber');
const submitButton = searchForm.querySelector('button[type="submit"]');

let gifInterval = null; // To store the interval ID

searchForm.addEventListener('submit', function (e) {
  e.preventDefault(); // Prevent default form submission

  const query = queryInput.value.trim();
  if (!query) {
    queryInput.focus();
    return;
  }

  // Reset state for new search
  currentPage = 0;
  allResults = [];
  searchResultsContainer.innerHTML = ''; // Clear previous results immediately
  paginationControls.style.display = 'none'; // Hide pagination

  // --- UI Updates for Loading ---
  submitButton.classList.add('loading');
  submitButton.disabled = true;
  loadingIndicator.style.display = 'block'; // Show loading section

  // Start cycling GIFs
  let gifIndex = Math.floor(Math.random() * gifs.length); // Start with a random GIF
  loadingGif.src = gifs[gifIndex]; // Set initial GIF immediately

  gifInterval = setInterval(() => {
    gifIndex = (gifIndex + 1) % gifs.length;
    loadingGif.src = gifs[gifIndex];
  }, 2000); // Change GIF every 2 seconds

  // --- Fetch Data ---
  fetch('/api/search/', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json; charset=utf-8'
    },
    body: JSON.stringify({ query: query }) // Ensure key matches backend expectation
  })
    .then(response => {
      if (!response.ok) {
        // Try to get error message from response body if possible
        return response.text().then(text => {
           throw new Error(`HTTP error! Status: ${response.status}, Message: ${text || response.statusText}`);
        });
      }
      return response.json();
    })
    .then(data => {
      if (!data.results || !Array.isArray(data.results)) {
         throw new Error("Invalid data format received from server.");
      }

      if (data.results.length === 0) {
        searchResultsContainer.innerHTML = '<p style="text-align: center; color: #a58d8d; margin-top: 30px;">No results found for your query.</p>';
      } else {
        allResults = data.results;
        renderPage(0); // Render the first page
        // Show pagination only if there are results
        paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
      }
    })
    .catch(error => {
      console.error('Search Error:', error);
      searchResultsContainer.innerHTML = `<p style="text-align: center; color: #ec2225; margin-top: 30px;">An error occurred: ${error.message}. Please try again.</p>`;
    })
    .finally(() => {
      // --- Cleanup UI After Loading ---
      clearInterval(gifInterval); // Stop changing GIFs
      gifInterval = null;
      loadingIndicator.style.display = 'none'; // Hide loading section
      submitButton.classList.remove('loading');
      submitButton.disabled = false;
    });
});

function renderPage(page) {
  // Ensure page is within valid bounds
  if (page < 0 || page * pageSize >= allResults.length) {
    console.warn("Attempted to render invalid page:", page);
    return;
  }

  searchResultsContainer.innerHTML = ''; // Clear previous page's results

  const start = page * pageSize;
  const end = Math.min(start + pageSize, allResults.length); // Ensure end doesn't exceed array length
  const pageResults = allResults.slice(start, end);

  const fragment = document.createDocumentFragment(); // Use fragment for performance

  pageResults.forEach(doc => {
    const div = document.createElement('div');
    div.className = 'result'; // Use the class defined in CSS

    // Sanitize content if it comes directly from user input or less trusted sources
    const title = doc.title || 'Untitled';
    const url = doc.url || '#'; // Provide a fallback URL
    const snippet = doc.snippet || 'No snippet available.';

    // Use textContent for potentially unsafe data to prevent XSS
    const titleLink = document.createElement('a');
    titleLink.href = url;
    titleLink.target = '_blank'; // Open in new tab
    titleLink.rel = 'noopener noreferrer'; // Security best practice
    titleLink.style.fontSize = '1.2em'; // Apply style via JS if needed, better in CSS
    titleLink.textContent = title;

    const titleHeader = document.createElement('h4');
    titleHeader.style.marginBottom = '3px';
    titleHeader.appendChild(titleLink);

    const urlSmall = document.createElement('small');
    urlSmall.textContent = url;

    const snippetPara = document.createElement('p');
    snippetPara.style.marginTop = '4.5px';
    snippetPara.textContent = snippet; // Use textContent for safety

    const containerDiv = document.createElement('div');
    containerDiv.style.textAlign = 'left';
    containerDiv.appendChild(titleHeader);
    containerDiv.appendChild(urlSmall);
    containerDiv.appendChild(snippetPara);

    div.appendChild(containerDiv);
    fragment.appendChild(div);
  });

  searchResultsContainer.appendChild(fragment); // Append all results at once
  currentPage = page;
  updatePaginationButtons();

  // Scroll to top of results after rendering new page
  searchResultsContainer.scrollIntoView({ behavior: 'smooth' });
}

function updatePaginationButtons() {
  pageNumberSpan.textContent = `Page ${currentPage + 1}`;

  // Disable/enable Previous button
  prevButton.disabled = (currentPage === 0);

  // Disable/enable Next button
  nextButton.disabled = ((currentPage + 1) * pageSize >= allResults.length);

   // Show/hide pagination controls based on total results
   paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
}

// Pagination Event Listeners
prevButton.addEventListener('click', () => {
  if (currentPage > 0) {
    renderPage(currentPage - 1);
  }
});

nextButton.addEventListener('click', () => {
  if ((currentPage + 1) * pageSize < allResults.length) {
    renderPage(currentPage + 1);
  }
});