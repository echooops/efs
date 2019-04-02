#pragma once
#include <cstdio>
#include <cstring>
#include <iostream>
#include <exception>
#include <functional>
#include <thread>
#include <map>

#include <thread/threadpool.hpp>
#include <log/logger.hpp>
#include <json/json.hpp>
#include <nn/nn.hpp>

namespace comm {
    
    class server             // 处理订阅的任务
    {
    public:
        server (const char *addr) : running_ (false)
                                  , rep_sock_ (AF_SP, NN_REP)
                                  , pool (4)
        {
            rep_sock_.bind (addr);
        }
        virtual ~server()
        {
            stop();
        }

        void start ()           // 开始接受订阅消息
        {
            try {

                running_ = true;
                // 启动消息接收器
                receiver_thread_ = std::thread (&server::receiver, this);

            } catch (std::exception &e) {
                LERROR ("[nanomsg]:{}", e.what());
            }
        }
        
        void stop ()
        {
            running_ = false;
            // 关闭sock
            rep_sock_.close();
            if (receiver_thread_.joinable()) {
                receiver_thread_.join();
            }
        }

        void receiver ()
        {
            while (running_) {
                try {
                    char * buf = nullptr;
                    size_t size = rep_sock_.recv(&buf, NN_MSG, 0);


                    LDEBUG ("收到消息:size({}) len({}) data({})", size, strlen(buf), buf);
                    
                    nlohmann::json j = nlohmann::json::parse(buf);
                    
                    std::string method = j["method"];
            
                    // 处理信息
                    nlohmann::json rep = methods_[method](j);
                    std::string sndbuf = std::move(rep.dump());
                    LDEBUG ("回复消息: {}", sndbuf.c_str());
                    rep_sock_.send(sndbuf.c_str(), sndbuf.size() + 1, 0);

                    if (buf)
                        nn::freemsg(buf);
                } catch  (std::exception &e) {
                    LERROR ("[nanomsg]:{}", e.what());
                }
            }
        }
        
        void register_methods (std::string method, std::function<nlohmann::json(const nlohmann::json &)> func)
        {
            if (!running_)
                methods_[method] = std::move(func);
            else
                LWARN ("method 已订阅，若需插入新的话题，需要先stop，注册方法，再start");
        }

    private:
        
        bool running_;
            
        nn::socket rep_sock_;

        std::thread receiver_thread_;
            
        std::map<std::string, std::function<nlohmann::json(const nlohmann::json &)>> methods_;

        utils::thread::threadpool pool; // 定义线程池
    };

}  // utils
