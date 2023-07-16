#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "IO/IOUtils.h"

using namespace Gleam;

TArray<Filesystem::path> IOUtils::OpenFileDialog(const TArray<FileType>& filterTypes)
{
    TArray<Filesystem::path> selectedFiles;
	
    return selectedFiles;
}

#endif

