#include <errno.h>
#include <fcntl.h> /* Definition of AT_* constants */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
/**
  ERRORS
     EACCES
            The parent directory does not allow write permission to the process,
   or one of the directories in path did not allow search  permis‐ sion.  (See
   also path_resolution(7).)

     EBADF  (mkdirat()) path is relative but dirfd is neither AT_FDCWD nor a
   valid file descriptor.

     EDQUOT
            The user's quota of disk blocks or inodes on the filesystem has been
   exhausted.

     EEXIST
            path already exists (not necessarily as a directory).  This includes
   the case where path is a symbolic link, dangling or not.

     EFAULT
            path points outside your accessible address space.

     EINVAL
            The  final component ("basename") of the new directory's path is
   invalid (e.g., it contains characters not permitted by the underly‐ ing
   filesystem).

     ELOOP  Too many symbolic links were encountered in resolving path.

     EMLINK
            The number of links to the parent directory would exceed LINK_MAX.

     ENAMETOOLONG
            path was too long.

     ENOENT
            A directory component in path does not exist or is a dangling
   symbolic link.

     ENOMEM
            Insufficient kernel memory was available.

     ENOSPC
            The device containing path has no room for the new directory.

     ENOSPC
            The new directory cannot be created because the user's disk quota is
   exhausted.

     ENOTDIR
            A component used as a directory in path is not, in fact, a
   directory.

     ENOTDIR
            (mkdirat()) path is relative and dirfd is a file descriptor
   referring to a file other than a directory.

     EPERM  The filesystem containing path does not support the creation of
   directories.

     EROFS  path refers to a file on a read-only filesystem.

     EOVERFLOW
            UID or GID mappings (see user_namespaces(7)) have not been
   configured.

*/

typedef struct vector_data {
  const char *path;
  mode_t mode;

} vector_data;
typedef struct vector {
  vector_data *vector_data;
  int len;
} vector;
int mkdir(const char *path, mode_t mode);
void mkdir_print_err(vector *vec) {
  for (int i = 0; i < vec->len; i++) {
    int res = mkdir(vec->vector_data[i].path, vec->vector_data[i].mode);
    printf("DEBUG: res: `%d` errno: `%d`. strerror: `%s`\n", res, errno,
           strerror(errno));
  }
}
int main() {

  // So relative path `~` traversal `../` current `./` root `/`
  // `~` && `/`(permission denied) doesnt work
  // `../` && `./` works
  vector_data *v_data = (vector_data[]){
      {"~/mkdir-directory", S_IRUSR | S_IWUSR},
      {"../mkdir-directory-path-traversal", S_IRUSR | S_IWUSR},
      {"./mkdir-directory-local", S_IRUSR | S_IWUSR},
      {"/mkdir-directory-root", S_IRUSR | S_IWUSR},
  };
  vector vec = {v_data, 4};

  mkdir_print_err(&vec);
  return 0;
}
