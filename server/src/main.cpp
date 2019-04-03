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
    j = nlohmann::json{{"st_uid", stbuf.st_uid},
                       {"st_gid", stbuf.st_gid},
                       {"st_mode", stbuf.st_mode},
                       {"st_nlink", stbuf.st_nlink},
                       {"st_size", stbuf.st_size}};
}

struct node_t {
    int mode_t;
    int type;
    std::string name;
    std::string hash;
    std::vector<struct node_t> subs;
};

nlohmann::json rxc_getattr(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    struct stat st;
    // int ret = lstat("/", &st);
    //  nlohmann::json j = st;
    // LDEBUG("看看权限 {}", j.dump(4).c_str());
    
    // nlohmann::json rep = { {"ret", -1},
    //                        {"stat", st} };
    nlohmann::json rep = {{"ret", -1},
                          {"stat", st}};
//    rep["ret"] = 0;
    if (global::o.find(path) != global::o.end()) {
        rep["ret"] = 0;
        rep["stat"] = global::o[path];
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
nlohmann::json rxc_chmod(const nlohmann::json &req)
{
    std::string path = req["args"]["path"];

    global::o[path]["st_mode"] = req["args"]["mode"];
    
    std::ofstream o("dtree.json");
    
    o << std::setw(4) << global::o << std::endl;

    return {{"ret", 0}};
}

void register_methods()
{
    global::server.register_methods("getattr", rxc_getattr);
    global::server.register_methods("readdir", rxc_readdir);
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
