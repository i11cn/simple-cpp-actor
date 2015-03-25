#ifndef __SNOWER_LOGGER_H__
#define __SNOWER_LOGGER_H__

#include <sys/time.h>
#include <functional>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace snower
{

using tag_func = std::function<void (std::ostream&)>;
using log_datas = std::vector<tag_func>;

inline void log_to_stream(std::ostream& os) {}

template<typename T, typename... Types>
void log_to_stream(std::ostream& os, T value, Types... args)
{
    os << value;
    log_to_stream(os, args...);
}

void log_to_stream(std::ostream& os, const log_datas& ld, const char* format);

template<typename T, typename... Types>
void log_to_stream(std::ostream& os, const log_datas& ld, const char* format, T value, Types... args)
{
    for(; *format != '\0'; format++)
    {
        if(*format == '%')
        {
            format++;
            if(*format != '\0')
            {
                switch(*format)
                {
                case 'F':
                    ld[0](os); break;

                case 'T':
                    ld[7](os); break;

                case 'f':
                    ld[1](os); break;

                case 'p':
                    ld[5](os); break;

                case 't':
                    ld[6](os); break;

                case 'l':
                    ld[2](os); break;

                case 'L':
                    ld[4](os); break;

                case 'N':
                    ld[3](os); break;

                case '%':
                    os << '%'; break;

                case 'm':
                    os << value;
                    log_to_stream(os, ld, format + 1, args...);
                    return;

                case 'M':
                    log_to_stream(os, value, args...); break;

                default:;
                    // 允许用户扩展
                }
            }
        }
        else
        {
            os << *format;
        }
    }
}

class log_appender
{
public:
    log_appender(const char* format);
    log_appender(const std::string& format);
    log_appender(std::string&& format);
    virtual ~log_appender(void) {}
    template<typename... Types>
    void write(const log_datas& ld, Types... args)
    {
        using namespace std;
        stringstream ss;
        log_to_stream(ss, ld, m_format.c_str(), forward<Types>(args)...);
        ss << endl;
        do_write(ss.str());
    }
    template<typename... Types>
    void writef(const std::string& format, const log_datas& ld, Types... args)
    {
        using namespace std;
        stringstream ss;
        log_to_stream(ss, ld, format.c_str(), forward<Types>(args)...);
        ss << endl;
        do_write(ss.str());
    }

protected:
    virtual void do_write(const std::string& log_msg) = 0;

private:
    std::string m_format;
};

using log_appender_ref = std::shared_ptr<log_appender>;

class console_appender : public log_appender
{
public:
    using log_appender::log_appender;

protected:
    virtual void do_write(const std::string& log_msg);
};

class stderr_appender : public log_appender
{
public:
    using log_appender::log_appender;

protected:
    virtual void do_write(const std::string& log_msg);

private:
    std::mutex m_write_mutex;
};

template<typename File>
class file_appender : public log_appender
{
public:
    using file_ref = std::shared_ptr<File>;
public:
    file_appender(const char* format, File* file_obj)
    : log_appender(format)
    , m_file_obj(file_obj)
    {
    }
    file_appender(const char* format, const file_ref& file_obj)
    : log_appender(format)
    , m_file_obj(file_obj)
    {
    }
    file_appender(const char* format, file_ref&& file_obj)
    : log_appender(format)
    , m_file_obj(std::move(file_obj))
    {
    }
    file_appender(const std::string& format, File* file_obj)
    : log_appender(format)
    , m_file_obj(file_obj)
    {
    }
    file_appender(const std::string& format, const file_ref& file_obj)
    : log_appender(format)
    , m_file_obj(file_obj)
    {
    }
    file_appender(const std::string& format, file_ref&& file_obj)
    : log_appender(format)
    , m_file_obj(std::move(file_obj))
    {
    }
    file_appender(std::string&& format, File* file_obj)
    : log_appender(std::move(format))
    , m_file_obj(file_obj)
    {
    }
    file_appender(std::string&& format, const file_ref& file_obj)
    : log_appender(std::move(format))
    , m_file_obj(file_obj)
    {
    }
    file_appender(std::string&& format, file_ref&& file_obj)
    : log_appender(std::move(format))
    , m_file_obj(std::move(file_obj))
    {
    }

protected:
    virtual void do_write(const std::string& log_msg)
    {
        m_file_obj->write(log_msg);
    }

private:
    file_ref m_file_obj;
};

class file_object_base
{
public:
    template<typename Duration = int>
    file_object_base(const std::string& name, size_t size, Duration&& rel_time = Duration(0))
    : m_file_name(get_abs_path_name(name))
    , m_truncate_size(size)
    , m_file_size(0)
    {
        using namespace std;
        using namespace std::chrono;
        get_postfix_from_time = [](const system_clock::time_point& t) -> string {
            char name[64] = {0};
            time_t tm_t = system_clock::to_time_t(t);
            tm ltm = *(std::localtime(&tm_t));
            snprintf(name, 63, "%04d-%02d-%02d.%02d%02d%02d", ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
            return move(string(name));
        };
    }
    virtual ~file_object_base(void);

    void write(const std::string& msg);

protected:
    virtual void check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now) = 0;

private:
    static std::string get_abs_path_name(const std::string& name);

protected:
    std::string m_file_name;
    size_t m_truncate_size;
    std::ofstream m_file;
    size_t m_file_size;
    std::chrono::system_clock::time_point m_file_open_time;
    std::mutex m_file_mutex;

    std::function<std::chrono::system_clock::time_point(const std::chrono::system_clock::time_point&)> get_next_open_time;
    std::function<std::string(const std::chrono::system_clock::time_point&)> get_postfix_from_time;
};

class file_infinite : public file_object_base
{
public:
    file_infinite(const std::string& name)
    : file_object_base(name, 0, 0)
    {
    }
    ~file_infinite(void);

protected:
    virtual void check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now);
};

