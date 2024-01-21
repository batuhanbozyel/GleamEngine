#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "IO/IOUtils.h"
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

using namespace Gleam;

TArray<Filesystem::path> IOUtils::OpenFileDialog(const TWString& filterName, const TWString& filterExtensions)
{
    TArray<Filesystem::path> selectedFiles;
    
    NSOpenPanel* fileDialog = [NSOpenPanel openPanel];
    [fileDialog setCanChooseFiles:YES];
    [fileDialog setAllowsMultipleSelection:YES];
    [fileDialog setCanChooseDirectories:NO];
    
    NSMutableArray<UTType*>* contentTypes = [NSMutableArray<UTType*> array];
    for (auto filterType : filterTypes)
    {
        auto fileExtensions = FileTypeSupportedExtensions(filterType);
        for (const auto& extension : fileExtensions)
        {
            if (!extension.empty())
            {
                [contentTypes addObject:[UTType typeWithFilenameExtension:[NSString stringWithUTF8String:extension.data()]]];
            }
        }
    }
    [fileDialog setAllowedContentTypes:contentTypes];
    
    if ([fileDialog runModal] == NSModalResponseOK)
    {
        NSArray* urls = [fileDialog URLs];
        selectedFiles.resize([urls count]);
        for(int i = 0; i < [urls count]; i++ )
        {
            NSURL* url = [urls objectAtIndex:i];
            selectedFiles[i] = Filesystem::path([url.path UTF8String]);
        }
    }
    [fileDialog close];
    
    return selectedFiles;
}

#endif

