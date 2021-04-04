#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>

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

    Print(L"All done...?\n");

    while (1);
    return EFI_SUCCESS;
}
