import { OPENAI_API_KEY } from './config.js';

let allResults = [];
let currentPage = 0;
const pageSize = 10;

// Set up an array of GIFs for the loading animation
const gifs = [
  'https://tenor.com/view/evan-peters-ip-ip-address-google-computer-gif-16211954443149729959.gif', // IP
  'https://media4.giphy.com/media/v1.Y2lkPTc5MGI3NjExMzZ4ZnpjdDVoOHlueW1kbmRtdG55NDIzcjA1aHdzaXlnam9tanp4MSZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/FArgGzk7KO14k/giphy.gif', // Bears
  'https://tenor.com/view/kerfuffle-fox-gif-18270783.gif', // Fox
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

const fetchedPages = new Set();

searchForm.addEventListener('submit', async function (e) {
  e.preventDefault();

  const query = queryInput.value.trim();
  if (!query) {
    queryInput.focus();
    return;
  }

  // Reset UI and state
  currentPage = 0;
  allResults = [];
  searchResultsContainer.innerHTML = '';
  document.getElementById('aiSummaryContainer').style.display = 'none';
  paginationControls.style.display = 'none';
  toggleAISummary();

  submitButton.classList.add('loading');
  submitButton.disabled = true;
  loadingIndicator.style.display = 'block';

  let gifIndex = Math.floor(Math.random() * gifs.length);
  loadingGif.src = gifs[gifIndex];
  gifInterval = setInterval(() => {
    gifIndex = (gifIndex + 1) % gifs.length;
    loadingGif.src = gifs[gifIndex];
  }, 2000);

  let aiText = "";

  try {
    const aiPromise = fetch("https://api.openai.com/v1/chat/completions", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "Authorization": "Bearer " + OPENAI_API_KEY
      },
      body: JSON.stringify({
        model: "gpt-4o-mini",
        messages: [
          {
            role: "system",
            content: "You are a non-interactive information assistant. When given a search query, respond with one clear, neutral, and self-contained paragraph that explains the topic. Do not include links. Do not ask questions. Do not invite further interaction."
          },
          {
            role: "user",
            content: "Search engine query: " + query
          }
        ]
      })
    });

    const searchPromise = fetch('/api/search/', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json; charset=utf-8'
      },
      body: JSON.stringify({ query })
    });

    const [searchResponse] = await Promise.all([searchPromise]);

    // Handle Search Results
    if (!searchResponse.ok) {
      const errorText = await searchResponse.text();
      throw new Error(`Search API error: ${searchResponse.status} - ${errorText}`);
    }

    const searchData = await searchResponse.json();
    console.log('Search API response completed');

    if (!Array.isArray(searchData.results)) {
      throw new Error("Invalid data format received from server.");
    }

    if (searchData.results.length === 0) {
      searchResultsContainer.innerHTML = `
        <p style="text-align: center; color: #a58d8d; margin-top: 30px;">
          No results found for your query.
        </p>`;
    } else {
      allResults = searchData.results;

      fetchedPages.clear();

      renderPage(0); // Show results right away

      // Start fetching snippets immediately after search response
      fetchSnippetsForPage(0).then(() => {
        renderPage(currentPage); // Re-render with snippets
        paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
      }).catch(error => {
        console.error('Snippet Fetch Error:', error);
      });

      paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
    }

    const [aiResponse] = await Promise.all([aiPromise]);

    // Handle AI response as soon as it's ready
    aiResponse.json().then(aiData => {
      console.log('OpenAI response completed');
      aiText = aiData.choices?.[0]?.message?.content || "";
      displayAISummary(aiText);
    }).catch(error => {
      console.error("AI Summary Error:", error);
    });

  } catch (error) {
    console.error('Search Error:', error);
    searchResultsContainer.innerHTML = `
      <p style="text-align: center; color: #ec2225; margin-top: 30px;">
        An error occurred: ${error.message}. Please try again.
      </p>`;
  } finally {
    clearInterval(gifInterval);
    gifInterval = null;
    loadingIndicator.style.display = 'none';
    submitButton.classList.remove('loading');
    submitButton.disabled = false;
  }
});

