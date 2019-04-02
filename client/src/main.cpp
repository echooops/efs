#include <iostream>

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <log/logger.hpp>
#include <comm/client.hpp>
#include <json/json.hpp>

#include <fuse/fuse.h>
#include <sys/xattr.h>

#include <unistd.h>             // rmdir
#include <dirent.h>

namespace global {
    utils::log::logger logger ("/tmp/efsfuse_client.txt", 1024*1024*64, 2);
    comm::client client ("ipc:///tmp/efsfuse.ipc");
}  // global


// struct fuse_context {
//     struct fuse *   fuse;
//     uid_t     uid;              /* effective user id */
//     gid_t     gid;              /* effective group id */
//     pid_t     pid;              /* thread id */
//     void      *private_data;	/* set by file system on mount */
//     mode_t    umask;            /* umask of the thread */
// };


void to_json(nlohmann::json &j, const struct fuse_context &context) {
    j = {{"uid", context.uid},
         {"gid", context.gid},
         {"pid", context.pid},
         {"umask", context.umask}};
}

void from_json(const nlohmann::json& j, struct stat &stbuf) {
    j.at("st_uid").get_to(stbuf.st_uid);
    j.at("st_gid").get_to(stbuf.st_gid);
    j.at("st_mode").get_to(stbuf.st_mode);
    j.at("st_nlink").get_to(stbuf.st_nlink);
    j.at("st_size").get_to(stbuf.st_size);
}

// 获取节点属性
int efs_getattr(const char *path, struct stat *stbuf)
{
    nlohmann::json req = { {"method", "getattr"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};

    // LDEBUG("efs_getattr -------------------------------------------- req \n{}", req.dump().c_str());
    auto rep = global::client.request(req);
    // LDEBUG("efs_getattr -------------------------------------------- rep \n{}", rep.dump().c_str());
    int ret = rep["ret"];
    if (ret == 0)
        *stbuf = rep["stat"];
    return ret;
}

int efs_mkdir(const char *path, mode_t mode)
{
    nlohmann::json req = { {"method", "mkdir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_mkdir -------------------------------------------- req \n{}", req.dump().c_str());

    return mkdir(path, mode);
}

int efs_opendir(const char *path, struct fuse_file_info *fi)
{
    LDEBUG("efs_opendir -------------------------------------------- req \n{}", path);
    return 0;
}

int efs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    nlohmann::json req = { {"method", "readdir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_readdir -------------------------------------------- req \n{}", req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_readdir -------------------------------------------- req \n{}", rep.dump().c_str());
    for (auto &e : rep["node"]) {
        filler(buf, e.get<std::string>().c_str(), nullptr, 0);
    }
    return rep["ret"];
}



int efs_rmdir(const char *path)
{
    nlohmann::json req = { {"method", "rmdir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_rmdir -------------------------------------------- req \n{}", req.dump().c_str());

    return rmdir(path);
}

int efs_releasedir(const char *path, fuse_file_info *fi)
{
    nlohmann::json req = { {"method", "releasedir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_releasedir -------------------------------------------- req \n{}", req.dump().c_str());

    return 0;
}

int efs_open(const char *path, struct fuse_file_info *fi)
{

    nlohmann::json req = { {"method", "open"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_open -------------------------------------------- req \n{}", req.dump().c_str());

    return 0;
}
int efs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    
    nlohmann::json req = { {"method", "read"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_read -------------------------------------------- req \n{}", req.dump().c_str());

    return 0;
}
int efs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{

    nlohmann::json req = { {"method", "write"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_write -------------------------------------------- req \n{}", req.dump().c_str());

    return 0;
}
int efs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    nlohmann::json req = { {"method", "mknod"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"mode", mode},
                                     {"rdev", rdev}}}};
    
    LDEBUG("efs_mknod -------------------------------------------- req \n{}", req.dump().c_str());
    return 0;
}
int efs_unlink()
{
    
}

int efs_chmod(const char *path, mode_t mode)
{
    nlohmann::json req = { {"method", "chmod"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"mode", mode}}} };
    
    LDEBUG("efs_chmod -------------------------------------------- req \n{}", req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_chmod -------------------------------------------- req \n{}", rep.dump().c_str());
    return rep["ret"];
}


const struct fuse_operations fake_ops = {
    .getattr = efs_getattr,
    .readlink = nullptr,
    .mknod = efs_mknod,
    .unlink = nullptr,
    .symlink = nullptr,
    .rename = nullptr,
    .link = nullptr,
    .chmod = efs_chmod,
    .chown = nullptr,
    .truncate = nullptr,
    .utime = nullptr,

    .open = efs_open,
    .read = efs_read,
    .release = nullptr,
    .write = efs_write,

    .statfs = nullptr,
    .flush = nullptr,
    .fsync = nullptr,

    .setxattr = nullptr,
    .getxattr = nullptr,
    .listxattr = nullptr,
    .removexattr = nullptr,

    .mkdir = efs_mkdir,
    .rmdir = efs_rmdir,
    .getdir = nullptr,
    .opendir = efs_opendir,
    .readdir = efs_readdir,
    .releasedir = efs_releasedir,
    .fsyncdir = nullptr,

    .init = nullptr,
    .destroy = nullptr,
    .access = nullptr,
    .create = nullptr,
    .ftruncate = nullptr,
    .fgetattr = nullptr,
};


int main(int argc, char *argv[])
{

    if (!fuse_main(argc, argv, &fake_ops, nullptr)) {
        std::cout << "hello world!" << "\n";
    }
    
    return 0;
}
