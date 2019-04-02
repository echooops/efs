#pragma once

#define SPDLOG_FUNCTION __PRETTY_FUNCTION__
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <cstdarg>
#include <unistd.h>
#include <sys/stat.h>

namespace utils {
    
    namespace log {

        class logger
        {
        public:
            logger(const char *filename, size_t max_file_size, size_t max_files)
            {
                mkdirs (filename); // 检查并创建不存在目录
                auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
                // 设置打印级别
#ifdef DEBUG
                stdout_sink->set_level(spdlog::level::debug);
#else 
                // 当发出 warn 或更严重的错误时立刻刷新到日志
                stdout_sink->set_level(spdlog::level::warn);
#endif
                
                auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filename, max_file_size, max_files);
                // 设置打印级别
                rotating_sink->set_level(spdlog::level::debug);
                std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
                auto my_logger = std::make_shared<spdlog::logger>("loggername", sinks.begin(), sinks.end());
#ifdef DEBUG
                my_logger->flush_on(spdlog::level::debug);
#else 
                // 当发出 warn 或更严重的错误时立刻刷新到日志
                my_logger->flush_on(spdlog::level::warn);
#endif
                spdlog::register_logger(my_logger);
                spdlog::set_default_logger(my_logger);
                spdlog::set_pattern("[%H:%M:%S] [%^---%L---%$] [%v]\n[%C-%m-%d] [thread %t] [%s:%!:%#]");
            }
            virtual ~logger()
            {
                spdlog::drop_all();
            }

        private:
            inline void mkdirs(const char *muldir)
            {
                char buf[4096] = {0};
                strncpy(buf, muldir, 2096);
                for (auto &ch : buf) {
                    if (ch == '/') {
                        ch = '\0';
                        if (access (buf, 0) != 0)
                            mkdir (buf, 0755);
                        ch = '/';
                    }
                }
            }
        };

    }  // log
    
}  // utils

// 这段代码的目的实际上是为了去掉编译告警 [-Wformat-extra-args]
inline void logger_fmt(char *out, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    vsnprintf(out, size, fmt, args);
    va_end (args);
}

// 支持传统 %s 控制符输出，还支持spdlog {} 条件控制输出，无缝替换
#define LOGGER_CALL(logger, fmt, ...)                               \
    {                                                               \
        if (strchr(fmt, '%')) {                                     \
            char temp[2048] = {0};                                  \
            logger_fmt (temp, sizeof(temp), fmt, ##__VA_ARGS__);    \
            logger (temp);                                          \
        } else {                                                    \
            logger (fmt, ##__VA_ARGS__);                            \
        }                                                           \
    }

#define LTRACE(fmt, ...) LOGGER_CALL(SPDLOG_TRACE, fmt, ##__VA_ARGS__)
#define LDEBUG(fmt, ...) LOGGER_CALL(SPDLOG_DEBUG, fmt, ##__VA_ARGS__)
#define LINFO(fmt, ...) LOGGER_CALL(SPDLOG_INFO, fmt, ##__VA_ARGS__)
#define LWARN(fmt, ...) LOGGER_CALL(SPDLOG_WARN, fmt, ##__VA_ARGS__)
#define LERROR(fmt, ...) LOGGER_CALL(SPDLOG_ERROR, fmt, ##__VA_ARGS__)
#define LFATAL(fmt, ...) LOGGER_CALL(SPDLOG_CRITICAL, fmt, ##__VA_ARGS__)

/*
 * Pattern说明
 * 输出格式的Pattern中可以有若干 %开头的标记，含义如下表：
 * 标记	说明
 * %v	实际需要被日志记录的文本，如果文本中有{占位符}会被替换
 * %t	线程标识符
 * %P	进程标识符
 * %n	日志记录器名称
 * %l	日志级别
 * %L	日志级别简写
 * %a	简写的周几，例如Thu
 * %A	周几，例如Thursday
 * %b	简写的月份，例如Aug
 * %B	月份，例如August
 * %c	日期时间，例如Thu Aug 23 15:35:46 2014
 * %C	两位年份，例如14
 * %Y	四位年份，例如2014
 * %D 或 %x	MM/DD/YY格式日期，例如"08/23/14
 * %m	月份，1-12之间
 * %d	月份中的第几天，1-31之间
 * %H	24小时制的小时，0-23之间
 * %I	12小时制的小时，1-12之间
 * %M	分钟，0-59
 * %S	秒，0-59
 * %e	当前秒内的毫秒，0-999
 * %f	当前秒内的微秒，0-999999
 * %F	当前秒内的纳秒， 0-999999999
 * %p	AM或者PM
 * %r	12小时时间，例如02:55:02 pm
 * %R	等价于%H:%M，例如23:55
 * %T 或 %X	HH:MM:SS
 * %z	时区UTC偏移，例如+02:00
 * %+	表示默认格式
 */
