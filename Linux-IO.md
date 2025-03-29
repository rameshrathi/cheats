# POSIX File I/O and File Handling APIs
This document details various POSIX APIs used for interacting with files and the filesystem. POSIX (Portable Operating System Interface) defines a standard interface between application programs and operating systems, ensuring a degree of portability across compliant systems (like Linux, macOS, BSD variants, etc.).

1. Basic File Operations
These are the core functions for opening, closing, reading, writing, and positioning within files.

### open() / openat()
(Requires <sys/types.h>, <sys/stat.h>, <fcntl.h>)

Synopsis: Opens or optionally creates a file. Returns a non-negative integer called a file descriptor on success, which is used by subsequent I/O calls. Returns -1 on error. openat() is similar but operates relative to a directory file descriptor (dirfd), enhancing security and avoiding race conditions in certain scenarios.

Key Arguments:

pathname: The path to the file.

flags: Specifies the access mode (O_RDONLY, O_WRONLY, O_RDWR) and various options (e.g., O_CREAT, O_EXCL, O_TRUNC, O_APPEND, O_NONBLOCK, O_SYNC, O_DSYNC, O_CLOEXEC). Flags are combined using bitwise OR (|).

mode (optional, required if O_CREAT is specified): Sets the file permission bits (e.g., 0644) for a newly created file, modified by the process's umask.

Use Cases:

Gaining access to an existing file for reading or writing.

Creating a new file.

Atomically creating a file only if it doesn't exist (O_CREAT | O_EXCL).

Truncating a file to zero length upon opening (O_TRUNC).

Ensuring writes append to the end of the file (O_APPEND).

Opening for non-blocking I/O (O_NONBLOCK).

Opening for synchronous I/O (O_SYNC, O_DSYNC).

Using openat() for operations within a specific directory, especially useful in multi-threaded programs or when dealing with changing working directories.

// Example: Open for reading, create if not exists, truncate if exists
int fd = open("myfile.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
if (fd == -1) {
    perror("open failed");
    // Handle error
}

// Example: Open relative to a directory fd
int dir_fd = open("/some/directory", O_RDONLY | O_DIRECTORY);
// ... error check dir_fd ...
int file_fd = openat(dir_fd, "relative_file.txt", O_WRONLY | O_CREAT, 0644);
// ... error check file_fd ...
close(dir_fd); // Close the directory fd when done
Use code with caution.
C
### creat()
(Requires <sys/types.h>, <sys/stat.h>, <fcntl.h>)

Synopsis: Historically used for creating files. creat(path, mode) is equivalent to open(path, O_WRONLY | O_CREAT | O_TRUNC, mode).

Use Cases:

Largely deprecated in favor of open() with the O_CREAT flag, as open() offers more flexibility (e.g., specifying read/write access, append mode, exclusive creation). Included for historical context and compatibility.

### close()
(Requires <unistd.h>)

Synopsis: Closes a file descriptor, releasing resources associated with it. Once closed, the file descriptor is no longer valid and may be reused by the system for subsequent open() calls.

Use Cases:

Releasing file resources (locks, memory) after finishing operations.

Ensuring buffered data is flushed (though fsync() provides stronger guarantees).

Preventing resource exhaustion (processes have a limit on open file descriptors).

Crucial for correctness; forgetting to close files can lead to data loss or subtle bugs.

Note: close() can fail (e.g., EINTR, I/O errors during final flush). Production code should check the return value.

if (close(fd) == -1) {
    perror("close failed");
    // Handle error (e.g., log it, retry if EINTR)
}
Use code with caution.
C
### read()
(Requires <unistd.h>)

Synopsis: Attempts to read up to count bytes from the file associated with the file descriptor fd into the buffer pointed to by buf. The file offset is advanced by the number of bytes read.

Return Value:

Number of bytes read (can be less than count, known as a "short read").

0 indicates end-of-file (EOF).

-1 indicates an error (check errno).

Use Cases:

Reading data sequentially from files, pipes, sockets, terminals, etc.

Reading data in chunks. Applications often need a loop to read the desired amount due to short reads.

char buffer[1024];
ssize_t bytes_read;

while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
    // Process the 'bytes_read' bytes in 'buffer'
}
if (bytes_read == -1) {
    perror("read failed");
    // Handle error
}
// bytes_read == 0 means EOF reached
Use code with caution.
C
### write()
(Requires <unistd.h>)

Synopsis: Attempts to write up to count bytes from the buffer pointed to by buf to the file associated with the file descriptor fd. The file offset is advanced by the number of bytes written.

Return Value:

Number of bytes written (can be less than count, a "short write").

-1 indicates an error (check errno).

Use Cases:

Writing data sequentially to files, pipes, sockets, terminals, etc.

