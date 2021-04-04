#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>

struct MemoryMap {
    UINTN buffer_size;
    VOID *buffer;
    UINTN map_size;
    UINTN map_key;
    UINTN descriptor_size;
    UINT32 descriptor_version;
};

#define CASE_STRING(memtype) case memtype: return L"" #memtype

const CHAR16 *GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
    switch (type) {
        CASE_STRING(EfiReservedMemoryType);
        CASE_STRING(EfiLoaderCode);
        CASE_STRING(EfiLoaderData);
        CASE_STRING(EfiBootServicesCode);
        CASE_STRING(EfiBootServicesData);
        CASE_STRING(EfiRuntimeServicesCode);
        CASE_STRING(EfiRuntimeServicesData);
        CASE_STRING(EfiConventionalMemory);
        CASE_STRING(EfiUnusableMemory);
        CASE_STRING(EfiACPIReclaimMemory);
        CASE_STRING(EfiACPIMemoryNVS);
        CASE_STRING(EfiMemoryMappedIO);
        CASE_STRING(EfiMemoryMappedIOPortSpace);
        CASE_STRING(EfiPalCode);
        CASE_STRING(EfiPersistentMemory);
        CASE_STRING(EfiMaxMemoryType);
        default: return L"InvalidMemoryType";
    }
}

const CHAR16 *GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT format) {
    switch (format) {
        CASE_STRING(PixelRedGreenBlueReserved8BitPerColor);
        CASE_STRING(PixelBlueGreenRedReserved8BitPerColor);
        CASE_STRING(PixelBitMask);
        CASE_STRING(PixelBltOnly);
        CASE_STRING(PixelFormatMax);
        default: return L"InvalidPixelFormat";
    }
}

EFI_STATUS GetMemoryMap(struct MemoryMap *memmap) {
    if (memmap->buffer == NULL) {
        return EFI_BUFFER_TOO_SMALL;
    }

    memmap->map_size = memmap->buffer_size;
    return gBS->GetMemoryMap(
        &memmap->map_size,
        (EFI_MEMORY_DESCRIPTOR *)memmap->buffer,
        &memmap->map_key,
        &memmap->descriptor_size,
        &memmap->descriptor_version
    );
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL **root_dir) {
    // what is handle, and what is protocol???
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    fs->OpenVolume(fs, root_dir);
    
    return EFI_SUCCESS;
}

EFI_STATUS SaveMemoryMap(struct MemoryMap *memmap, EFI_FILE_PROTOCOL *file) {
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header = "Index, Type, Type (name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    file->Write(file, &len, header);

    Print(L"memmap->buffer = %08lx, memmap->map_size = %08lx\n", memmap->buffer, memmap->map_size);

    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for (
        iter = (EFI_PHYSICAL_ADDRESS)memmap->buffer, i = 0;
        iter < (EFI_PHYSICAL_ADDRESS)memmap->buffer + memmap->map_size;
        iter += memmap->descriptor_size, i++
    ) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)iter;
        len = AsciiSPrint(
            buf,
            sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i, // index
            desc->Type,
            GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart,
            desc->NumberOfPages,
            desc->Attribute
        );
        file->Write(file, &len, buf);
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
    UINTN num_gop_handles = 0;
    EFI_HANDLE *gop_handles = NULL;
    gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles
    );

    gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID **)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table
) {
    Print(L"Hello, Ponkan World!\n");

    // init memory map buffer
    CHAR8 memmap_buf[4096 * 4]; // page size: 4
    struct MemoryMap memmap = { sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0 };
    GetMemoryMap(&memmap);

    // open memory map file
    EFI_FILE_PROTOCOL *root_dir;
    OpenRootDir(image_handle, &root_dir);

    EFI_FILE_PROTOCOL *memmap_file;
    root_dir->Open(
        root_dir,
        &memmap_file,
        L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );

    SaveMemoryMap(&memmap, memmap_file);
    memmap_file->Close(memmap_file);
    Print(L"memmap saved.\n");

    // try graphics
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    OpenGOP(image_handle, &gop);
    Print(
        L"Resolution: %ux%u, Pixel Format: %s, %u pixels/line\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        GetPixelFormatUnicode(gop->Mode->Info->PixelFormat),
        gop->Mode->Info->PixelsPerScanLine
    );
    Print(
        L"Frame buffer: 0x%0lx - 0x%0lx, Size: %lu bytes\n",
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferBase + gop->Mode->FrameBufferSize,
        gop->Mode->FrameBufferSize
    );

    UINT8 *frame_buffer = (UINT8 *)gop->Mode->FrameBufferBase;
    for (UINTN i = 0; i < gop->Mode->FrameBufferSize; ++i) {
        frame_buffer[i] = 255; // 0x11111111
    }

    // load kernel.elf
    EFI_FILE_PROTOCOL *kernel_file;
    root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ,
        0
    );

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16)*12;
    UINT8 file_info_buf[file_info_size];
    kernel_file->GetInfo(
        kernel_file,
        &gEfiFileInfoGuid,
        &file_info_size, // in out
        file_info_buf // out
    );

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buf;
    UINTN kernel_file_size = file_info->FileSize;

    EFI_PHYSICAL_ADDRESS kernel_base_address = 0x100000;
    gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        (kernel_file_size + 0x0fff) / 0x1000, // 1 page = 4 KiB
        &kernel_base_address // in out
    );
    kernel_file->Read(
        kernel_file,
        &kernel_file_size, // in out
        (VOID *)kernel_base_address // out
    );
    Print(L"Kernel: 0x%0lx (%lu bytes)\n", kernel_base_address, kernel_file_size);

    // want to print kernel entry point, so find it here
    UINT64 entry_addr = *(UINT64 *)(kernel_base_address + 24);
    Print(L"Kernel entry point: 0x%0lx\n", entry_addr);

    // exit UEFI boot service
    EFI_STATUS status;
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
        Print(L"Failed to exit boot service (%r). Retry.\n", status);
        
        status = GetMemoryMap(&memmap);
        if (EFI_ERROR(status)) {
            Print(L"Failed to get memory map: %r\n", status);
            while (1);
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if (EFI_ERROR(status)) {
            Print(L"Failed to exit boot service. Critical.\n");
            while (1);
        }
    }

    // start kernel
    typedef void (*EntryPointType)(void);
    EntryPointType entry_point = (EntryPointType)entry_addr;
    (*entry_point)();

    while (1);
    return EFI_SUCCESS;
}
