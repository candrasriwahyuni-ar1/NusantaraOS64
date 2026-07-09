/**
 * @file vfs.c
 * @brief Virtual File System - Lapisan Abstraksi File System
 * @author NusantaraOS64 Team
 * 
 * Fase 8: Storage, VFS & File System (55% -> 65%)
 * 
 * Implementasi VFS menyediakan API standar: open, read, write, close, stat
 * Mendukung multiple file system backends (initramfs, FAT32-ready)
 */

#include "../include/nusantara.h"

#define MAX_OPEN_FILES 256
#define MAX_MOUNT_POINTS 4

// File descriptor table global
static vfs_file_t open_files[MAX_OPEN_FILES];
static mount_point_t mount_points[MAX_MOUNT_POINTS];
static int next_fd = 3; // 0,1,2 reserved untuk stdin/stdout/stderr

// Forward declarations
static int initramfs_open(mount_point_t* mp, const char* path);
static int initramfs_read(vfs_file_t* file, void* buf, int size);
static int initramfs_write(vfs_file_t* file, const void* buf, int size);
static int initramfs_close(vfs_file_t* file);
static int initramfs_stat(mount_point_t* mp, const char* path, stat_t* st);

// File system operations untuk initramfs
static fs_ops_t initramfs_ops = {
    .open = initramfs_open,
    .read = initramfs_read,
    .write = initramfs_write,
    .close = initramfs_close,
    .stat = initramfs_stat,
    .readdir = NULL,
    .mkdir = NULL,
    .unlink = NULL
};

// ============================================================================
// INITRAMFS IMPLEMENTATION
// ============================================================================

// Struktur file dalam initramfs
typedef struct {
    char name[64];
    uint32_t type;      // FILE_TYPE_FILE atau FILE_TYPE_DIR
    uint32_t size;
    uint8_t* data;      // Pointer ke data (NULL untuk directory)
    uint32_t parent;    // Index parent directory
} initramfs_entry_t;

// Initramfs sederhana dengan beberapa file built-in
static initramfs_entry_t initramfs_entries[] = {
    {"/", FILE_TYPE_DIR, 0, NULL, 0},
    {"/bin", FILE_TYPE_DIR, 0, NULL, 0},
    {"/etc", FILE_TYPE_DIR, 0, NULL, 0},
    {"/home", FILE_TYPE_DIR, 0, NULL, 0},
    {"/bin/sh", FILE_TYPE_FILE, 0, NULL, 0},  // Placeholder shell
    {"/etc/passwd", FILE_TYPE_FILE, 48, (uint8_t*)"root:x:0:0:root:/root:/bin/sh\n", 2},
    {"/etc/hostname", FILE_TYPE_FILE, 14, (uint8_t*)"nusantara-os\n", 0},
    {"/home/welcome.txt", FILE_TYPE_FILE, 58, (uint8_t*)"Selamat datang di NusantaraOS64!\nSistem siap digunakan.\n", 0},
    {"/README", FILE_TYPE_FILE, 79, (uint8_t*)"NusantaraOS64 v0.8 - Sistem Operasi 64-bit Buatan Indonesia\nFase 8: VFS aktif!\n", 0},
};

#define INITRAMFS_ENTRY_COUNT (sizeof(initramfs_entries) / sizeof(initramfs_entry_t))

// Cari entry berdasarkan path
static int initramfs_find_path(const char* path) {
    if (!path || !path[0]) return -1;
    
    for (uint32_t i = 0; i < INITRAMFS_ENTRY_COUNT; i++) {
        if (strcmp(initramfs_entries[i].name, path) == 0) {
            return (int)i;
        }
    }
    return -1;  // Not found
}

static int initramfs_open(mount_point_t* mp, const char* path) {
    (void)mp;
    
    if (!path) return -ERR_INVALID;
    
    int idx = initramfs_find_path(path);
    if (idx < 0) return -ERR_NOTFOUND;
    
    // Buka entry
    initramfs_entry_t* entry = &initramfs_entries[idx];
    
    // Buat file descriptor baru
    if (next_fd >= MAX_OPEN_FILES) return -ERR_NOMEM;
    
    vfs_file_t* file = &open_files[next_fd];
    file->fd = next_fd;
    file->type = entry->type;
    file->size = entry->size;
    file->pos = 0;
    file->data = entry->data;
    file->fs_specific = (void*)(uintptr_t)idx;  // Simpan index sebagai pointer
    file->mount = mp;
    file->is_open = 1;
    
    return next_fd++;
}