Writing data in chunks. Applications often need a loop to write the desired amount due to short writes.

const char *message = "Hello, POSIX!";
size_t len = strlen(message);
ssize_t bytes_written;
size_t total_written = 0;

while (total_written < len) {
    bytes_written = write(fd, message + total_written, len - total_written);
    if (bytes_written == -1) {
        if (errno == EINTR) continue; // Interrupted, retry
        perror("write failed");
        // Handle error (break or return)
        break;
    }
    total_written += bytes_written;
}
Use code with caution.
C
### lseek()
(Requires <sys/types.h>, <unistd.h>)

Synopsis: Repositions the file offset (the read/write pointer) for the open file descriptor fd.

Arguments:

fd: The file descriptor.

offset: The offset value.

whence: Determines how offset is interpreted:

SEEK_SET: Offset is set to offset bytes from the beginning.

SEEK_CUR: Offset is set to its current location plus offset bytes.

SEEK_END: Offset is set to the size of the file plus offset bytes.

Return Value: The resulting offset location measured in bytes from the beginning of the file, or -1 on error.

Use Cases:

Implementing random access within regular files.

Seeking to the beginning or end of a file.

Determining the current file offset (lseek(fd, 0, SEEK_CUR)).

Determining file size (lseek(fd, 0, SEEK_END)), though fstat() is generally preferred.

Creating "holes" in files by seeking past the end and writing.

Limitations: Cannot be used on non-seekable file descriptors like pipes, FIFOs, sockets, or terminals.

// Seek to byte 100 from the beginning
off_t new_offset = lseek(fd, 100, SEEK_SET);
if (new_offset == (off_t)-1) {
    perror("lseek SEEK_SET failed");
}

// Seek 50 bytes forward from current position
new_offset = lseek(fd, 50, SEEK_CUR);
if (new_offset == (off_t)-1) {
    perror("lseek SEEK_CUR failed");
}

// Seek to the end of the file
off_t file_size = lseek(fd, 0, SEEK_END);
if (file_size == (off_t)-1) {
    perror("lseek SEEK_END failed");
}
Use code with caution.
C
### pread() / pwrite()
(Requires <unistd.h>)

Synopsis: Read or write data at a specific offset within the file without changing the file descriptor's current file offset. These operations are atomic with respect to the positioning and I/O.

Use Cases:

Reading/writing to specific locations in a file in multi-threaded applications without needing external locks to protect the file offset managed by lseek/read/write. Each thread can operate on different parts of the file concurrently using pread/pwrite.

Simplifying code that needs to read/write at a specific location and then restore the previous offset.

char buffer[512];
off_t position = 4096;
ssize_t bytes_read = pread(fd, buffer, sizeof(buffer), position);
// The file descriptor's main offset remains unchanged by this call.

const char *data = "Specific data";
ssize_t bytes_written = pwrite(fd, data, strlen(data), position);
// The file descriptor's main offset remains unchanged.
Use code with caution.
C
2. File Metadata and Attributes
These functions allow querying and modifying file properties like permissions, ownership, size, and timestamps.

### stat() / fstat() / lstat() / statx()
(Requires <sys/types.h>, <sys/stat.h>, <unistd.h>; statx also needs <fcntl.h>)

Synopsis: Retrieve detailed information about a file.

stat(pathname, &statbuf): Gets info about the file specified by pathname. If pathname is a symbolic link, it follows the link and returns info about the target file.

fstat(fd, &statbuf): Gets info about the file referenced by the open file descriptor fd.

lstat(pathname, &statbuf): Like stat(), but if pathname is a symbolic link, it returns info about the link itself, not the target.

statx(dirfd, pathname, flags, mask, &statxbuf): A modern, more extensible interface providing finer control over which attributes are fetched and how symlinks are handled (via flags), relative path resolution (dirfd), and potentially more information.

Output: Fills a struct stat (or struct statx) containing fields like:

st_mode: File type (regular, directory, symlink, etc.) and permissions.

st_ino: Inode number.

st_dev: Device ID containing the file.

st_nlink: Number of hard links.

st_uid: User ID of owner.

st_gid: Group ID of owner.

st_size: Total size in bytes (for regular files).

st_atime: Time of last access.

st_mtime: Time of last modification.

st_ctime: Time of last status change (metadata change).

st_blksize: Preferred block size for I/O.

st_blocks: Number of 512B blocks allocated.

Use Cases:

Checking if a path exists and determining its type (file, directory, link).

Getting file size before reading.

Checking permissions or ownership.

Comparing files based on modification times.

Using lstat to specifically examine symbolic links.

Using fstat when you already have an open file descriptor.

Using statx for more advanced or performance-sensitive queries.

