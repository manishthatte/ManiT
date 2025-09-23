# parse_chat.py
import sys
from bs4 import BeautifulSoup

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 parse_chat.py <input_gemini_export.html> <output.txt>")
        sys.exit(1)

    input_file_path = sys.argv[1]
    output_file_path = sys.argv[2]

    print(f"Reading from: {input_file_path}")

    try:
        with open(input_file_path, 'r', encoding='utf-8') as f:
            soup = BeautifulSoup(f, 'lxml')
    except FileNotFoundError:
        print(f"Error: Input file not found at '{input_file_path}'")
        sys.exit(1)

    # NOTE: The CSS selectors '.user-query' and '.model-response-text' are
    # based on the typical structure of a Gemini export. You may need to
    # open the HTML file in a browser and use "Inspect Element" to find
    # the correct class names for your specific export file.
    queries = soup.select('.user-query .query-text')
    responses = soup.select('.model-response-text')

    if not queries or not responses:
        print("\nWarning: Could not find chat messages with the expected CSS selectors.")
        print("Please open the HTML file to find the correct class names for user queries and model responses and update this script.")
        
    with open(output_file_path, 'w', encoding='utf-8') as f:
        # We assume for every query there is a response.
        num_messages = min(len(queries), len(responses))
        for i in range(num_messages):
            user_text = queries[i].get_text(strip=True)
            model_text = responses[i].get_text(strip=True)
            
            f.write("--- USER ---\n")
            f.write(user_text)
            f.write("\n\n--- GEMINI ---\n")
            f.write(model_text)
            f.write("\n\n========================================\n\n")

    print(f"Successfully parsed {num_messages} message pairs into: {output_file_path}")

if __name__ == '__main__':
    main()