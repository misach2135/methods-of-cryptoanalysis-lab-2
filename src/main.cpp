#include <fstream>
#include <iostream>
#include <sstream>

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

#include <text_processor.h>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Error: Path to the public text is required" << std::endl;
    return -1;
  }

  std::string filepath(argv[1]);

  std::ifstream file(filepath);

  if (!file.is_open()) {
    std::cerr << "Error: Can't open the file." << std::endl;
    return -1;
  }

  file.seekg(0, std::ios::end);
  std::size_t size = file.tellg();
  file.seekg(0);

  std::string input_text(size, '\0');
  file.read(input_text.data(), size);

  std::string test = lab2::prepareText(input_text);
  std::cout << test << std::endl;
  return 0;
}