struct stat sb;
if (stat("/path/to/file", &sb) == -1) {
    perror("stat failed");
} else {
    printf("File size: %lld bytes\n", (long long)sb.st_size);
    if (S_ISDIR(sb.st_mode)) {
        printf("Is a directory\n");
    }
}

// Check a symlink itself
if (lstat("/path/to/symlink", &sb) == -1) { /* ... */ }

// Get info from an open fd
if (fstat(fd, &sb) == -1) { /* ... */ }
Use code with caution.
C
### access() / faccessat()
(Requires <unistd.h>; faccessat also needs <fcntl.h>)

Synopsis: Checks if the calling process has permission to access a file in a specific mode. Uses the real user ID and group ID, not the effective IDs (important for set-UID/set-GID programs). faccessat() is the modern, more flexible version operating relative to dirfd.

Arguments:

pathname: Path to the file.

mode: Specifies the check(s) to perform (bitwise OR):

R_OK: Test for read permission.

W_OK: Test for write permission.

X_OK: Test for execute permission.

F_OK: Test for the existence of the file.

Return Value: 0 if access is permitted, -1 otherwise (check errno).

Use Cases:

Quickly checking if a file exists (F_OK).

Performing preliminary permission checks before attempting an operation like open().

Caution (TOCTOU): There's a potential Time-of-Check to Time-of-Use race condition. The file's status or permissions could change between the access() call and the subsequent operation (e.g., open()). It's often better to just attempt the open() and handle the potential EACCES or ENOENT errors directly. access() is most reliable for checking F_OK or in situations where the environment is tightly controlled.

if (access("config.ini", R_OK | W_OK) == 0) {
    printf("Have read/write access (based on real UID/GID).\n");
} else {
    if (errno == ENOENT) {
        printf("File does not exist.\n");
    } else if (errno == EACCES) {
        printf("Permission denied.\n");
    } else {
        perror("access failed");
    }
}
Use code with caution.
C
### chmod() / fchmod() / fchmodat()
(Requires <sys/types.h>, <sys/stat.h>; fchmodat also needs <fcntl.h>)

Synopsis: Changes the permission bits (mode) of a file.

chmod(pathname, mode): Changes permissions for the file at pathname. Follows symlinks by default.

fchmod(fd, mode): Changes permissions for the open file descriptor fd.

fchmodat(dirfd, pathname, mode, flags): Relative path version. flags can specify AT_SYMLINK_NOFOLLOW to operate on a symlink itself instead of its target.

mode Argument: Typically an octal number (e.g., 0644, 0755) or constructed using constants like S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, etc.

Use Cases:

Setting initial permissions after creating a file.

Restricting or granting access to files/directories.

Making a script executable (chmod +x).

// Set permissions to read/write for owner, read-only for group/others
if (chmod("data.db", 0644) == -1) {
    perror("chmod failed");
}

// Using fchmod on an open file descriptor
if (fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) { // Equivalent to 0644
    perror("fchmod failed");
}
Use code with caution.
C
### chown() / fchown() / lchown() / fchownat()
(Requires <sys/types.h>, <unistd.h>; fchownat also needs <fcntl.h>)

Synopsis: Changes the owner (user ID) and/or group (group ID) of a file. Typically requires superuser privileges unless specific conditions are met (e.g., changing the group to one the user belongs to, depending on OS).

chown(pathname, owner, group): Changes ownership for pathname. Follows symlinks.

fchown(fd, owner, group): Changes ownership for the open file descriptor fd.

lchown(pathname, owner, group): Like chown, but operates on the symbolic link itself, not its target.

fchownat(dirfd, pathname, owner, group, flags): Relative path version. flags can include AT_SYMLINK_NOFOLLOW.

Arguments: owner is a uid_t, group is a gid_t. Specify -1 for either if you don't want to change it.

Use Cases:

Transferring ownership of files (e.g., after creating files on behalf of another user).

Setting appropriate group ownership for shared files.

Using lchown to change the ownership of a symbolic link itself (less common).

uid_t new_owner_uid = 1001;
gid_t new_group_gid = 100;

// Change owner and group of a file
if (chown("user_file.txt", new_owner_uid, new_group_gid) == -1) {
    perror("chown failed"); // Often EPERM if not privileged
}

// Change only the group of a symlink itself
if (lchown("my_link", (uid_t)-1, new_group_gid) == -1) {
    perror("lchown failed");
}
Use code with caution.
C
### utime() / utimes() / futimens() / utimensat()
(Requires <sys/types.h>, <utime.h>; futimens/utimensat require <sys/stat.h>)

Synopsis: Changes the access (atime) and modification (mtime) timestamps of a file. Newer functions (futimens, utimensat) use struct timespec for nanosecond precision and offer more control.

utime(pathname, &times): Uses struct utimbuf (seconds precision). If times is NULL, sets timestamps to the current time.

