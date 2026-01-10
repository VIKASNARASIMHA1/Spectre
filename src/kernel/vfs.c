#include "kernel.h"

VFS* vfs_create() {
    VFS* vfs = (VFS*)malloc(sizeof(VFS));
    if (!vfs) return NULL;
    
    memset(vfs, 0, sizeof(VFS));
    strcpy(vfs->current_dir, "/");
    vfs->file_count = 0;
    
    return vfs;
}

void vfs_destroy(VFS* vfs) {
    if (vfs) {
        for (int i = 0; i < vfs->file_count; i++) {
            if (vfs->files[i].data) {
                free(vfs->files[i].data);
            }
        }
        free(vfs);
    }
}

int vfs_create_file(VFS* vfs, const char* name, size_t size) {
    if (vfs->file_count >= MAX_FILES) {
        ERROR("Too many files\n");
        return -1;
    }
    
    // Check if file already exists
    for (int i = 0; i < vfs->file_count; i++) {
        if (strcmp(vfs->files[i].name, name) == 0) {
            ERROR("File already exists: %s\n", name);
            return -1;
        }
    }
    
    VFile* file = &vfs->files[vfs->file_count];
    strncpy(file->name, name, MAX_NAME_LEN - 1);
    file->name[MAX_NAME_LEN - 1] = '\0';
    
    file->capacity = size + 1024;  // Extra capacity
    file->data = (uint8_t*)malloc(file->capacity);
    if (!file->data) {
        ERROR("Failed to allocate file data\n");
        return -1;
    }
    
    memset(file->data, 0, file->capacity);
    file->size = size;
    file->timestamp = get_time_ms();
    file->is_open = false;
    
    vfs->file_count++;
    INFO("Created file: %s (size: %zu)\n", name, size);
    return vfs->file_count - 1;
}

int vfs_open_file(VFS* vfs, const char* name) {
    for (int i = 0; i < vfs->file_count; i++) {
        if (strcmp(vfs->files[i].name, name) == 0) {
            vfs->files[i].is_open = true;
            vfs->files[i].timestamp = get_time_ms();
            return i;
        }
    }
    ERROR("File not found: %s\n", name);
    return -1;
}

int vfs_read_file(VFS* vfs, int fd, void* buffer, size_t size) {
    if (fd < 0 || fd >= vfs->file_count) {
        ERROR("Invalid file descriptor\n");
        return -1;
    }
    
    VFile* file = &vfs->files[fd];
    if (!file->is_open) {
        ERROR("File not open\n");
        return -1;
    }
    
    size_t to_read = size;
    if (to_read > file->size) {
        to_read = file->size;
    }
    
    memcpy(buffer, file->data, to_read);
    file->timestamp = get_time_ms();
    
    return to_read;
}

int vfs_write_file(VFS* vfs, int fd, void* data, size_t size) {
    if (fd < 0 || fd >= vfs->file_count) {
        ERROR("Invalid file descriptor\n");
        return -1;
    }
    
    VFile* file = &vfs->files[fd];
    if (!file->is_open) {
        ERROR("File not open\n");
        return -1;
    }
    
    // Resize if needed
    if (file->size + size > file->capacity) {
        size_t new_capacity = file->capacity * 2;
        if (new_capacity < file->size + size) {
            new_capacity = file->size + size + 1024;
        }
        
        uint8_t* new_data = (uint8_t*)realloc(file->data, new_capacity);
        if (!new_data) {
            ERROR("Failed to resize file\n");
            return -1;
        }
        
        file->data = new_data;
        file->capacity = new_capacity;
    }
    
    memcpy(file->data + file->size, data, size);
    file->size += size;
    file->timestamp = get_time_ms();
    
    return size;
}

void vfs_list_files(VFS* vfs) {
    printf("\n=== Virtual Filesystem ===\n");
    printf("Current directory: %s\n", vfs->current_dir);
    printf("Files (%d):\n", vfs->file_count);
    
    for (int i = 0; i < vfs->file_count; i++) {
        VFile* file = &vfs->files[i];
        printf("  %s [%s] Size: %zu, Modified: %lu\n",
               file->name,
               file->is_open ? "open" : "closed",
               file->size,
               file->timestamp);
    }
}