#pragma once

#include <fstream>
#include <iostream>

// stream string operations derived from:
// Optimized C++ by Kurt Guntheroth (O’Reilly).
// Copyright 2016 Kurt Guntheroth, 978-1-491-92206-4

namespace fileops
{

inline static std::streamoff streamSize(std::istream& file)
{
  std::istream::pos_type current_pos = file.tellg();
  if (current_pos == std::istream::pos_type(-1)) {
    return -1;
  }
  file.seekg(0, std::istream::end);
  std::istream::pos_type end_pos = file.tellg();
  file.seekg(current_pos);
  return end_pos - current_pos;
}

inline bool streamReadString(std::istream& file, std::string& file_contents)
{
  std::streamoff len = streamSize(file);
  if (len == -1) {
    return false;
  }

  file_contents.resize(static_cast<std::string::size_type>(len));

  file.read(&file_contents[0], file_contents.length());
  return true;
}

inline bool readFile(const std::string& filename, std::string& file_contents)
{
  std::ifstream file(filename, std::ios::binary);

  if (!file.is_open()) {
    return false;
  }

  return streamReadString(file, file_contents);
}

} // namespace fileops