utimes(pathname, tvp[2]): Uses struct timeval array (microsecond precision). tvp[0] is access time, tvp[1] is modification time. NULL sets current time.

futimens(fd, times[2]): Operates on an open file descriptor fd. Uses struct timespec array (nanosecond precision).

utimensat(dirfd, pathname, times[2], flags): Relative path version with nanosecond precision. flags can include AT_SYMLINK_NOFOLLOW.

Use Cases:

Restoring timestamps after modifying a file (e.g., in backup/archive utilities).

"Touching" a file to update its timestamp (e.g., for build systems like make).

Setting specific historical timestamps.

// Using modern utimensat to set current time, not following symlinks
if (utimensat(AT_FDCWD, "some_file_or_link", NULL, AT_SYMLINK_NOFOLLOW) == -1) {
    perror("utimensat failed");
}

// Using futimens to set specific times on an open fd
struct timespec specific_times[2];
// ... fill specific_times[0] (atime) and specific_times[1] (mtime) ...
if (futimens(fd, specific_times) == -1) {
    perror("futimens failed");
}
Use code with caution.
C
### truncate() / ftruncate()
(Requires <sys/types.h>, <unistd.h>)

Synopsis: Truncates or extends a file to a specified length.

truncate(pathname, length): Operates on the file specified by pathname.

ftruncate(fd, length): Operates on the open file descriptor fd.

Behavior:

If the file was larger than length, the extra data is discarded.

If the file was smaller, it is extended. The extended part reads as zero bytes (creating a "hole" if supported by the filesystem). File metadata (e.g., mtime, ctime) is updated.

Use Cases:

Resetting a log file to zero length without deleting and recreating it.

Pre-allocating space for a file (by extending it).

Clearing the contents of a temporary file.

off_t desired_size = 0; // Truncate to zero length

// Truncate using path
if (truncate("logfile.log", desired_size) == -1) {
    perror("truncate failed");
}

// Truncate using file descriptor
if (ftruncate(fd, desired_size) == -1) {
    perror("ftruncate failed");
}
Use code with caution.
C
3. Directory Operations
Functions for creating, removing, and reading the contents of directories.

### mkdir() / mkdirat()
(Requires <sys/types.h>, <sys/stat.h>; mkdirat also needs <fcntl.h>)

Synopsis: Creates a new directory. mkdirat() operates relative to dirfd.

Arguments:

pathname: The path of the directory to create.

mode: The permission bits for the new directory (e.g., 0755), modified by the process umask.

Use Cases:

Creating directories needed by an application (e.g., for configuration, cache, output).

Building directory structures.

Note: mkdir only creates the final component of the path; parent directories must already exist.

// Create a directory with rwxr-xr-x permissions (modified by umask)
if (mkdir("/var/myapp/cache", 0755) == -1) {
    // Check errno (EEXIST is common and might be okay)
    if (errno != EEXIST) {
        perror("mkdir failed");
    }
}
Use code with caution.
C
### rmdir()
(Requires <unistd.h>)

Synopsis: Removes an empty directory.

Use Cases:

Cleaning up temporary or empty directories.

Note: The directory must be empty (contain only . and ..). Fails with ENOTEMPTY if it contains other files or directories. Use nftw() or recursive manual deletion (using opendir/readdir/unlink/rmdir) to remove non-empty directories.

if (rmdir("/tmp/myapp_tempdir") == -1) {
    perror("rmdir failed");
    // Common errors: ENOENT (doesn't exist), ENOTEMPTY (not empty), EACCES (permission denied)
}
Use code with caution.
C
### opendir()
(Requires <sys/types.h>, <dirent.h>)

Synopsis: Opens a directory stream corresponding to the directory specified by name. Returns a pointer to a DIR structure (a directory stream handle) on success, or NULL on error.

Use Cases:

Beginning the process of listing the contents of a directory. The returned DIR* is used with readdir.

### readdir() / readdir_r()
(Requires <dirent.h>)

Synopsis: Reads the next directory entry from the directory stream dirp opened by opendir().

readdir(): Returns a pointer to a struct dirent representing the next entry, or NULL at the end of the directory or if an error occurs. The returned pointer might point to static storage; subsequent calls may overwrite it. Not guaranteed to be thread-safe in older POSIX versions, but generally safe on modern systems (Linux, BSD, macOS).

readdir_r(): Deprecated. A reentrant version intended for thread safety, but complex to use correctly and less efficient. Modern readdir() is preferred.

struct dirent: Contains at least d_ino (inode number) and d_name (null-terminated filename). Other fields like d_type (file type, e.g., DT_REG, DT_DIR, DT_LNK) may be present but are not guaranteed by base POSIX.

Use Cases:

