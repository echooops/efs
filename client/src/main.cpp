#include <iostream>

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <log/logger.hpp>
#include <comm/client.hpp>
#include <json/json.hpp>

#include <fuse/fuse.h>

#include <sys/types.h>
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

void to_json(nlohmann::json& j, const struct stat& stbuf) {
    j = nlohmann::json{{"st_mode", stbuf.st_mode},
                       {"st_ino", stbuf.st_ino},
                       {"st_dev", stbuf.st_dev},
                       {"st_rdev", stbuf.st_rdev},
                       {"st_nlink", stbuf.st_nlink},
                       {"st_uid", stbuf.st_uid},
                       {"st_gid", stbuf.st_gid},
                       {"st_size", stbuf.st_size},
                       {"st_atime", stbuf.st_atime},
                       {"st_mtime", stbuf.st_mtime},
                       {"st_ctime", stbuf.st_ctime},
                       {"st_blksize", stbuf.st_blksize},
                       {"st_blocks", stbuf.st_blocks}};
}

void from_json(const nlohmann::json& j, struct stat &stbuf) {
    j.at("st_mode").get_to(stbuf.st_mode);
    j.at("st_ino").get_to(stbuf.st_ino);
    j.at("st_dev").get_to(stbuf.st_dev);
    j.at("st_rdev").get_to(stbuf.st_rdev);
    j.at("st_nlink").get_to(stbuf.st_nlink);
    j.at("st_uid").get_to(stbuf.st_uid);
    j.at("st_gid").get_to(stbuf.st_gid);
    j.at("st_size").get_to(stbuf.st_size);
    j.at("st_atime").get_to(stbuf.st_atime);
    j.at("st_mtime").get_to(stbuf.st_mtime);
    j.at("st_ctime").get_to(stbuf.st_ctime);
    j.at("st_blksize").get_to(stbuf.st_blksize);
    j.at("st_blocks").get_to(stbuf.st_blocks);
}

int efs_access(const char *path, int mask)
{
    nlohmann::json req = { {"method", "access"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"mask", mask}}}};

    try {
        auto rep = global::client.request(req);
        LDEBUG("efs_access [{}]\nreq:{}\nrep:{}", path, req.dump().c_str(), rep.dump().c_str());
        return rep["ret"];
    } catch (std::exception &e) {
        LERROR("{}", e.what());
        return -1;
    }
}
#ifdef __APPLE__
int efs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t position)
{
    return -ENOATTR;
}
int efs_getxattr(const char *path, const char *name, char *value, size_t size, uint32_t position)
{
    nlohmann::json req = { {"method", "access"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"name", name}}}};
    LDEBUG("efs_getxattr {} ----------------------------------------- req \n{}", path, req.dump().c_str());   
    return -ENOATTR;
}
#else /* !__APPLE__ */
int efs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    return -ENOATTR;
}
int efs_getxattr(const char *path, const char *name, char *value, size_t size)
{
    return -ENOATTR;
}
#endif /* __APPLE__ */

int efs_listxattr(const char *path, char *list, size_t size)
{
    LDEBUG("efs_listxattr {} ----------------------------------------- req \n{}", path);   
    return -ENOATTR;
}
int efs_removexattr(const char *path, const char *name)
{
    return -ENOATTR;
}


// 获取节点属性
int efs_getattr(const char *path, struct stat *stbuf)
{
    nlohmann::json req = { {"method", "getattr"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"stat", *stbuf}}}};
    try {
        auto rep = global::client.request(req);
        LDEBUG("efs_getattr [{}]\nreq:{}\nrep:{}", path, req.dump().c_str(), rep.dump().c_str());
        *stbuf = rep["stat"];
        return rep["ret"];
    } catch (std::exception &e) {
        LERROR("{}", e.what());
        return -ENOENT;
    }
}