static int initramfs_read(vfs_file_t* file, void* buf, int size) {
    if (!file || !buf || !file->is_open) return -ERR_INVALID;
    if (file->type != FILE_TYPE_FILE) return -ERR_INVALID;
    
    int remaining = file->size - file->pos;
    if (remaining <= 0) return 0;  // EOF
    
    int to_read = (size < remaining) ? size : remaining;
    
    // Copy data dari initramfs ke buffer
    memcpy(buf, file->data + file->pos, to_read);
    file->pos += to_read;
    
    return to_read;
}

static int initramfs_write(vfs_file_t* file, const void* buf, int size) {
    (void)file;
    (void)buf;
    (void)size;
    
    // Initramfs adalah read-only
    return -ERR_READONLY;
}

static int initramfs_close(vfs_file_t* file) {
    if (!file || !file->is_open) return -ERR_INVALID;
    
    file->is_open = 0;
    file->fd = -1;
    
    // Bisa deallocate jika perlu
    
    return ERR_OK;
}

static int initramfs_stat(mount_point_t* mp, const char* path, stat_t* st) {
    (void)mp;
    
    if (!path || !st) return -ERR_INVALID;
    
    int idx = initramfs_find_path(path);
    if (idx < 0) return -ERR_NOTFOUND;
    
    initramfs_entry_t* entry = &initramfs_entries[idx];
    
    memset(st, 0, sizeof(stat_t));
    st->type = entry->type;
    st->size = entry->size;
    strncpy(st->name, entry->name, sizeof(st->name) - 1);
    
    return ERR_OK;
}

// ============================================================================
// VFS CORE FUNCTIONS
// ============================================================================

void vfs_init(void) {
    console_printf("[VFS] Initializing Virtual File System...\n");
    
    // Reset file descriptor table
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        open_files[i].fd = -1;
        open_files[i].is_open = 0;
    }
    
    // Reset mount points
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        mount_points[i].mounted = 0;
    }
    
    // Mount initramfs di root
    mount_point_t* root_mp = &mount_points[0];
    root_mp->mounted = 1;
    strcpy(root_mp->path, "/");
    root_mp->fs_type = FS_INITRAMFS;
    root_mp->ops = &initramfs_ops;
    root_mp->root = NULL;
    
    console_printf("[VFS] Mounted initramfs at /\n");
    
    // Setup stdin/stdout/stderr
    for (int i = 0; i < 3; i++) {
        open_files[i].fd = i;
        open_files[i].is_open = 1;
        open_files[i].type = FILE_TYPE_CHAR;
        open_files[i].size = 0;
        open_files[i].pos = 0;
        open_files[i].data = NULL;
    }
    
    console_printf("[VFS] VFS initialized with %d entries in initramfs\n", 
            INITRAMFS_ENTRY_COUNT);
}

// Cari mount point untuk path tertentu
static mount_point_t* vfs_find_mount(const char* path) {
    if (!path || !path[0]) return NULL;
    
    // Cari mount point terpanjang yang match
    mount_point_t* best_match = NULL;
    int best_len = 0;
    
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (!mount_points[i].mounted) continue;
        
        int mount_len = strlen(mount_points[i].path);
        if (mount_len > best_len && strncmp(path, mount_points[i].path, mount_len) == 0) {
            // Pastikan ini adalah prefix yang valid (akhir string atau /)
            if (path[mount_len] == '\0' || path[mount_len] == '/') {
                best_match = &mount_points[i];
                best_len = mount_len;
            }
        }
    }
    
    return best_match;
}