async function fetchSnippetsForPage(pageIndex) {
  const start = pageIndex * pageSize;
  const end = Math.min(start + pageSize, allResults.length);
  const pageResults = allResults.slice(start, end);

  const urlsToFetch = pageResults
    .filter(doc => !doc.snippet)
    .map(doc => doc.url);

  if (urlsToFetch.length === 0) {
    return;
  }

  try {
    const snippetResponse = await fetch('/api/snippets/', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json; charset=utf-8'
      },
      body: JSON.stringify({ urls: urlsToFetch })
    });

    if (!snippetResponse.ok) {
      throw new Error(`Snippet API error: ${snippetResponse.status}`);
    }

    const snippetData = await snippetResponse.json();

    // Update allResults with fetched snippets
    snippetData.results.forEach(updatedDoc => {
      const index = allResults.findIndex(doc => doc.url === updatedDoc.url);
      if (index !== -1) {
        allResults[index].snippet = updatedDoc.snippet;
        allResults[index].title = updatedDoc.title;
      }
    });

    fetchedPages.add(pageIndex); // Mark this page as fetched
  } catch (error) {
    console.error('Snippet Fetch Error:', error);
  }
}

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
    const title = doc.title || doc.url;
    const url = doc.url || '#'; // Provide a fallback URL
    const snippet = doc.snippet;

    // Use textContent for potentially unsafe data to prevent XSS
    const titleLink = document.createElement('a');
    titleLink.href = url;
    titleLink.target = '_blank'; // Open in new tab
    titleLink.rel = 'noopener noreferrer'; // Security best practice
    titleLink.style.fontSize = '1.2em'; // Apply style via JS if needed, better in CSS
    titleLink.textContent = title;

    const titleHeader = document.createElement('h4');
    titleHeader.style.marginBottom = '5px';
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
    let newPage = currentPage - 1;
    renderPage(newPage);
    if (!fetchedPages.has(newPage)) {
      fetchSnippetsForPage(newPage).then(() => {
        renderPage(newPage); // Re-render with snippets
        paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
      }).catch(error => {
        console.error('Snippet Fetch Error:', error);
      });
    }
  }
});

nextButton.addEventListener('click', () => {
  if ((currentPage + 1) * pageSize < allResults.length) {
    let newPage = currentPage + 1;
    renderPage(newPage);
    if (!fetchedPages.has(newPage)) {
      fetchSnippetsForPage(newPage).then(() => {
        renderPage(newPage); // Re-render with snippets
        paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
      }).catch(error => {
        console.error('Snippet Fetch Error:', error);
      });
    }
  }
});

function toggleAISummary() {
  const content = document.getElementById('aiSummaryContent');
  const icon = document.getElementById('aiToggleIcon');
  if (content.style.display === 'none') {
    content.style.display = 'block';
    document.getElementsByClassName('panel-heading')[0].style.borderBottomLeftRadius = '0px';
    document.getElementsByClassName('panel-heading')[0].style.borderBottomRightRadius = '0px';
    icon.classList.remove('glyphicon-chevron-down');
    icon.classList.add('glyphicon-chevron-up');
  }
  else {
    content.style.display = 'none';
    document.getElementsByClassName('panel-heading')[0].style.borderBottomLeftRadius = '16px';
    document.getElementsByClassName('panel-heading')[0].style.borderBottomRightRadius = '16px';
    icon.classList.remove('glyphicon-chevron-up');
    icon.classList.add('glyphicon-chevron-down');
  }
}

window.toggleAISummary = toggleAISummary;

function displayAISummary(summaryText) {
  const summaryContainer = document.getElementById('aiSummaryContainer');
  const summaryTextEl = document.getElementById('aiSummaryText');

  summaryTextEl.textContent = summaryText;
  summaryContainer.style.display = 'block';
  document.getElementById('aiSummaryContent').style.display = 'block';
  document.getElementsByClassName('panel-heading')[0].style.borderBottomLeftRadius = '0px';
  document.getElementsByClassName('panel-heading')[0].style.borderBottomRightRadius = '0px';
  document.getElementById('aiToggleIcon').classList.remove('glyphicon-chevron-down');
  document.getElementById('aiToggleIcon').classList.add('glyphicon-chevron-up');
}