Iterating through the files and subdirectories within a directory.

Implementing tools like ls or recursive operations.

### closedir()
(Requires <sys/types.h>, <dirent.h>)

Synopsis: Closes the directory stream dirp previously opened by opendir(), releasing associated resources.

Return Value: 0 on success, -1 on error.

Use Cases:

Essential cleanup after finishing reading a directory with opendir/readdir.

#include <stdio.h>
#include <dirent.h>
#include <errno.h>

void list_dir(const char *path) {
    DIR *dirp;
    struct dirent *entry;

    dirp = opendir(path);
    if (dirp == NULL) {
        perror("opendir failed");
        return;
    }

    errno = 0; // To distinguish end-of-stream from error
    while ((entry = readdir(dirp)) != NULL) {
        // Skip "." and ".." entries if desired
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        printf("Found entry: %s", entry->d_name);
        // Check d_type if available and needed
        #ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type == DT_DIR) {
            printf(" (directory)\n");
        } else if (entry->d_type == DT_REG) {
            printf(" (regular file)\n");
        } else {
            printf(" (other type)\n");
        }
        #else
        printf("\n"); // d_type not available, need stat() for type
        #endif
    }

    if (errno != 0 && entry == NULL) {
        perror("readdir failed");
    }

    if (closedir(dirp) == -1) {
        perror("closedir failed");
    }
}
Use code with caution.
C
### rewinddir()
(Requires <sys/types.h>, <dirent.h>)

Synopsis: Resets the position of the directory stream dirp to the beginning. Subsequent readdir() calls will re-read entries from the start.

Use Cases:

Processing directory entries multiple times without closing and reopening the directory stream.

### seekdir() / telldir()
(Requires <sys/types.h>, <dirent.h>)

Synopsis:

telldir(dirp): Returns a value representing the current position within the directory stream dirp.

seekdir(dirp, loc): Sets the position of the directory stream dirp to a location loc previously obtained from telldir().

Use Cases:

Saving and restoring a specific position while iterating through a directory. Less commonly used than rewinddir.

4. Links (Hard and Symbolic)
Functions for creating and managing filesystem links.

### link() / linkat()
(Requires <unistd.h>; linkat also needs <fcntl.h>)

Synopsis: Creates a new hard link newpath pointing to the existing file oldpath. A hard link is another directory entry referring to the same underlying inode (file data and metadata). linkat() provides relative path capabilities and flags.

Behavior:

Increments the link count (st_nlink in struct stat) of the inode.

Hard links cannot span across different filesystems.

Hard links generally cannot be created for directories (to prevent loops).

Use Cases:

Creating multiple names for the same file content within the same filesystem.

Used by some backup systems or file management tools.

The file data is only deleted when the link count drops to zero and no process holds the file open.

if (link("original_file.data", "hardlink_copy.data") == -1) {
    perror("link failed");
    // EEXIST if newpath exists, EXDEV across filesystems, EPERM for directory
}
Use code with caution.
C
### symlink() / symlinkat()
(Requires <unistd.h>; symlinkat also needs <fcntl.h>)

Synopsis: Creates a symbolic link (symlink or soft link) named linkpath that contains the string target. symlinkat() operates relative to dirfd.

Behavior:

A symlink is a special file whose content is the path to another file or directory.

When the OS accesses a symlink (unless specifically told not to, e.g., lstat, readlink), it usually follows the path stored within the link.

Symlinks can span filesystems.

Symlinks can point to directories.

Symlinks can become "dangling" if the target file is moved or deleted.

Use Cases:

Creating shortcuts or aliases for files/directories.

Redirecting paths (e.g., /usr/bin/editor pointing to /usr/bin/vim).

Pointing to files across different partitions/filesystems.

if (symlink("/path/to/real/target", "my_shortcut_link") == -1) {
    perror("symlink failed");
}
Use code with caution.
C
### unlink() / unlinkat()
(Requires <unistd.h>; unlinkat also needs <fcntl.h>)

Synopsis: Removes a name (a directory entry, which could be a hard link or the last link) from the filesystem. unlinkat() operates relative to dirfd and has flags (e.g., AT_REMOVEDIR to behave like rmdir).

Behavior:

Decrements the link count (st_nlink) of the inode the name referred to.

If the link count drops to zero and no process has the file open, the file's data blocks and inode are deallocated (the file is truly deleted).

If the link count drops to zero but processes still have it open, the name is removed, but the data persists until the last process closes its file descriptor. This is useful for temporary files (open, then unlink).

Use Cases:

Deleting files.

Removing one of multiple hard links.

Implementing secure temporary files (create, open, then immediately unlink the name; the file persists via the open fd but has no directory entry).