class file_truncated : public file_object_base
{
public:
    using file_object_base::file_object_base;
    ~file_truncated(void);

private:
    virtual void check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now);
};

class file_splitted : public file_object_base
{
public:
    using file_object_base::file_object_base;
    ~file_splitted(void);

private:
    virtual void check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now);
};

#define TRACE(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_TRACE, ##__VA_ARGS__)
#define DEBUG(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_DEBUG, ##__VA_ARGS__)
#define INFO(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_INFO, ##__VA_ARGS__)
#define LOG(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_LOG, ##__VA_ARGS__)
#define WARN(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_WARN, ##__VA_ARGS__)
#define ERROR(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_ERROR, ##__VA_ARGS__)
#define FATAL(...) log(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_FATAL, ##__VA_ARGS__)

#define TRACEF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_TRACE, ##__VA_ARGS__)
#define DEBUGF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_DEBUG, ##__VA_ARGS__)
#define INFOF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_INFO, ##__VA_ARGS__)
#define LOGF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_LOG, ##__VA_ARGS__)
#define WARNF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_WARN, ##__VA_ARGS__)
#define ERRORF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_ERROR, ##__VA_ARGS__)
#define FATALF(...) logf(__FILE__, __FUNCTION__, __LINE__, logger::LEVEL_FATAL, ##__VA_ARGS__)

class logger final
{
public:
    static constexpr int LEVEL_ALL = 0;
    static constexpr int LEVEL_TRACE = 10;
    static constexpr int LEVEL_DEBUG = 20;
    static constexpr int LEVEL_INFO = 30;
    static constexpr int LEVEL_LOG = 40;
    static constexpr int LEVEL_WARN = 500;
    static constexpr int LEVEL_ERROR = 60;
    static constexpr int LEVEL_FATAL = 70;
    static constexpr int LEVEL_NONE = 100;

public:
    logger(const std::string& name, int level = 0);
    logger(std::string&& name, int level = 0);
    logger(const logger& l);
    logger(logger&& l);
    ~logger(void);

    logger& add_appender(log_appender* appender);
    logger& add_appender(const log_appender_ref& appender);
    logger& add_appender(log_appender_ref&& appender);
    void clear_appender(void);
    void enable(bool on = true);
    void set_top_level(int level);
    void set_level(int level);

    template<typename... Types>
    void log(const char* file, const char* func, int line, int level, Types... args)
    {
        using namespace std;
        if(level > LEVEL_NONE)
        {
            level = LEVEL_NONE;
        }
        if(level < LEVEL_ALL)
        {
            level = LEVEL_ALL;
        }
        if(m_enable && level >= m_level && level <= m_top_level)
        {
            log_datas ld;
            ld.emplace_back([file](ostream& os) { os << file; });
            ld.emplace_back([func](ostream& os) { os << func; });
            ld.emplace_back([line](ostream& os) { os << line; });
            ld.emplace_back([this](ostream& os) { os << this->m_logger_name; });
            string level_name = level_to_name(level);
            ld.emplace_back([level_name](ostream& os) { os << level_name; });
            auto pid = getpid();
            ld.emplace_back([pid](ostream& os) { os << pid; });
            auto tid = this_thread::get_id();
            ld.emplace_back([tid](ostream& os) { os << tid; });
            auto now = m_get_time_func();
            ld.emplace_back([now](ostream& os) { os << now; });
            {
                lock_guard<mutex> locker(m_appenders_mutex);
                for(log_appender_ref& a : m_appenders)
                {
                    a->write(ld, forward<Types>(args)...);
                }
            }
        }
    }
    template<typename... Types>
    void logf(const char* file, const char* func, int line, int level, const std::string& format, Types... args)
    {
        using namespace std;
        if(level > LEVEL_NONE)
        {
            level = LEVEL_NONE;
        }
        if(level < LEVEL_ALL)
        {
            level = LEVEL_ALL;
        }
        if(m_enable && level >= m_level && level <= m_top_level)
        {
            log_datas ld;
            ld.emplace_back([file](ostream& os) { os << file; });
            ld.emplace_back([func](ostream& os) { os << func; });
            ld.emplace_back([line](ostream& os) { os << line; });
            ld.emplace_back([this](ostream& os) { os << this->m_logger_name; });
            string level_name = level_to_name(level);
            ld.emplace_back([level_name](ostream& os) { os << level_name; });
            auto pid = getpid();
            ld.emplace_back([pid](ostream& os) { os << pid; });
            auto tid = this_thread::get_id();
            ld.emplace_back([tid](ostream& os) { os << tid; });
            auto now = m_get_time_func();
            ld.emplace_back([now](ostream& os) { os << now; });
            {
                lock_guard<mutex> locker(m_appenders_mutex);
                for(log_appender_ref& a : m_appenders)
                {
                    a->writef(format, ld, forward<Types>(args)...);
                }
            }
        }
    }

private:
    std::string get_time_string(void);
    std::string level_to_name(int level);

private:
    std::string m_logger_name;
    bool m_enable;
    int m_top_level;
    int m_level;
    std::vector<log_appender_ref> m_appenders;
    std::mutex m_appenders_mutex;
    std::function<std::string()> m_get_time_func;
};

}

#endif // __SNOWER_LOGGER_H__

