#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>

#include "frame_buffer_config.hpp"
#include "elf.hpp"

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

void CalcLoadAddressRange(Elf64_Ehdr *ehdr, UINT64 *first, UINT64 *last) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);
    *first = MAX_UINT64;
    *last = 0;

    // iterate over program headers
    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD) continue;

        *first = MIN(*first, phdr[i].p_vaddr);
        *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
    }
}

void CopyLoadSegments(Elf64_Ehdr *ehdr) {
    Elf64_Phdr *phdr = (Elf64_Phdr *)((UINT64)ehdr + ehdr->e_phoff);

    for (Elf64_Half i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD) continue;

        UINT64 seg_in_file = (UINT64)ehdr + phdr[i].p_offset;
        CopyMem((VOID *)phdr[i].p_vaddr, (VOID *)seg_in_file, phdr[i].p_filesz);

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem((VOID *)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}

void Halt(void) {
    while (1)
        __asm__("hlt");
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
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    status = gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if (EFI_ERROR(status)) {
        return status;
    }

    status = gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if (EFI_ERROR(status)) {
        return status;
    }

    return fs->OpenVolume(fs, root_dir);
}

EFI_STATUS SaveMemoryMap(struct MemoryMap *memmap, EFI_FILE_PROTOCOL *file) {
    EFI_STATUS status;
    CHAR8 buf[256];
    UINTN len;

    CHAR8 *header = "Index, Type, Type (name), PhysicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    status = file->Write(file, &len, header);
    if (EFI_ERROR(status)) {
        return status;
    }

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
        status = file->Write(file, &len, buf);
        if (EFI_ERROR(status)) {
            return status;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
    EFI_STATUS status;
    UINTN num_gop_handles = 0;
    EFI_HANDLE *gop_handles = NULL;

    status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles
    );
    if (EFI_ERROR(status)) {
        return status;
    }

    status = gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID **)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
    );
    if (EFI_ERROR(status)) {
        return status;
    }

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table
) {
    EFI_STATUS status;

    Print(L"Hello, Ponkan World!\n");

    // init memory map buffer
    CHAR8 memmap_buf[4096 * 4]; // page size: 4
    struct MemoryMap memmap = { sizeof(memmap_buf), memmap_buf, 0, 0, 0, 0 };
    status = GetMemoryMap(&memmap);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get memory map: %r\n", status);
        Halt();
    }

    // open memory map file
    EFI_FILE_PROTOCOL *root_dir;
    status = OpenRootDir(image_handle, &root_dir);
    if (EFI_ERROR(status)) {
        Print(L"Failed to open root dir: %r\n", status);
        Halt();
    }

    EFI_FILE_PROTOCOL *memmap_file;
    status = root_dir->Open(
        root_dir,
        &memmap_file,
        L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to open file '\\memmap': %r\n", status);
        Print(L"Skipped.\n");
    } else {
        status = SaveMemoryMap(&memmap, memmap_file);
        if (EFI_ERROR(status)) {
            Print(L"Failed to save memory map: %r\n", status);
            Halt();
        }

        status = memmap_file->Close(memmap_file);
        if (EFI_ERROR(status)) {
            Print(L"Failed to close memory map: %r\n", status);
            Halt();
        }
    }

    // try graphics
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = OpenGOP(image_handle, &gop);
    if (EFI_ERROR(status)) {
        Print(L"Failed to open graphics output protocol (GOP): %r\n", status);
        Halt();
    }

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
    status = root_dir->Open(
        root_dir,
        &kernel_file,
        L"\\kernel.elf",
        EFI_FILE_MODE_READ,
        0
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to open file '\\kernel.elf': %r\n", status);
        Halt();
    }

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16)*12;
    UINT8 file_info_buf[file_info_size];
    status = kernel_file->GetInfo(
        kernel_file,
        &gEfiFileInfoGuid,
        &file_info_size, // in out
        file_info_buf // out
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to get file information: %r\n", status);
        Halt();
    }

    EFI_FILE_INFO *file_info = (EFI_FILE_INFO *)file_info_buf;
    UINTN kernel_file_size = file_info->FileSize;

    // 1. load kernel to temp buffer
    VOID *kernel_buffer;
    status = gBS->AllocatePool(
        EfiLoaderData,
        kernel_file_size,
        &kernel_buffer
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate pool: %r\n", status);
        Halt();
    }

    status = kernel_file->Read(
        kernel_file,
        &kernel_file_size, // in out
        kernel_buffer // out
    );
    if (EFI_ERROR(status)) {
        Print(L"File read error ('\\kernel.elf'): %r\n", status);
        Halt();
    }
    Print(L"Kernel buffer: 0x%0lx (%lu bytes)\n", kernel_buffer, kernel_file_size);

    // 2. read kernel ELF header, calculate destination addr
    Elf64_Ehdr *kernel_ehdr = (Elf64_Ehdr *)kernel_buffer;
    UINT64 kernel_first_addr, kernel_last_addr;
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);
    Print(L"Kernel: 0x%0lx - 0x%0lx\n", kernel_first_addr, kernel_last_addr);

    // 3. allocate pages, finalize kernel
    UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
    status = gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        num_pages,
        &kernel_first_addr // in out
    );
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate kernel pages: %r\n", status);
        Halt();
    }

    CopyLoadSegments(kernel_ehdr);

    // free up kernel file buffer
    status = gBS->FreePool(kernel_buffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to free pool: %r\n", status);
        Halt();
    }

    // want to print kernel entry point, so find it here
    UINT64 entry_addr = *(UINT64 *)(kernel_first_addr + 24);
    Print(L"Kernel entry point: 0x%0lx\n", entry_addr);

    // exit UEFI boot service
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if (EFI_ERROR(status)) {
        Print(L"Failed to exit boot service (%r). Retry.\n", status);
        
        status = GetMemoryMap(&memmap);
        if (EFI_ERROR(status)) {
            Print(L"Failed to get memory map: %r\n", status);
            Halt();
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if (EFI_ERROR(status)) {
            Print(L"Failed to exit boot service. Critical.\n");
            Halt();
        }
    }

    // prepare frame buffer config
    struct FrameBufferConfig config = {
        (UINT8 *)gop->Mode->FrameBufferBase,
        gop->Mode->Info->PixelsPerScanLine,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        0
    };

    switch (gop->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            config.pixel_format = kPixelRGBResv8BitPerColor;
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            config.pixel_format = kPixelBGRResv8BitPerColor;
            break;
        default:
            Print(L"Unsupported pixel format: %d\n", gop->Mode->Info->PixelFormat);
            Halt();
    }

    // start kernel
    typedef void (*EntryPointType)(const struct FrameBufferConfig *);
    EntryPointType entry_point = (EntryPointType)entry_addr;
    (*entry_point)(&config);

    Print(L"All done...?\n");
    return EFI_SUCCESS;
}
