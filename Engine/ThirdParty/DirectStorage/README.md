# DirectStorage SDK

This package contains the DirectStorage SDK, including the redistributable binaries.

For help getting started and other information for DirectStorage, please see:

https://aka.ms/DirectStorage

The included licenses apply to the following files:

- **LICENSE.txt** : applies to all files under `native/bin/`
- **LICENSE-CODE.txt** : applies to all files under `native/include/`
# Changelog

## 1.2.3
- Recompiled built-in GPU decompression shaders with DXC version 1.8.2405.

### Bug Fixes

- Fix deadlock in built-in decompression threadpool.
- Disable asserts for release builds.
- Fix race condition in file IO stack when multiple threads were submitting 
  read requests.

## 1.2.2

### Bug Fixes

- Fix race condition that would, in some rare circumstances, cause DirectStorage
  to stop processing requests when the CPU is under high load.

- Change DirectStorage worker thread to wakeup periodically and process requests
  to catch rare cases where processing has stopped.

### Breaking Changes
- Remove support for 32-bit ARM.


## 1.2.1
- Ensure that the destination resource is left in the D3D12_RESOURCE_STATE_COMMON state after the DirectStorage request has completed.

## 1.2.0

### New Features
- Add support for enabling buffered file IO for use on HDDs that may benefit from OS file caching behaviors.
- Add IDStorageQueue2::GetCompressionSupport API to indicate what path the DirectStorage runtime will take when decompressing a supported GPU decompression format.
- Update dstorage.h and dstorageerr.h to be covered by the MIT License.
- Add Microsoft.Direct3D.DirectStorage.winmd, to ease generation of non-C++ bindings to the API.

### Bug Fixes
- Add "Reserved1" field to DSTORAGE_REQUEST_OPTIONS.  This makes the in-memory layout of the structure more explicit, but doesn't actually change the layout from previous versions.
- Fix DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION for 3D textures.
- Fix scheduling issue that manifested when transferring uncompressed data from memory to buffers

### Performance improvements
- Move the copy after GPU decompression onto the compute queue for GPUs where this is faster.

## 1.1.1
- Fix region size calculation for texture destinations with DepthOrArraySize > 1 which caused bogus DirectStorage Error/Warning message DSTORAGE_MESSAGE_ID_SMALLER_UNCOMPRESSED_SIZE

## 1.1.0

### New Features
- Add support for GPU decompression and GDeflate:
  - New compression format: `DSTORAGE_COMPRESSION_FORMAT_GDEFLATE`
  - New interface: `IDStorageCustomCompressionQueue1`
  - New function: `DStorageCreateCompressionCodec`
  - New interface: `IDStorageCompressionCodec`
- New interface: `IDStorageQueue1`:
  - New method `EnqueueSetEvent()` that allows a Win32 event to be signaled directly by DirectStorage.
- Add more warnings for invalid queue capacity, and invalid request fields.

### Bug Fixes
- Fix hang when processing memory sourced requests that exceed in total the total size of the available staging buffer size.
- Fix running on Windows Server 2016.
- Add note in `dstorage.h` about how `IDStorageFile::Close()` doesn't affect the reference count.
- Fail enqueued requests that require a D3D device on queues that were created without one.
- For requests to GPU destinations (e.g. buffers, subresources), support requests that have uncompressed size greater than the destination size.

### Breaking Changes

1.1.0 is source compatible with 1.0.2, but recompilation will be required so that the DirectStorage binaries match the header:

- DSTORAGE_ERROR_PARAMETERS_REQUEST::RequestName size changed from MAX_PATH to DSTORAGE_REQUEST_MAX_NAME.
- New DSTORAGE_CONFIGURATION fields.

This is a trade-off between having a simpler API and binary compatibility.  As DirectStorage is now more feature complete with GPU decompression, binary compatibility will become a higher priority.

### Known Issues
- On older versions of D3D12, the debug layer may generate incorrect warnings that there are simultaneous writes to the input staging buffer from the copy queue that brings the data into the buffer and the compute queue that performs GPU decompression.
  - As a workaround, you can use the Agility SDK: https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/.



## 1.0.2

### Breaking Changes

* Update NuGet package layout to be consistent with other native NuGet packages.

### New Features

* Add `DSTORAGE_CUSTOM_DECOMPRESSION_FLAGS`; these can be used to determine if the destination for a custom decompression requests is in write-combined memory.

### Bug Fixes

* Fix building on x86.
* Improve documentation.

### Performance Improvements

* Improve Windows 10 performance.

## 1.0.0

First release.
