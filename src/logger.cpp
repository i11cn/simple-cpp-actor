#include <limits.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <snower/logger.h>
#include <snower/singleton.h>

using namespace std;
using namespace snower;

namespace snower
{

void log_to_stream(std::ostream& os, const log_datas& ld, const char* format)
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
                    os << "%m"; break;

                case 'M':
                    os << "%M"; break;

                default:
                    os << '%' << *format; break;
                }
            }
        }
        else
        {
            os << *format;
        }
    }
}

log_appender::log_appender(const char* format)
: m_format(format)
{
}

log_appender::log_appender(const std::string& format)
: m_format(format)
{
}

log_appender::log_appender(std::string&& format)
: m_format(std::move(format))
{
}

void console_appender::do_write(const std::string& log_msg)
{
    using namespace std;
    static mutex write_mutex;
    lock_guard<mutex> locker(write_mutex);
    cout << log_msg;
}

void stderr_appender::do_write(const std::string& log_msg)
{
    using namespace std;
    lock_guard<mutex> locker(m_write_mutex);
    cerr << log_msg;
}

std::vector<std::string> split_string(const std::string& src, const std::string& deli, int max = 0)
{
    using namespace std;
    vector<string> ret;
    size_t pos = 0;
    size_t deli_len = deli.size();
    while(true)
    {
        if(max == 0 || ret.size() < (max - 1))
        {
            size_t find_pos = src.find(deli, pos);
            if(find_pos != src.npos)
            {
                ret.push_back(src.substr(pos, find_pos - pos));
                pos = find_pos + deli_len;
            }
            else
            {
                ret.push_back(src.substr(pos));
                return move(ret);
            }
        }
        else
        {
            ret.push_back(src.substr(pos));
            return move(ret);
        }
    }
    return move(ret);
}

bool is_abs_path_name(const std::string& name)
{
#ifdef WIN32
    return name.find(":\\") != name.npos;
#else
    return name.compare(0, 1, "/") == 0;
#endif // WIN32
}

std::string get_abs_path_name(const std::string& name)
{
    using namespace std;
#ifdef WIN32
    static string slash = "\\";
#else
    static string slash = "/";
#endif // WIN32
    vector<string> parts = split_string(name, slash);
    if(!is_abs_path_name(name))
    {
        char buf[PATH_MAX];
        getcwd(buf, PATH_MAX);
        vector<string> use = split_string(string(buf), slash);
        for(string& s : parts)
        {
            use.push_back(s);
        }
        use.swap(parts);
    }
    vector<string> use;
    for(string& s : parts)
    {
        if(s.empty() || s.compare(".") == 0)
        {
            continue;
        }
        if(s.compare("..") == 0)
        {
            use.pop_back();
            continue;
        }
        use.push_back(s);
    }
    std::stringstream ss;
    for(string& s : use)
    {
        ss << slash << s;
    }
    return ss.str();
}

template<>
file_object_base::file_object_base(const std::string& name, size_t size, std::chrono::hours&& rel_time)
: m_file_name(get_abs_path_name(name))
, m_truncate_size(size)
, m_file_size(0)
{
    using namespace std;
    using namespace std::chrono;
    get_next_open_time = [&](const system_clock::time_point& t) ->system_clock::time_point {
        hours delta = move(rel_time);
        time_t tm_t = system_clock::to_time_t(t);
        tm ltm = *(std::localtime(&tm_t));
        ltm.tm_min = 0;
        ltm.tm_sec = 0;
        return system_clock::from_time_t(mktime(&ltm)) + delta;
    };
    get_postfix_from_time = [](const system_clock::time_point& t) -> string {
        char name[64] = {0};
        time_t tm_t = system_clock::to_time_t(t);
        tm ltm = *(std::localtime(&tm_t));
        snprintf(name, 63, "%04d-%02d-%02d.%02d", ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour);
        return move(string(name));
    };
}