int vfs_open(const char* path, int flags) {
    (void)flags;  // TODO: Support flags (O_RDONLY, O_WRONLY, dll)
    
    if (!path) return -ERR_INVALID;
    
    // Cari mount point
    mount_point_t* mp = vfs_find_mount(path);
    if (!mp) {
        console_printf("[VFS] No mount point for: %s\n", path);
        return -ERR_NOTFOUND;
    }
    
    // Hitung path relatif terhadap mount point
    const char* rel_path = path + strlen(mp->path);
    if (rel_path[0] == '/') rel_path++;
    if (rel_path[0] == '\0') rel_path = "/";
    
    // Panggil filesystem backend
    if (mp->ops && mp->ops->open) {
        return mp->ops->open(mp, rel_path);
    }
    
    return -ERR_NOTSUPP;
}

int vfs_read(int fd, void* buf, int size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -ERR_INVALID;
    
    vfs_file_t* file = &open_files[fd];
    if (!file->is_open) return -ERR_INVALID;
    
    // Handle stdin khusus
    if (fd == 0) {
        // Baca dari keyboard buffer
        return keyboard_read((char*)buf, size);
    }
    
    // Handle stdout/stderr
    if (fd == 1 || fd == 2) {
        // Tidak bisa read dari stdout/stderr
        return -ERR_INVALID;
    }
    
    // Panggil filesystem backend
    if (file->mount && file->mount->ops && file->mount->ops->read) {
        return file->mount->ops->read(file, buf, size);
    }
    
    return -ERR_NOTSUPP;
}

int vfs_write(int fd, const void* buf, int size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -ERR_INVALID;
    
    vfs_file_t* file = &open_files[fd];
    
    // Handle stdout/stderr
    if (fd == 1 || fd == 2) {
        // Tulis ke console
        const char* str = (const char*)buf;
        for (int i = 0; i < size; i++) {
            console_putchar(str[i]);
        }
        return size;
    }
    
    // Handle stdin
    if (fd == 0) {
        return -ERR_INVALID;
    }
    
    if (!file->is_open) return -ERR_INVALID;
    
    // Panggil filesystem backend
    if (file->mount && file->mount->ops && file->mount->ops->write) {
        return file->mount->ops->write(file, buf, size);
    }
    
    return -ERR_NOTSUPP;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -ERR_INVALID;
    if (fd < 3) return ERR_OK;  // Jangan tutup stdin/stdout/stderr
    
    vfs_file_t* file = &open_files[fd];
    if (!file->is_open) return -ERR_INVALID;
    
    int ret = ERR_OK;
    
    // Panggil filesystem backend
    if (file->mount && file->mount->ops && file->mount->ops->close) {
        ret = file->mount->ops->close(file);
    }
    
    // Reset file descriptor
    file->fd = -1;
    file->is_open = 0;
    
    return ret;
}

int vfs_stat(const char* path, stat_t* st) {
    if (!path || !st) return -ERR_INVALID;
    
    // Cari mount point
    mount_point_t* mp = vfs_find_mount(path);
    if (!mp) return -ERR_NOTFOUND;
    
    // Hitung path relatif
    const char* rel_path = path + strlen(mp->path);
    if (rel_path[0] == '/') rel_path++;
    if (rel_path[0] == '\0') rel_path = "/";
    
    // Panggil filesystem backend
    if (mp->ops && mp->ops->stat) {
        return mp->ops->stat(mp, rel_path, st);
    }
    
    return -ERR_NOTSUPP;
}

int vfs_readdir(const char* path, dirent_t* dirp) {
    if (!path || !dirp) return -ERR_INVALID;
    
    mount_point_t* mp = vfs_find_mount(path);
    if (!mp) return -ERR_NOTFOUND;
    
    if (mp->ops && mp->ops->readdir) {
        return mp->ops->readdir(mp, path, dirp);
    }
    
    // Fallback: list semua entry di initramfs yang parent-nya path ini
    if (mp->fs_type == FS_INITRAMFS) {
        int parent_idx = initramfs_find_path(path);
        if (parent_idx < 0) return -ERR_NOTFOUND;
        
        static int readdir_index = 0;
        
        if (dirp->offset == 0) {
            readdir_index = 0;
        }
        
        // Cari entry berikutnya dengan parent yang sesuai
        while (readdir_index < (int)INITRAMFS_ENTRY_COUNT) {
            initramfs_entry_t* entry = &initramfs_entries[readdir_index];
            if (entry->parent == (uint32_t)parent_idx) {
                strncpy(dirp->name, entry->name, sizeof(dirp->name) - 1);
                dirp->type = entry->type;
                dirp->offset = ++readdir_index;
                readdir_index++;
                return ERR_OK;
            }
            readdir_index++;
        }
        
        return -ERR_NOTFOUND;  // End of directory
    }
    
    return -ERR_NOTSUPP;
}

