#include <spdlog/spdlog.h>

#include <fstream>
#include <sstream>

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

#include <text_processor.h>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    spdlog::error("Error: Path to the text is required.");
    return -1;
  }

  std::string filepath(argv[1]);
  std::ifstream file(filepath);

  if (!file.is_open()) {
    spdlog::error("Error: Can't open the file {}", filepath);
    return 1;
  }

  file.seekg(0, std::ios::end);
  std::size_t size = file.tellg();
  file.seekg(0);

  std::string text(size, '\0');
  file.read(text.data(), size);

  spdlog::info("Text is loaded. Text size in bytes: {}", text.size());

  spdlog::info("Start preprocessing.");
  lab2::prepareText(text);
  spdlog::info("Preprocessing end.");
  spdlog::info("Preprocessing end. Text size after preprocessing: {}.",
               text.size());

  return 0;
}
