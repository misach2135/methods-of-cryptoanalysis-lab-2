#include <fstream>
#include <iostream>
#include <sstream>

// TODO: Variant 4 = 1.0-1.3, 3.0, 5.1

#include <text_processor.h>

int main(int argc, char* argv[]) {
  std::string test = lab2::processText(u8"hello world. Привіт Світ");
  std::cout << test << std::endl;
  return 0;
}
