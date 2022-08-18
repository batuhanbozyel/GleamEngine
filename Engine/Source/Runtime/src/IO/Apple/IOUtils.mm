#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "IO/IOUtils.h"
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

using namespace Gleam;

TArray<Filesystem::path> IOUtils::OpenFileDialog()
{
    TArray<Filesystem::path> selectedFiles;
    
    NSOpenPanel* fileDialog = [NSOpenPanel openPanel];
    [fileDialog setCanChooseFiles:YES];
    [fileDialog setAllowsMultipleSelection:YES];
    [fileDialog setCanChooseDirectories:NO];
    [fileDialog setAllowedContentTypes:[UTType typesWithTag:@"" tagClass:UTTagClassFilenameExtension conformingToType:nil]];
    
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

