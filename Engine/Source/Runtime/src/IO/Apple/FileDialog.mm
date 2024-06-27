#include "gpch.h"

#ifdef PLATFORM_MACOS
#include "IO/FileDialog.h"

#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

using namespace Gleam;

TArray<Filesystem::path> FileDialog::Open(const TWString& filterName, const TWString& filterExtensions)
{
    TArray<Filesystem::path> selectedFiles;
    
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    [openPanel setTitle:@"Open File"];
    [openPanel setMessage:@"Choose a file"];
    [openPanel setCanChooseFiles:YES];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setAllowsMultipleSelection:YES];
    
    if (!filterName.empty() && !filterExtensions.empty())
    {
        NSMutableArray<UTType*>* contentTypes = [NSMutableArray<UTType*> array];
        
        TWStringStream extensions(filterExtensions);
        for (TWString extension; std::getline(extensions, extension, L',');)
        {
            extension.erase(extension.begin());
            NSString* objcExtension = [[NSString alloc] initWithBytes:extension.data()
                                                               length:extension.size() * sizeof(wchar_t)
                                                             encoding:NSUTF32LittleEndianStringEncoding];
            if (UTType* type = [UTType typeWithFilenameExtension:objcExtension])
            {
                [contentTypes addObject:type];
            }
        }
        
        if ([contentTypes count] > 0)
        {
            [openPanel setAllowedContentTypes:contentTypes];
        }
    }
    
    if ([openPanel runModal] == NSModalResponseOK)
    {
        NSArray* urls = [openPanel URLs];
        for (NSURL* url in urls)
        {
            NSData* pathData = [[url path] dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
            TWString pathStr((wchar_t*)[pathData bytes], [pathData length] / sizeof(wchar_t));
            selectedFiles.push_back(Filesystem::path(pathStr));
        }
    }
    return selectedFiles;
}

#endif