if (unlink("file_to_delete.txt") == -1) {
    perror("unlink failed");
    // Common errors: ENOENT (doesn't exist), EACCES (permission), EISDIR (can't unlink directory - use rmdir or unlinkat w/ flag)
}
Use code with caution.
C
### readlink() / readlinkat()
(Requires <unistd.h>; readlinkat also needs <fcntl.h>)

Synopsis: Reads the value (the target path) of a symbolic link into a buffer. Does not null-terminate the buffer. readlinkat() operates relative to dirfd.

Return Value: Number of bytes placed in the buffer on success, or -1 on error (EINVAL if not a symlink, ENOENT if link doesn't exist).

Use Cases:

Determining where a symbolic link points.

Resolving paths that might contain symbolic links.

char link_target[PATH_MAX]; // PATH_MAX from <limits.h> or allocate dynamically
ssize_t len = readlink("my_shortcut_link", link_target, sizeof(link_target) - 1);

if (len == -1) {
    perror("readlink failed");
} else {
    link_target[len] = '\0'; // Manually null-terminate
    printf("Symlink points to: %s\n", link_target);
}
Use code with caution.
C
### rename() / renameat()
(Requires <stdio.h>, <unistd.h>; renameat needs <fcntl.h>)

Synopsis: Renames a file, moving it between directories if required. This operation is atomic on POSIX-compliant systems if oldpath and newpath are on the same filesystem. renameat() provides relative path capabilities.

Behavior:

If newpath exists, it is first silently removed (like unlink). Restrictions apply (e.g., cannot replace a non-empty directory with a file).

Can move files across directories within the same filesystem atomically.

Moving across filesystems is often not supported atomically and may fail (EXDEV) or involve a copy-then-delete operation internally.

Use Cases:

Renaming files or directories.

Atomically replacing a file (e.g., write to a temporary file, then rename it over the original). This ensures readers always see either the complete old file or the complete new file.

Moving files within the same filesystem.

// Atomically replace config.ini with a new version
if (rename("config.ini.tmp", "config.ini") == -1) {
    perror("rename failed");
    // Common errors: EACCES, ENOENT, EXDEV (cross-device link)
}
Use code with caution.
C
5. File Descriptor Manipulation
Functions for managing open file descriptors themselves.

### dup() / dup2() / dup3()
(Requires <unistd.h>; dup3 also needs <fcntl.h> and is Linux-specific or requires _GNU_SOURCE)

Synopsis: Duplicate an existing file descriptor.

dup(oldfd): Creates a copy of oldfd, using the lowest-numbered unused file descriptor.

dup2(oldfd, newfd): Makes newfd become a copy of oldfd. If newfd was already open, it is first silently closed. If oldfd equals newfd, it does nothing but return newfd. Atomic.

dup3(oldfd, newfd, flags): Like dup2, but includes a flags argument (e.g., O_CLOEXEC to set the close-on-exec flag atomically).

Behavior: The new and old file descriptors share the same underlying open file description (file offset, file status flags). Changes to the offset or flags via one descriptor affect the other. The close-on-exec flag (FD_CLOEXEC) is not set by dup or dup2 by default, but can be set by dup3.

Use Cases:

Redirecting standard input/output/error (often using dup2 before exec):

int fd = open("output.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
// ... error check fd ...
dup2(fd, STDOUT_FILENO); // Redirect stdout to fd
dup2(fd, STDERR_FILENO); // Redirect stderr to fd
close(fd); // Close original fd, no longer needed
execl("/usr/bin/mycommand", "mycommand", NULL); // Command's output goes to file
Use code with caution.
C
Saving/restoring standard streams.

Setting up pipes between processes.

### fcntl()
(Requires <unistd.h>, <fcntl.h>)

Synopsis: The "file control" function; a versatile tool for manipulating open file descriptors. Performs various operations specified by the cmd argument.

Common Commands (cmd):

F_DUPFD / F_DUPFD_CLOEXEC: Duplicates a file descriptor, similar to dup(), but finds the lowest available descriptor greater than or equal to a specified third argument (arg). _CLOEXEC variant sets the close-on-exec flag atomically.

F_GETFD: Get the file descriptor flags (currently only FD_CLOEXEC).

F_SETFD: Set the file descriptor flags (currently only FD_CLOEXEC).

F_GETFL: Get the file status flags (e.g., O_APPEND, O_NONBLOCK, O_RDWR, etc.) associated with the open file description.

F_SETFL: Set the file status flags. Only O_APPEND, O_NONBLOCK, O_ASYNC, and a few others can typically be changed after opening.

F_GETLK: Test for the existence of a file lock. Takes a pointer to a struct flock (arg). Non-blocking.

F_SETLK: Acquire or release an advisory file lock. Takes a pointer to a struct flock (arg). Non-blocking (returns error if lock is unavailable).

F_SETLKW: Acquire or release an advisory file lock. Takes a pointer to a struct flock (arg). Blocks if the lock is unavailable until it can be acquired.

Use Cases:

Setting/clearing the close-on-exec flag (FD_CLOEXEC) on a file descriptor.

Getting or setting non-blocking mode (O_NONBLOCK) on a file descriptor after it's been opened.

Implementing advisory file locking (shared/read locks, exclusive/write locks) to coordinate access between different processes accessing the same file.

Duplicating file descriptors with more control than dup().

// Set non-blocking mode on an existing fd
int flags = fcntl(fd, F_GETFL, 0);
if (flags == -1) { perror("fcntl F_GETFL failed"); }
flags |= O_NONBLOCK;
if (fcntl(fd, F_SETFL, flags) == -1) {
    perror("fcntl F_SETFL failed");
}

// Set close-on-exec flag
if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
     perror("fcntl F_SETFD failed");
}

// Acquire an exclusive write lock on the entire file (blocking)
struct flock fl;
fl.l_type   = F_WRLCK;  // Exclusive write lock
fl.l_whence = SEEK_SET; // Base offset for l_start
fl.l_start  = 0;        // Start offset
fl.l_len    = 0;        // Lock entire file (0 means to EOF)
fl.l_pid    = getpid(); // Optional: Process ID

if (fcntl(fd, F_SETLKW, &fl) == -1) {
    perror("fcntl F_SETLKW failed");
}
// ... Critical section ...

// Release the lock
fl.l_type = F_UNLCK;
if (fcntl(fd, F_SETLK, &fl) == -1) { // Use F_SETLK for unlock
    perror("fcntl F_UNLCK failed");
}
Use code with caution.
C
6. File Synchronization
Ensuring data written to files is physically stored on the underlying device.

### fsync()
(Requires <unistd.h>)

Synopsis: Transfers ("flushes") all modified data and metadata (like modification time, file size) associated with the file descriptor fd to the underlying permanent storage device (e.g., disk). The call blocks until the device reports completion.

Use Cases:

Ensuring data durability, critical for databases, transaction logs, or any application where data loss after a crash is unacceptable.

Guaranteeing that changes are visible to other processes or systems accessing the same storage.

ssize_t bytes_written = write(fd, data, len);
// ... check bytes_written ...

// Ensure data and metadata are on disk
if (fsync(fd) == -1) {
    perror("fsync failed");
}
Use code with caution.
C
### fdatasync()
(Requires <unistd.h>)

Synopsis: Similar to fsync(), but may be more efficient. It flushes all modified data for the file descriptor fd to the storage device. It also flushes metadata changes needed to allow the data to be retrieved correctly (e.g., file size updates). It does not necessarily flush non-essential metadata like modification times.

Use Cases:

Performance-sensitive applications requiring data durability but not necessarily immediate updates of all metadata (like mtime). Often sufficient for database systems.

ssize_t bytes_written = write(fd, data, len);
// ... check bytes_written ...

// Ensure data (and necessary metadata) are on disk
if (fdatasync(fd) == -1) {
    perror("fdatasync failed");
}
Use code with caution.
C
7. Advanced I/O
More sophisticated I/O mechanisms beyond basic read/write.

### mmap()
(Requires <sys/mman.h>)

Synopsis: Maps a file (or other object) into the process's virtual memory address space. Allows file I/O to be treated as memory access.

Arguments:

addr: Hint for starting address (usually NULL lets the kernel choose).

length: Length of the mapping.

prot: Memory protection flags (PROT_READ, PROT_WRITE, PROT_EXEC, PROT_NONE). Must be compatible with file open mode.

flags: Mapping flags (MAP_SHARED, MAP_PRIVATE, MAP_FIXED, MAP_ANONYMOUS, etc.).

MAP_SHARED: Writes to memory are written back to the file and visible to other processes mapping the same file.

MAP_PRIVATE: Creates a copy-on-write mapping; changes are not written back to the file and not visible to others.

fd: File descriptor of the file to map (use -1 with MAP_ANONYMOUS for memory not backed by a file).

offset: Offset within the file where the mapping should start (must be multiple of page size).

Return Value: Pointer to the mapped memory region on success, MAP_FAILED ((void *) -1) on error.

Use Cases:

High-performance file I/O (avoids buffer copies between kernel and user space).

Random access to large files.

Sharing memory between related processes (using MAP_SHARED on a file or /dev/zero).

Creating anonymous memory regions (using MAP_ANONYMOUS).

Loading executable code and libraries (done by the dynamic loader).

### munmap()
(Requires <sys/mman.h>)

Synopsis: Unmaps a previously mapped memory region.

Arguments:

addr: Starting address of the region to unmap (must be the value returned by mmap).

length: Length of the region to unmap (should match the length used in mmap).

Use Cases:

Releasing memory resources obtained via mmap when no longer needed. Changes made to MAP_SHARED regions might be written back to the file before or during unmapping.

### msync()
(Requires <sys/mman.h>)

Synopsis: Synchronizes a file mapping with the underlying storage. Flushes changes made to the mapped memory region back to the file.

Arguments:

addr: Address within the mapped region.

length: Length of the sub-region to synchronize.

flags: Synchronization flags (MS_SYNC, MS_ASYNC, MS_INVALIDATE).

MS_SYNC: Perform synchronous write-back. Blocks until disk write completes.

MS_ASYNC: Schedule write-back, but return immediately.

MS_INVALIDATE: Ask kernel to invalidate cached copies of mapped data (useful after file changed on disk by another process).

Use Cases:

Ensuring data written to a MAP_SHARED mapping is durable on disk, similar to fsync but for memory-mapped regions.

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

// Example using mmap for writing
int fd = open("mapped_file.dat", O_RDWR | O_CREAT, 0644);
if (fd == -1) { /* error */ }

size_t file_size = 4096;
if (ftruncate(fd, file_size) == -1) { /* error */ } // Ensure file has size

char *map = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if (map == MAP_FAILED) {
    perror("mmap failed");
    close(fd);
    // handle error
}
close(fd); // fd can be closed after successful mmap

// Write to the mapped memory
strcpy(map, "Data written via mmap!");
map[1000] = 'X';

// Synchronize changes to disk
if (msync(map, file_size, MS_SYNC) == -1) {
    perror("msync failed");
}

// Unmap the memory
if (munmap(map, file_size) == -1) {
    perror("munmap failed");
}
Use code with caution.
C
### Asynchronous I/O (AIO) - aio_* functions
(Requires <aio.h>)

Synopsis: A family of functions (aio_read, aio_write, aio_fsync, aio_error, aio_return, aio_suspend, aio_cancel, lio_listio) that allow initiating one or more I/O operations that proceed concurrently with application execution. The application is notified later (e.g., via signals or polling) when the operation completes.

Key Structure: struct aiocb (Asynchronous I/O Control Block) holds parameters for an AIO request (fd, buffer, size, offset, notification method).

Use Cases:

High-performance applications (e.g., database servers, web servers) that need to overlap I/O operations with computation or other I/O.

Handling many concurrent I/O requests efficiently.

Note: POSIX AIO can be complex to use correctly and its implementation quality/performance varies across systems. Some platforms might implement it using threads internally (thread-pool AIO), negating some potential benefits over manual threading. Linux has its own native AIO interface (io_submit, etc.) which is often preferred for maximum performance there.

8. Temporary Files and Directories
Functions for securely creating temporary files and directories.

### mkstemp()
(Requires <stdlib.h> or <unistd.h> depending on standard level)

Synopsis: Creates a unique temporary file from a template string (e.g., /tmp/myapp_XXXXXX). The XXXXXX portion is replaced with characters to make the filename unique. The file is created with mode 0600 (read/write for owner only) and opened for reading and writing. Returns an open file descriptor on success, or -1 on error.

Use Cases:

Securely creating temporary files without race conditions on the filename. The file is created and opened atomically. Often used in conjunction with unlink() right after opening if the file only needs to exist while the process holds the fd open.

### mkdtemp()
(Requires <stdlib.h> or <unistd.h>)

Synopsis: Creates a unique temporary directory from a template string (e.g., /tmp/myappdir_XXXXXX). The XXXXXX portion is replaced. The directory is created with mode 0700 (read/write/execute for owner only). Returns the pointer to the modified template string (containing the actual directory name) on success, or NULL on error.

Use Cases:

Securely creating temporary directories without race conditions.

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Create a temporary file
char template_file[] = "/tmp/myapp_XXXXXX";
int temp_fd = mkstemp(template_file);
if (temp_fd == -1) {
    perror("mkstemp failed");
} else {
    printf("Created temp file: %s (fd: %d)\n", template_file, temp_fd);
    // Optional: Unlink name immediately if only fd access is needed
    // unlink(template_file);
    // ... use temp_fd ...
    close(temp_fd);
    // Optional: remove file if not unlinked earlier
    // unlink(template_file);
}


// Create a temporary directory
char template_dir[] = "/tmp/myappdir_XXXXXX";
char *temp_dir_name = mkdtemp(template_dir);
if (temp_dir_name == NULL) {
    perror("mkdtemp failed");
} else {
    printf("Created temp directory: %s\n", temp_dir_name);
    // ... use temp_dir_name ...
    // Clean up directory and its contents later
    // rmdir(temp_dir_name); // (only if empty)
}