// Mount filesystem baru
int vfs_mount(const char* device, const char* path, fs_type_t fs_type) {
    if (!device || !path) return -ERR_INVALID;
    
    // Cari slot mount point kosong
    mount_point_t* mp = NULL;
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (!mount_points[i].mounted) {
            mp = &mount_points[i];
            break;
        }
    }
    
    if (!mp) return -ERR_NOMEM;
    
    // Setup mount point
    mp->mounted = 1;
    strncpy(mp->path, path, sizeof(mp->path) - 1);
    mp->fs_type = fs_type;
    mp->device = device;
    
    // Set ops berdasarkan filesystem type
    switch (fs_type) {
        case FS_INITRAMFS:
            mp->ops = &initramfs_ops;
            break;
        case FS_FAT32:
            // TODO: Implementasi FAT32
            console_printf("[VFS] FAT32 support not yet implemented\n");
            return -ERR_NOTSUPP;
        default:
            return -ERR_INVALID;
    }
    
    console_printf("[VFS] Mounted %s at %s (type: %d)\n", device, path, fs_type);
    
    return ERR_OK;
}

// Unmount filesystem
int vfs_unmount(const char* path) {
    if (!path) return -ERR_INVALID;
    
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].mounted && strcmp(mount_points[i].path, path) == 0) {
            // TODO: Check apakah ada file yang masih terbuka
            mount_points[i].mounted = 0;
            console_printf("[VFS] Unmounted %s\n", path);
            return ERR_OK;
        }
    }
    
    return -ERR_NOTFOUND;
}

// List semua mount points
void vfs_list_mounts(void) {
    console_printf("\n=== MOUNTED FILESYSTEMS ===\n");
    console_printf("%-20s %-10s %s\n", "PATH", "TYPE", "DEVICE");
    console_printf("----------------------------------------\n");
    
    for (int i = 0; i < MAX_MOUNT_POINTS; i++) {
        if (mount_points[i].mounted) {
            const char* type_str = "UNKNOWN";
            switch (mount_points[i].fs_type) {
                case FS_INITRAMFS: type_str = "initramfs"; break;
                case FS_FAT32: type_str = "FAT32"; break;
            }
            console_printf("%-20s %-10s %s\n", 
                    mount_points[i].path, 
                    type_str,
                    mount_points[i].device ? mount_points[i].device : "(none)");
        }
    }
    console_printf("=========================\n\n");
}

// Test fungsi VFS
void vfs_test(void) {
    console_printf("\n=== VFS TEST ===\n");
    
    // Test 1: List mount points
    vfs_list_mounts();
    
    // Test 2: Stat file
    stat_t st;
    const char* test_files[] = {
        "/README",
        "/etc/hostname",
        "/etc/passwd",
        "/home/welcome.txt",
        "/bin",
        "/nonexistent"
    };
    
    for (int i = 0; i < 6; i++) {
        int ret = vfs_stat(test_files[i], &st);
        if (ret == ERR_OK) {
            console_printf("  [OK] %s: type=%d, size=%d\n", 
                    st.name, st.type, st.size);
        } else {
            console_printf("  [FAIL] %s: error=%d\n", test_files[i], ret);
        }
    }
    
    // Test 3: Read file
    console_printf("\n  Reading /README:\n");
    int fd = vfs_open("/README", 0);
    if (fd >= 0) {
        char buf[256];
        int n = vfs_read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            console_printf("    %s\n", buf);
        }
        vfs_close(fd);
    } else {
        console_printf("    Failed to open: %d\n", fd);
    }
    
    // Test 4: Read directory
    console_printf("\n  Listing /etc:\n");
    dirent_t de;
    de.offset = 0;
    while (vfs_readdir("/etc", &de) == ERR_OK) {
        console_printf("    %s (%s)\n", de.name, de.type == FILE_TYPE_DIR ? "dir" : "file");
    }
    
    console_printf("================\n\n");
}
