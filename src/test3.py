import json
import sys
import random # Still needed for rank

# --- Configuration ---
NUM_WEBSITES = 128
OUTPUT_FILE = "websites_data.jsonl"
# Removed constants related to random generation (TLDS, MIN/MAX_LEN, etc.)

# --- Removed Unused Functions ---
# generate_random_string
# generate_random_url
# generate_random_content

# --- Main Generation Logic ---

print(f"Generating {NUM_WEBSITES} websites to {OUTPUT_FILE}...")
# Removed generated_domains set as URLs are now deterministic and unique

try:
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        for i in range(NUM_WEBSITES):
            # Generate URL based on index 'i'
            url = f"https://{i}.com" # New URL format

            # Generate content based on index 'i'
            content = str(i)         # New content format

            # Create data structure for this website
            website_data = {
                "url": url,
                "rank": 1, # Keep random rank for now
                "content": content               # Assign the new content
            }

            # Write as JSON line
            json.dump(website_data, f)
            f.write('\n')

            # Progress indicator (kept for consistency, threshold might be high)
            if (i + 1) % 5000 == 0:
                print(f"  Generated {i + 1}/{NUM_WEBSITES}...", file=sys.stderr)

except IOError as e:
    print(f"Error writing to file {OUTPUT_FILE}: {e}", file=sys.stderr)
    sys.exit(1)

print(f"Done. Data saved to {OUTPUT_FILE}")