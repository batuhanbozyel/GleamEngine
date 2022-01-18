#pragma once
#include <fstream>

namespace Gleam {

namespace IOUtils {

static TArray<uint8_t> ReadBinaryFile(const TStringView filename)
{
	auto filepath = std::filesystem::current_path().append(filename);
	std::ifstream file(filepath.c_str(), std::ios::ate | std::ios::binary);
	GLEAM_ASSERT(file.is_open());

	size_t size = file.tellg();
	TArray<uint8_t> buffer(size);

	file.seekg(0);
	file.read(As<char*>(buffer.data()), size);

	return buffer;
}

static void ExecuteCommand(const TString& cmd)
{
	int success = system((cmd + " > command.err 2>&1").c_str());
	GLEAM_ASSERT(success == 0);
}

} // namespace FileUtils

} // namespace Gleam