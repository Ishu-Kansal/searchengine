import { OPENAI_API_KEY } from './config.js';

let allResults = [];
let currentPage = 0;
const pageSize = 10;

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

const fetchedPages = new Set();

let gifTimeout = null;
let gifInterval = null;

// Set up an array of GIFs for the loading animation
const gifs = [
  'https://tenor.com/view/evan-peters-ip-ip-address-google-computer-gif-16211954443149729959.gif', // IP
  'https://media4.giphy.com/media/v1.Y2lkPTc5MGI3NjExMzZ4ZnpjdDVoOHlueW1kbmRtdG55NDIzcjA1aHdzaXlnam9tanp4MSZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/FArgGzk7KO14k/giphy.gif', // Bears
  'https://tenor.com/view/kerfuffle-fox-gif-18270783.gif', // Fox
  'https://media0.giphy.com/media/v1.Y2lkPTc5MGI3NjExeWcwM3U1dGl1aGIyamgzbWNqcDE1MXRkenM0OTBudHRuNnF5NGl0NiZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/E6jscXfv3AkWQ/giphy.gif', // Cat
  'https://media3.giphy.com/media/v1.Y2lkPTc5MGI3NjExbjBuMHI2b3dpZWpzbzF1eGF0enhvcWwzM2duZHh2b3FnYzZhbXNhNiZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/42wQXwITfQbDGKqUP7/giphy.gif', // Pikachu
  'https://media2.giphy.com/media/v1.Y2lkPTc5MGI3NjExbTY0MmUxMHlpN3dycmt0cWl4aWJ2NTJ6ZG50c3B5Zzk3eWNyZHdydyZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/A53vF9xNk7AKnQPLDs/giphy.gif' // Cookie monster
];

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
  aiSummaryImageContainer.style.display = 'none';
  toggleAISummary();
  document.getElementById('aiSummaryTitle').innerText = 'AI Summary';

  submitButton.classList.add('loading');
  submitButton.disabled = true;
  loadingIndicator.style.display = 'block';

  document.getElementById('searchSummaryBox').style.display = 'none';

  let gifIndex = Math.floor(Math.random() * gifs.length);
  gifTimeout = null;

  // Hide GIF initially
  loadingGif.style.display = 'none';
  loading.style.display = 'none';
  loadingGif.src = ''; // Clear any existing GIF

  // Start a delayed timer to show the GIF after .25 second
  gifTimeout = setTimeout(() => {
    loadingGif.src = gifs[gifIndex];
    loadingGif.style.display = 'block';
    loading.style.display = 'block';
    gifInterval = setInterval(() => {
      gifIndex = (gifIndex + 1) % gifs.length;
      loadingGif.src = gifs[gifIndex];
    }, 2000);
  }, 1000);

  let aiText = "";

  try {
    const aiPromise = fetch("https://api.openai.com/v1/chat/completions", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
        "Authorization": "Bearer " + OPENAI_API_KEY
      },
      body: JSON.stringify({
        model: "gpt-4.1-nano",
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

    // Display searchData.summary above results
    if (searchData.summary) {
      const searchSummaryBox = document.getElementById('searchSummaryBox');
      const searchSummaryText = document.getElementById('searchSummaryText');

      searchSummaryText.textContent = searchData.summary;
      searchSummaryBox.style.display = 'block';
    } else {
      document.getElementById('searchSummaryBox').style.display = 'none';
    }

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
    clearTimeout(gifTimeout);
    gifTimeout = null;

    clearInterval(gifInterval);
    gifInterval = null;

    loadingGif.style.display = 'none'; // Ensure GIF is hidden
    loadingGif.src = ''; // Clear GIF source
    
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

function scrollToMaintainBottomOffset(offsetFromBottom) {
  const totalHeight = document.body.scrollHeight;
  window.scrollTo({
    top: totalHeight - offsetFromBottom,
    behavior: 'instant'
  });
}

// Pagination Event Listeners
prevButton.addEventListener('click', () => {
  if (currentPage > 0) {
    let offsetFromBottom = document.body.scrollHeight - window.scrollY;

    let newPage = currentPage - 1;
    renderPage(newPage);
    scrollToMaintainBottomOffset(offsetFromBottom);

    if (!fetchedPages.has(newPage)) {
      fetchSnippetsForPage(newPage).then(() => {
        if (currentPage == newPage) {
          //offsetFromBottom = document.body.scrollHeight - window.scrollY;
          renderPage(currentPage);
          //scrollToMaintainBottomOffset(offsetFromBottom);
        }
        paginationControls.style.display = allResults.length > pageSize ? 'block' : 'none';
      }).catch(error => {
        console.error('Snippet Fetch Error:', error);
      });
    }
  }
});

nextButton.addEventListener('click', () => {
  if ((currentPage + 1) * pageSize < allResults.length) {
    let offsetFromBottom = document.body.scrollHeight - window.scrollY;

    let newPage = currentPage + 1;
    renderPage(newPage);
    scrollToMaintainBottomOffset(offsetFromBottom);

    if (!fetchedPages.has(newPage)) {
      fetchSnippetsForPage(newPage).then(() => {
        if (currentPage == newPage) {
          //offsetFromBottom = document.body.scrollHeight - window.scrollY;
          renderPage(currentPage);
          //scrollToMaintainBottomOffset(offsetFromBottom);
        }
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

const imageSearchBtn = document.querySelector('.imageSearchBtn');
const imageInput = document.getElementById('imageInput');

// Click triggers file select
imageSearchBtn.addEventListener('click', () => {
  imageInput.click();
});

// Handle file input
imageInput.addEventListener('change', (event) => {
  const file = event.target.files[0];
  if (file) {
    handleImageSearch(event, file);
  }
});

//const imageSearchIcon = document.getElementById('searchForm');
const imageSearchIcon = document.getElementsByTagName('body')[0];

// Prevent default drag behavior on the image button itself
imageSearchIcon.addEventListener('dragover', (e) => {
  e.preventDefault();
  imageSearchIcon.classList.add('drag-hover');
});

imageSearchIcon.addEventListener('dragleave', (e) => {
  e.preventDefault();
  imageSearchIcon.classList.remove('drag-hover');
});

imageSearchIcon.addEventListener('drop', (e) => {
  e.preventDefault();
  imageSearchIcon.classList.remove('drag-hover');
  const file = e.dataTransfer.files[0];
  if (file && file.type.startsWith('image/')) {
    handleImageSearch(e, file);
  }
});

async function handleImageSearch(e, file) {
  e.preventDefault();

  let questionText = "Describe what is in the image in as few words as possible, no more than 3 words";
  console.log("Image selected:", file);

  // Reset UI and state
  currentPage = 0;
  allResults = [];
  searchResultsContainer.innerHTML = '';
  document.getElementById('aiSummaryContainer').style.display = 'none';
  paginationControls.style.display = 'none';
  aiSummaryImageContainer.style.display = 'none';
  document.getElementById('queryInput').value = 'Looking over the image...';
  document.getElementById('aiSummaryTitle').innerText = 'Image Search';
  toggleAISummary();
  document.getElementById('aiSummaryTitle').innerText = 'Image Search';

  //imageSearchBtn.classList.add('loading');
  //imageSearchBtn.disabled = true;
  loadingIndicator.style.display = 'block';

  document.getElementById('searchSummaryBox').style.display = 'none';

  clearTimeout(gifTimeout);
  gifTimeout = null;

  clearInterval(gifInterval);
  gifInterval = null;

  let gifIndex = Math.floor(Math.random() * gifs.length);

  gifTimeout = setTimeout(() => {
    loadingGif.src = gifs[gifIndex];
    loadingGif.style.display = 'block';
    loading.style.display = 'block';
    gifInterval = setInterval(() => {
      gifIndex = (gifIndex + 1) % gifs.length;
      loadingGif.src = gifs[gifIndex];
    }, 2000);
  }, 0);

  try {
    // Convert image to PDF
    const { jsPDF } = window.jspdf;
    const pdf = new jsPDF();
    
    const img = new Image();
    const imageUrl = URL.createObjectURL(file);

    // Show the image in the image display area
    // const aiSummaryImageContainer = document.getElementById('aiSummaryImageContainer');
    // const uploadedImage = document.getElementById('uploadedImage');
    // uploadedImage.src = imageUrl;
    // aiSummaryImageContainer.style.display = 'block';
    
    img.onload = async () => {
      try {
        // Add the image to the PDF
        pdf.addImage(img, 'JPEG', 10, 10, 180, 160);

        // Generate PDF Blob
        const pdfBlob = pdf.output('blob');

        // Upload the PDF Blob
        const formData = new FormData();
        formData.append("file", pdfBlob, "converted_image.pdf");
        formData.append("purpose", "user_data");

        const uploadResponse = await fetch("https://api.openai.com/v1/files", {
          method: "POST",
          headers: {
            Authorization: "Bearer " + OPENAI_API_KEY
          },
          body: formData
        });

        const uploadData = await uploadResponse.json();
        if (!uploadData.id) throw new Error("File upload failed.");
        const fileId = uploadData.id;
        console.log("Uploaded file ID:", fileId);

        // Query using file_id
        const response = await fetch("https://api.openai.com/v1/responses", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
            "Authorization": "Bearer " + OPENAI_API_KEY
          },
          body: JSON.stringify({
            model: "gpt-4.1-mini",
            input: [
              {
                role: "user",
                content: [
                  {
                    type: "input_file",
                    file_id: fileId
                  },
                  {
                    type: "input_text",
                    text: questionText
                  }
                ]
              }
            ]
          })
        });

        const data = await response.json();
        console.log(data);
        let resultText = data.output[0].content[0].text;
        if (resultText.endsWith(".")) {
          resultText = resultText.slice(0, -1);
        }

        if (!resultText) throw new Error("No valid response from API.");
        console.log("Response:", resultText);
        queryInput.value = resultText;
        searchForm.dispatchEvent(new Event('submit'));

      } 
      catch (error) {
        console.error("Error during image processing:", error);
      } 
      finally {
        loadingIndicator.style.display = 'none';
        //imageSearchBtn.classList.remove('loading');
        //imageSearchBtn.disabled = false;

        // Show the image in the image display area
        document.getElementById('aiSummaryImage').src = imageUrl;
        document.getElementById('aiSummaryImageContainer').style.display = 'block';
        document.getElementById('aiSummaryTitle').innerText = 'Image Search';
      }
    };

    // Start loading the image
    img.src = imageUrl; 

  }
  catch (error) {
    console.error("Error:", error.message);
    throw error;
  }
}

document.getElementById("imageInput").addEventListener("change", function (event) {
  const file = event.target.files[0];
  if (file) {
    const reader = new FileReader();
    reader.onload = function (e) {
      document.getElementById("aiSummaryImage").src = e.target.result;
      document.getElementById("aiSummaryImageContainer").style.display = "block";
    };
    reader.readAsDataURL(file);
  }
});
