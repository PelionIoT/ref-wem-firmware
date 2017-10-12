#include <string>
#include <FATFileSystem.h>

#define FS_NAME "sd"
#define FS_MOUNT_POINT "/" FS_NAME

int fs_init();
int fs_test();
void fs_shutdown();

int fs_ls(std::string &path);
int fs_cat(std::string &path);
int fs_mkdir(std::string &path);
int fs_remove(std::string &path);
int fs_format();
int fs_mount();
int fs_unmount();