int efs_mkdir(const char *path, mode_t mode)
{
    nlohmann::json req = { {"method", "mkdir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"mode", mode}}}};
    LDEBUG("efs_mkdir {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_mkdir -------------------------------------------- rep \n{}", rep.dump().c_str());
    return rep["ret"];
}

int efs_opendir(const char *path, struct fuse_file_info *fi)
{
    LDEBUG("efs_opendir {} -------------------------------------------- req \n", path);
    return 0;
}

int efs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    nlohmann::json req = { {"method", "readdir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_readdir {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_readdir -------------------------------------------- rep \n{}", rep.dump().c_str());
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
    
    LDEBUG("efs_rmdir {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_readdir -------------------------------------------- rep \n{}", rep.dump().c_str());

    return rep["ret"];
}

int efs_releasedir(const char *path, fuse_file_info *fi)
{
    nlohmann::json req = { {"method", "releasedir"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_releasedir {} ----------------------------------------- req \n{}", path, req.dump().c_str());

    return 0;
}

int efs_open(const char *path, struct fuse_file_info *fi)
{

    nlohmann::json req = { {"method", "open"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_open {} ----------------------------------------- req \n{}", path, req.dump().c_str());

    return 0;
}
int efs_release(const char *path, struct fuse_file_info *fi)
{

    nlohmann::json req = { {"method", "release"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_release {} ----------------------------------------- req \n{}", path, req.dump().c_str());

    return 0;
}
int efs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    
    nlohmann::json req = { {"method", "read"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_read {} ----------------------------------------- req \n{}", path, req.dump().c_str());

    return 0;
}
int efs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{

    nlohmann::json req = { {"method", "write"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}}};
    
    LDEBUG("efs_write {} ----------------------------------------- req \n{}", path, req.dump().c_str());

    return 0;
}
int efs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    nlohmann::json req = { {"method", "mknod"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"mode", mode},
                                     {"rdev", rdev}}}};
    
    LDEBUG("efs_mknod {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    return 0;
}
int efs_unlink(const char *path)
{
    nlohmann::json req = { {"method", "unlink"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}} };
    LDEBUG("efs_unlink {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_unlink  ------------------------------------------- req \n{}", rep.dump().c_str());

    return rep["ret"];
}

int efs_chmod(const char *path, mode_t mode)
{
    nlohmann::json req = { {"method", "chmod"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"mode", mode}}} };
    
    LDEBUG("efs_chmod {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_chmod -------------------------------------------- rep \n{}", rep.dump().c_str());
    return rep["ret"];
}

int efs_statfs(const char *path, struct statvfs *buf)
{
    nlohmann::json req = { {"method", "chmod"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}}} };
    
    LDEBUG("efs_statfs {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    return 0;
}
int efs_readlink(const char *path, char *buf, size_t len)
{
    nlohmann::json req = { {"method", "chmod"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path},
                                     {"buf", buf},
                                     {"len", len}}} };
    LDEBUG("efs_readlink {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    return 0;
}
int efs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    nlohmann::json req = { {"method", "create"},
                           {"context", *fuse_get_context()},
                           {"args", {{"path", path}, {"mode", mode}}}};
    LDEBUG("efs_create {} ----------------------------------------- req \n{}", path, req.dump().c_str());
    auto rep = global::client.request(req);
    LDEBUG("efs_create -------------------------------------------- rep \n{}", rep.dump().c_str());
    return rep["ret"];
}


int main(int argc, char *argv[])
{
    struct fuse_operations fake_ops;
    memset(&fake_ops, 0, sizeof(struct fuse_operations));

    fake_ops.getattr = efs_getattr;
    fake_ops.readlink = efs_readlink;
    fake_ops.mknod = efs_mknod;
    fake_ops.unlink = efs_unlink;
    fake_ops.symlink = nullptr;
    fake_ops.rename = nullptr;
    fake_ops.link = nullptr;
    fake_ops.chmod = efs_chmod;
    fake_ops.chown = nullptr;
    fake_ops.truncate = nullptr;
    fake_ops.utime = nullptr;

    fake_ops.open = efs_open;
    fake_ops.read = efs_read;
    fake_ops.release = efs_release;
    fake_ops.write = efs_write;

    fake_ops.statfs = efs_statfs;
    fake_ops.flush = nullptr;
    fake_ops.fsync = nullptr;

    fake_ops.setxattr = efs_setxattr;
    fake_ops.getxattr = efs_getxattr;
    fake_ops.listxattr = efs_listxattr;
    fake_ops.removexattr = efs_removexattr;

    fake_ops.mkdir = efs_mkdir;
    fake_ops.rmdir = efs_rmdir;
    fake_ops.getdir = nullptr;
    fake_ops.opendir = efs_opendir;
    fake_ops.readdir = efs_readdir;
    fake_ops.releasedir = efs_releasedir;
    fake_ops.fsyncdir = nullptr;

    fake_ops.init = nullptr;
    fake_ops.destroy = nullptr;
    fake_ops.access = efs_access;
    fake_ops.create = efs_create;
    fake_ops.ftruncate = nullptr;
    fake_ops.fgetattr = nullptr;
 
    if (!fuse_main(argc, argv, &fake_ops, nullptr)) {
        std::cout << "hello world!" << "\n";
    }
    
    return 0;
}
