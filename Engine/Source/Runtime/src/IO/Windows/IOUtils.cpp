#include "gpch.h"

#ifdef PLATFORM_WINDOWS
#include "IO/IOUtils.h"

#include <Windows.h>
#include <shobjidl.h>

using namespace Gleam;

TArray<Filesystem::path> IOUtils::OpenFileDialog(const TWString& filterName, const TWString& filterExtensions)
{
	TArray<Filesystem::path> selectedFiles;

	if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return {};

	IFileOpenDialog* pFileOpenDialog;
	if (!SUCCEEDED(CoCreateInstance(
		CLSID_FileOpenDialog,
		NULL,
		CLSCTX_ALL,
		IID_IFileOpenDialog,
		reinterpret_cast<void**>(&pFileOpenDialog)
	))){ return {};}

	// Set File Open Dialog options
	FILEOPENDIALOGOPTIONS options;
	if (!SUCCEEDED(pFileOpenDialog->GetOptions(&options))) return {};
	if (!SUCCEEDED(pFileOpenDialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT))) return {};

	// Set the initial directory
	IShellItem* pInitialDirItem = NULL;
	const auto& assetPath = Filesystem::current_path();
	if (SUCCEEDED(SHCreateItemFromParsingName(assetPath.c_str(), NULL, IID_PPV_ARGS(&pInitialDirItem))))
	{
		pFileOpenDialog->SetDefaultFolder(pInitialDirItem);
		pInitialDirItem->Release();
	}

	COMDLG_FILTERSPEC filterSpec;
	filterSpec.pszName = filterName.c_str();
	filterSpec.pszSpec = filterExtensions.c_str();

	pFileOpenDialog->SetFileTypes(1, &filterSpec);
	pFileOpenDialog->SetFileTypeIndex(1);

	// Show the File Open Dialog
	if (!SUCCEEDED(pFileOpenDialog->Show(NULL))) return {};

	IShellItemArray* pItems = NULL;
	if (SUCCEEDED(pFileOpenDialog->GetResults(&pItems)))
	{
		DWORD numItems = 0;
		if (SUCCEEDED(pItems->GetCount(&numItems)))
		{
			for (DWORD index = 0; index < numItems; index++)
			{
				IShellItem* pItem = NULL;
				if (SUCCEEDED(pItems->GetItemAt(index, &pItem)))
				{
					PWSTR pszFilePath;
					if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
					{
						auto path = Filesystem::path(pszFilePath);
						selectedFiles.push_back(path);
						CoTaskMemFree(pszFilePath);
					}
					pItem->Release();
				}
			}
		}
		pItems->Release();
	}

	pFileOpenDialog->Release();
	CoUninitialize();

    return selectedFiles;
}

#endif

