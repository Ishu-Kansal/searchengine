import random
import string
import json
import sys

# --- Configuration ---
NUM_WEBSITES = 1000000
OUTPUT_FILE = "websites_data.jsonl" 

# --- Data Generation Parameters ---
TLDS = ['.com', '.org', '.net', '.io', '.co', '.edu', '.gov', '.ai', '.app', '.xyz']
WORDS = [
    "apple", "banana", "orange", "grape", "kiwi", "mango", "search", "engine",
    "web", "site", "page", "content", "information", "data", "query", "result",
    "index", "crawl", "link", "document", "text", "algorithm", "computer",
    "science", "programming", "python", "cplusplus", "java", "database", "network",
    "internet", "protocol", "http", "html", "css", "javascript", "server", "client",
    "user", "interface", "design", "develop", "software", "hardware", "cloud",
    "security", "privacy", "technology", "future", "innovation", "research", "study",
    "learn", "teach", "university", "college", "school", "book", "article", "paper",
    "news", "blog", "post", "comment", "like", "share", "social", "media", "photo",
    "video", "audio", "music", "movie", "game", "play", "work", "life", "world",
    "city", "country", "travel", "food", "health", "sport", "art", "culture",
    "history", "science", "math", "physics", "chemistry", "biology", "ecology",
    "environment", "climate", "change", "policy", "government", "law", "business",
    "finance", "market", "stock", "trade", "economy", "job", "career", "skill",
    "project", "team", "meeting", "report", "analysis", "strategy", "goal", "plan",
    "random", "generate", "list", "vector", "string", "number", "integer", "float",
    "example", "test", "debug", "run", "build", "compile", "execute", "performance",
    "memory", "cpu", "storage", "system", "os", "linux", "windows", "macos"
]
MIN_WORDS_PER_PAGE = 200   # Min number of words on a simulated page
MAX_WORDS_PER_PAGE = 1200  # Max number of words on a simulated page
MIN_DOMAIN_LEN = 5
MAX_DOMAIN_LEN = 15
MIN_PATH_LEN = 0          # Allow no path
MAX_PATH_LEN = 25
PATH_CHANCE = 0.7         # % chance of having a path component
SUBDOMAIN_CHANCE = 0.3    # % chance of having a subdomain (e.g., blog.site.com)


def generate_random_string(length, characters=string.ascii_lowercase + string.digits):
    if length <= 0:
        return ""
    return ''.join(random.choices(characters, k=length))

def generate_random_url(generated_domains):
    while True:
        # Domain part
        domain_len = random.randint(MIN_DOMAIN_LEN, MAX_DOMAIN_LEN)
        domain_main = generate_random_string(domain_len, string.ascii_lowercase)
        tld = random.choice(TLDS)
        full_domain = f"{domain_main}{tld}"

        # Optional Subdomain
        if random.random() < SUBDOMAIN_CHANCE:
            subdomain_len = random.randint(3, 8)
            subdomain = generate_random_string(subdomain_len, string.ascii_lowercase)
            full_domain = f"{subdomain}.{full_domain}"

        # Ensure domain uniqueness (important for realistic indexing)
        if full_domain not in generated_domains:
            generated_domains.add(full_domain)
            break
        # If domain exists, loop again to generate a new one

    url = f"http://{full_domain}"

    if random.random() < PATH_CHANCE:
        num_path_segments = random.randint(1, 4)
        path_parts = []
        for _ in range(num_path_segments):
            path_len = random.randint(3, MAX_PATH_LEN // num_path_segments) # Distribute length
            path_parts.append(generate_random_string(path_len, string.ascii_lowercase + string.digits + '-'))

        if random.random() > 0.6 and path_parts:
             if random.random() > 0.5:
                 path_parts[-1] += ".html"
             else:
                 path_parts[-1] += ".php"

        url += "/" + "/".join(path_parts)
    elif random.random() < 0.1: # Small chance of just root path
         url += "/"

    return url

def generate_random_content(word_list, min_words, max_words):
    num_words = random.randint(min_words, max_words)
    content = random.choices(word_list, k=num_words)
    return content # Return as a list of words

# --- Main Generation Logic ---

print(f"Generating {NUM_WEBSITES} websites to {OUTPUT_FILE}...")
generated_domains = set() # Keep track of domains to ensure URL uniqueness

try:
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        for i in range(NUM_WEBSITES):
            # Generate URL (ensuring unique domain)
            url = generate_random_url(generated_domains)

            # Generate page content (list of words)
            content_words = generate_random_content(WORDS, MIN_WORDS_PER_PAGE, MAX_WORDS_PER_PAGE)

            # Create data structure for this website
            website_data = {
                "url": url,
                "rank": random.randint(1, 1000), # Assign a random rank/score
                "content": content_words         # List of words
                # Optionally: Join words into a single string if needed:
                # "content_text": " ".join(content_words)
            }

            # Write as JSON line
            json.dump(website_data, f)
            f.write('\n')

            # Progress indicator
            if (i + 1) % 5000 == 0:
                print(f"  Generated {i + 1}/{NUM_WEBSITES}...", file=sys.stderr)

except IOError as e:
    print(f"Error writing to file {OUTPUT_FILE}: {e}", file=sys.stderr)
    sys.exit(1)

print(f"Done. Data saved to {OUTPUT_FILE}")