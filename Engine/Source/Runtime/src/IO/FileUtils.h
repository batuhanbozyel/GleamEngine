#pragma once
#include <fstream>

namespace Gleam {

namespace FileUtils {

static TArray<char> ReadBinaryFile(const TStringView fileName)
{
	std::ifstream file(fileName.data(), std::ios::ate | std::ios::binary);
	GLEAM_ASSERT(file.is_open());

	size_t size = file.tellg();
	TArray<char> buffer(size);

	file.seekg(0);
	file.read(buffer.data(), size);

	return buffer;
}

} // namespace FileUtils

} // namespace Gleam