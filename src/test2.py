import json

count = 0

with open("websites_data.jsonl", "r", encoding="utf-8") as file:
    for line in file:
        try:
            data = json.loads(line)
            content = data.get("content", [])
            if "apple" in content and "banana" in content:
                count += 1
        except json.JSONDecodeError:
            continue  # skip malformed lines

print(f"Number of URLs with 'apple' or 'banana': {count}")
