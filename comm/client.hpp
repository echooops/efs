#pragma once
#include <iostream>
#include <exception>
#include <thread>
#include <vector>

#include <log/logger.hpp>
#include <json/json.hpp>
#include <nn/nn.hpp>

namespace comm {

    class client            // client
    {
    public:
        client (const char *addr) : req_sock_(AF_SP, NN_REQ)
        {
            req_sock_.connect(addr);
        }
        virtual ~client () {
        }

        nlohmann::json request (const nlohmann::json &req, int timeout = 3000)
        {
            std::string sndbuf = std::move(req.dump());
            req_sock_.send (sndbuf.c_str(), sndbuf.size() + 1, 0);
            // 收取回复结果
            char * buf = nullptr;
            int size = req_sock_.recv(&buf, NN_MSG, 0);
            auto rep = nlohmann::json::parse(buf);
            nn::freemsg(buf);
            return rep;
        }
    private:
        nn::socket req_sock_;
    };

}  // comm