template<>
file_object_base::file_object_base(const std::string& name, size_t size, std::chrono::minutes&& rel_time)
: m_file_name(get_abs_path_name(name))
, m_truncate_size(size)
, m_file_size(0)
{
    using namespace std;
    using namespace std::chrono;
    get_next_open_time = [&](const system_clock::time_point& t) ->system_clock::time_point {
        minutes delta = move(rel_time);
        time_t tm_t = system_clock::to_time_t(t);
        tm ltm = *(std::localtime(&tm_t));
        ltm.tm_min = 0;
        return system_clock::from_time_t(mktime(&ltm)) + delta;
    };
    get_postfix_from_time = [](const system_clock::time_point& t) -> string {
        char name[64] = {0};
        time_t tm_t = system_clock::to_time_t(t);
        tm ltm = *(std::localtime(&tm_t));
        snprintf(name, 63, "%04d-%02d-%02d.%02d%02d", ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour, ltm.tm_min);
        return move(string(name));
    };
}

template<>
file_object_base::file_object_base(const std::string& name, size_t size, std::chrono::seconds&& rel_time)
: m_file_name(get_abs_path_name(name))
, m_truncate_size(size)
, m_file_size(0)
{
    using namespace std;
    using namespace std::chrono;
    get_next_open_time = [&](const system_clock::time_point& t) ->system_clock::time_point {
        seconds delta = move(rel_time);
        time_t tm_t = system_clock::to_time_t(t);
        return system_clock::from_time_t(tm_t) + delta;
    };
}

file_object_base::~file_object_base(void)
{
    if(m_file.is_open())
    {
        m_file.close();
    }
}

void file_object_base::write(const std::string& msg)
{
    using namespace std;
    using namespace std::chrono;
    bool time_out = false;
    system_clock::time_point now = system_clock::now();
    if(get_next_open_time)
    {
        now = system_clock::now();
        time_out = (now >= m_file_open_time);
    }
    lock_guard<mutex> locker(m_file_mutex);
    check_file(msg.length(), time_out, now);
    if(m_file.is_open())
    {
        m_file << msg;
        m_file_size += msg.length();
        if(time_out)
        {
            m_file_open_time = get_next_open_time(now);
        }
    }
}

std::string file_object_base::get_abs_path_name(const std::string& name)
{
    using namespace std;
#ifdef WIN32
    static string slash = "\\";
#else
    static string slash = "/";
#endif // WIN32
    vector<string> parts = split_string(name, slash);
    if(!is_abs_path_name(name))
    {
        char buf[PATH_MAX];
        getcwd(buf, PATH_MAX);
        vector<string> use = split_string(string(buf), slash);
        for(string& s : parts)
        {
            use.push_back(s);
        }
        use.swap(parts);
    }
    vector<string> use;
    for(string& s : parts)
    {
        if(s.empty() || s.compare(".") == 0)
        {
            continue;
        }
        if(s.compare("..") == 0)
        {
            use.pop_back();
            continue;
        }
        use.push_back(s);
    }
    std::stringstream ss;
    for(string& s : use)
    {
        ss << slash << s;
    }
    return ss.str();
}

file_infinite::~file_infinite(void)
{
}

void file_infinite::check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now)
{
    if(!m_file.is_open())
    {
        m_file.open(m_file_name, std::ios_base::out | std::ios_base::app);
    }
}

file_truncated::~file_truncated(void)
{
}

void file_truncated::check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now)
{
    using namespace std;
    if(!m_file.is_open() || ((m_file_size + add_len) > m_truncate_size) || time_out)
    {
        if(m_file.is_open())
        {
            m_file.close();
        }
        m_file.open(m_file_name, ios_base::out | ios_base::trunc);
        m_file_size = 0;
    }
}

file_splitted::~file_splitted(void)
{
}

void file_splitted::check_file(size_t add_len, bool time_out, const std::chrono::system_clock::time_point& now)
{
    using namespace std;
    if(!m_file.is_open() || ((m_file_size + add_len) > m_truncate_size) || time_out)
    {
        if(m_file.is_open())
        {
            m_file.close();
        }
        stringstream ss;
        ss << m_file_name << "." << get_postfix_from_time(now);
        m_file.open(ss.str(), ios_base::out | ios_base::trunc);
        m_file_size = 0;
    }
}

logger::logger(const std::string& name, int level)
: m_logger_name(name)
, m_enable(true)
, m_level(level)
, m_top_level(LEVEL_NONE)
{
    m_get_time_func = std::bind(&logger::get_time_string, this);
}

logger::logger(std::string&& name, int level)
: m_logger_name(std::move(name))
, m_enable(true)
, m_level(level)
, m_top_level(LEVEL_NONE)
{
    m_get_time_func = std::bind(&logger::get_time_string, this);
}

