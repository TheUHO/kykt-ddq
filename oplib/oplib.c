#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>
#include	<unistd.h>

#define PATH_SIZE 1000

int is_valid_filename(const char *path) {  
    // 简单的检查，确保不包含".."  
    if (strstr(path, "..") != NULL) {
        return 0;
    }
    return 1;
}  

int is_standard_file(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return 0;
    }
    // 检查文件是否为标准文件类型
    return S_ISREG(path_stat.st_mode);
}

char *  find_filename(char *fn)
{
	// TODO
	// 依次使用当前位置和 oplib_path 宏指定的位置为基础，寻找相对路径fn指定的文件。
	// free掉输入的fn，重新malloc内存返回
	// 如果找不到文件，则返回NULL
	// 额外功能：检查文件名的合法性：
	// 		不允许包括“..”
	// 		不允许文件类型为非标准文件，比如软链接或FIFO等——真需要这么严格吗？也许不需要！
	// 目前版本是先凑合一下的，回头要改掉

	char *buf = NULL, *path = NULL;

    // 检查文件名合法性
    if (!is_valid_filename(fn)) {
        free(fn);
        return NULL;
    }

    // 首先检查当前目录
    if ((path = getcwd(NULL, 0)) == NULL) {
        free(fn);
        free(path);
        return NULL;
    }

    // 构造在 当前目录 中的路径
    buf = malloc(strlen(path) + strlen(fn) + 2);
    if (!buf) {
        free(fn);
        free(path);
        return NULL;
    }
    strcpy(buf, path);
    strcat(buf, "/");
    strcat(buf, fn);

    // 检查在 当前目录 中的文件
    if (is_standard_file(buf)) {
        free(fn);
        free(path);
        return buf;
    }
    free(path);
    free(buf);

    // 构造在 oplib_path 中的路径
    buf = malloc(strlen(oplib_path) + strlen(fn) + 2);
    if (!buf) {
        free(fn);
        return NULL;
    }
    strcpy(buf, oplib_path);
    strcat(buf, "/");
    strcat(buf, fn);

    // 检查在 oplib_path 中的文件
    if (is_standard_file(buf)) {
        free(fn);
        return buf;
    }

    // 文件未找到，清理并返回 NULL
    free(buf);
    free(fn);
    return NULL;
}


