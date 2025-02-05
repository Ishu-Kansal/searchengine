import re

def count_the_in_file(filename):
    pattern = r'\bthe\b'
    count = 0

    with open(filename, 'r', encoding='utf-8') as file:
        for line in file:
            count += len(re.findall(pattern, line)) 

    return count

filename = "BigJunkHtml.txt" 
print(f"'the' appears {count_the_in_file(filename)} times in the file.")