logger::logger(const logger& l)
: m_logger_name(l.m_logger_name)
, m_enable(l.m_enable)
, m_level(l.m_level)
, m_top_level(LEVEL_NONE)
, m_appenders(l.m_appenders)
{
    m_get_time_func = std::bind(&logger::get_time_string, this);
}

logger::logger(logger&& l)
: m_logger_name(std::move(l.m_logger_name))
, m_enable(l.m_enable)
, m_level(l.m_level)
, m_top_level(LEVEL_NONE)
, m_appenders(std::move(l.m_appenders))
{
    m_get_time_func = std::bind(&logger::get_time_string, this);
}

logger::~logger(void)
{
}

logger& logger::add_appender(log_appender* appender)
{
    return add_appender(log_appender_ref(appender));
}

logger& logger::add_appender(const log_appender_ref& appender)
{
    m_appenders.push_back(appender);
    return *this;
}

logger& logger::add_appender(log_appender_ref&& appender)
{
    m_appenders.push_back(std::move(appender));
    return *this;
}

void logger::clear_appender(void)
{
    m_appenders.clear();
}

void logger::enable(bool on)
{
    m_enable = on;
}

void logger::set_top_level(int level)
{
    if(level > LEVEL_NONE)
    {
        level = LEVEL_NONE;
    }
    if(level < LEVEL_ALL)
    {
        level = LEVEL_ALL;
    }
    m_top_level = level;
}

void logger::set_level(int level)
{
    if(level > LEVEL_NONE)
    {
        level = LEVEL_NONE;
    }
    if(level < LEVEL_ALL)
    {
        level = LEVEL_ALL;
    }
    m_level = level;
}

std::string logger::get_time_string(void)
{
    using namespace std;
    string ret;
    char buff[64] = {0};
    struct timeval tmval;
    gettimeofday(&tmval, NULL);
    time_t now = tmval.tv_sec;
    struct tm newTimeObj;
    struct tm* newTime = localtime_r(&now, &newTimeObj);
    // 格式化为：2007-12-25 01:45:32:123456
    snprintf(buff, 63, "%4d-%02d-%02d %02d:%02d:%02d:%06ld"
            , newTime->tm_year + 1900, newTime->tm_mon + 1
            , newTime->tm_mday, newTime->tm_hour
            , newTime->tm_min, newTime->tm_sec, (long int)tmval.tv_usec);
    return move(string(buff));
}

std::string logger::level_to_name(int level)
{
    switch(level)
    {

    case LEVEL_TRACE:
        return "TRACE";

    case LEVEL_DEBUG:
        return "DEBUG";

    case LEVEL_INFO:
        return "INFO";

    case LEVEL_LOG:
        return "LOG";

    case LEVEL_WARN:
        return "WARN";

    case LEVEL_ERROR:
        return "ERROR";

    case LEVEL_FATAL:
        return "FATAL";
    }
    if(level < LEVEL_TRACE)
    {
        return "TRACE-";
    }
    else if(level < (LEVEL_DEBUG - (LEVEL_DEBUG - LEVEL_TRACE) / 2))
    {
        return "TRACE+";
    }
    else if(level < LEVEL_DEBUG)
    {
        return "DEBUG-";
    }
    else if(level < (LEVEL_INFO - (LEVEL_INFO - LEVEL_DEBUG) / 2))
    {
        return "DEBUG+";
    }
    else if(level < LEVEL_INFO)
    {
        return "INFO-";
    }
    else if(level < (LEVEL_LOG - (LEVEL_LOG - LEVEL_INFO) / 2))
    {
        return "INFO+";
    }
    else if(level < LEVEL_LOG)
    {
        return "LOG-";
    }
    else if(level < (LEVEL_WARN - (LEVEL_WARN - LEVEL_LOG) / 2))
    {
        return "LOG+";
    }
    else if(level < LEVEL_WARN)
    {
        return "WARN-";
    }
    else if(level < (LEVEL_ERROR - (LEVEL_ERROR - LEVEL_WARN) / 2))
    {
        return "WARN+";
    }
    else if(level < LEVEL_ERROR)
    {
        return "ERROR-";
    }
    else if(level < (LEVEL_FATAL - (LEVEL_FATAL - LEVEL_ERROR) / 2))
    {
        return "ERROR+";
    }
    else if(level < LEVEL_FATAL)
    {
        return "FATAL-";
    }
    else
    {
        return "FATAL+";
    }
}

} // namespace snower

