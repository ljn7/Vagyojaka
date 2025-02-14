# Read the .dic file and clean it
print("Input filename: ")
input_filename = input("Input file name: ")
print("Output filename: ")
output_filename = input("Input file name: ")
with open(f"{input_filename}.dic", "r", encoding="utf-8") as dic_file:
    # Split the word by "/" and take the first part
    words = [line.split("/")[0].strip() for line in dic_file]

# Save the cleaned words to a .txt file
with open(f"{output_filename}.txt", "w", encoding="utf-8") as txt_file:
    txt_file.write("\n".join(words))

print("Cleaned and saved to output.txt")
