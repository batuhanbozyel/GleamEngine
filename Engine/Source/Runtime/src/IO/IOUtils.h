#pragma once
#include <fstream>

namespace Gleam {

namespace IOUtils {

static TArray<uint8_t> ReadBinaryFile(const Filesystem::path& filepath)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        TArray<uint8_t> buffer(size);

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        return buffer;
    }

	GLEAM_CORE_ERROR("File {0} could not be opened!", filepath.string());
	return {};
}

static TString ReadFile(const Filesystem::path& filepath)
{
    std::ifstream file(filepath);

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        TString buffer(size, 0);

        file.seekg(0);
        file.read(buffer.data(), size);

        return buffer;
    }

	GLEAM_CORE_ERROR("File {0} could not be opened!", filepath.string());
	return TString();
}

static void ExecuteCommand(const TString& cmd)
{
    int success = system((cmd + " > command.err 2>&1").c_str());
    GLEAM_ASSERT(success == 0);
}

TArray<Filesystem::path> OpenFileDialog(const TWString& filterName = L"All Files", const TWString& filterExtensions = L"*.*");

} // namespace FileUtils

} // namespace Gleam
