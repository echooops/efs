#include <iostream>
#include <vector>
#include <fstream>
#include <iomanip>

#include <log/logger.hpp>
#include <comm/server.hpp>
#include <json/json.hpp>
#include <sys/stat.h>

namespace global {
    utils::log::logger logger ("/tmp/efsfuse_server.txt", 1024*1024*64, 2);
    comm::server server ("ipc:///tmp/efsfuse.ipc");

    nlohmann::json o;           // object
}  // global

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

nlohmann::json rxc_access(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    if (global::o.find(path) != global::o.end()) {
        return {{"ret", 0}};
    }
    else {
        return {{"ret", -1}};
    }
}

nlohmann::json rxc_getattr(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    // path = "." + path;
    
    // LDEBUG("看看权限 {}", path.c_str());
    // struct stat st = req["args"]["stat"];
    
    // int ret = lstat(path.c_str(), &st);
    //  nlohmann::json j = st;
    // LDEBUG("看看权限 {}", j.dump(4).c_str());
    
    nlohmann::json rep = { {"ret", -ENOENT},
                           {"stat", req["args"]["stat"]} };
//    return rep;
    
    // nlohmann::json rep = {{"ret", -1},
    //                       {"stat", st}};
//    rep["ret"] = 0;
    if (global::o.find(path) != global::o.end()) {
        rep["ret"] = 0;
        rep["stat"] = global::o[path]["stat"];
    }
    // nlohmann::json rep = { {"ret", ret},
    //                        {"stat", st} };
    return rep;
}

nlohmann::json rxc_readdir(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    nlohmann::json rep;

    if (path.at(path.size() - 1) != '/')
        path += '/';
    
    for (auto &e: global::o.items()) {
        if (e.key() == "/")
            continue;
        std::string::size_type pos = e.key().find(path);
        if (std::string::npos == pos)
            continue;
        if (e.key().find("/", pos + path.size()) != std::string::npos)
            continue;
        rep["node"].push_back(e.key().substr(pos + path.size()));
    }
    rep["ret"] = 0;
    return rep;
}
nlohmann::json rxc_mkdir(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    struct stat st;
    memset(&st, 0, sizeof(struct stat));

    global::o[path]["stat"] = st;

    nlohmann::json &j = global::o[path]["stat"];
    j["st_mode"] = req["args"]["mode"];
    j["st_uid"] = req["context"]["uid"];
    j["st_gid"] = req["context"]["gid"];

    // 回写回去
    std::ofstream o("dtree.json");
    o << std::setw(4) << global::o << std::endl;

    return {{"ret", 0}};
}
nlohmann::json rxc_rmdir(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    global::o.erase(path);

    // 回写回去
    std::ofstream o("dtree.json");
    o << std::setw(4) << global::o << std::endl;

    return {{"ret", 0}};
}
nlohmann::json rxc_create(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    struct stat st;
    memset(&st, 0, sizeof(struct stat));

    global::o[path]["stat"] = st;

    nlohmann::json &j = global::o[path]["stat"];
    j["st_mode"] = req["args"]["mode"];
    j["st_uid"] = req["context"]["uid"];
    j["st_gid"] = req["context"]["gid"];

    // 回写回去
    std::ofstream o("dtree.json");
    o << std::setw(4) << global::o << std::endl;

    return {{"ret", 0}};
}

nlohmann::json rxc_unlink(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    global::o.erase(path);

    // 回写回去
    std::ofstream o("dtree.json");
    o << std::setw(4) << global::o << std::endl;

    return {{"ret", 0}};
}


nlohmann::json rxc_chmod(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    global::o[path]["stat"]["st_mode"] = req["args"]["mode"];
    
    // 回写回去
    std::ofstream o("dtree.json");
    o << std::setw(4) << global::o << std::endl;

    return {{"ret", 0}};
}

void register_methods()
{
    global::server.register_methods("access", rxc_access);
    global::server.register_methods("getattr", rxc_getattr);
    global::server.register_methods("readdir", rxc_readdir);
    global::server.register_methods("mkdir", rxc_mkdir);
    global::server.register_methods("rmdir", rxc_rmdir);
    global::server.register_methods("create", rxc_create);
    global::server.register_methods("unlink", rxc_unlink);
    global::server.register_methods("chmod", rxc_chmod);
    global::server.start();
}


/* 
   040000 tree d8329fc1cc938780ffdd9f94e0d364e0ea74f579  bak
   100644 blob fa49b077972391ad58037050f2a75f74e3671e92  new.txt
   100644 blob 1f7a7a472abf3dd9643fd615f6da379c4acb3e3a  test.txt
*/



int main(int argc, char *argv[])
{

    std::ifstream i("dtree.json");
    nlohmann::json j;
    i >> j;
    LDEBUG("{}", j.dump(4).c_str());
    global::o = j;
    
    register_methods();
    std::this_thread::sleep_for(std::chrono::seconds(10000));
    
    return 0;
}
