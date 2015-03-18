#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include <snower/logger.h>
#include <snower/singleton.h>

using namespace std;
using namespace std::chrono;
using namespace snower;

TEST(TestLogger, ConsoleLogger)
{
    logger& l = singletons<logger>::get_instance("console");
    l.add_appender(new console_appender("(%N) %T - %M , %M"));
    l.add_appender(new file_appender<file_infinite>("(%N) %T - %M , %M", new file_infinite("/home/snower/work/actor/test.log")));
    //l.add_appender(new file_appender<file_truncated>("(%N) %T - %M , %M", new file_truncated("/home/snower/work/actor/test2.log", 128)));
    //l.add_appender(new file_appender<file_splitted>("(%N) %T - %M , %M", new file_splitted("/home/snower/work/actor/test3.log", 128)));
    l.enable(true);
    l.set_level(logger::LEVEL_ALL);
    for(int i = 0; i < 20; i++)
    {
        l.LOG("test", 123);
        this_thread::sleep_for(seconds(1));
    }
}

