#include <iostream>
#include <gtest/gtest.h>
#include <snower/actor/actor.h>
#include <snower/actor/actor_system.h>

using namespace std;
using namespace snower;
using namespace snower::actor;

void func(int a, const char* msg)
{
}

class TestActor : public snower::actor::actor
{
private:
    using parent_type = snower::actor::actor;

public:
    TestActor(bool& flag, const std::string& msg)
    : m_flag(flag)
    {
        if(msg.compare("test create actor") == 0)
        {
            flag = true;
        }
        parent_type::handle(&TestActor::foo, this);
        parent_type::handle(&TestActor::foo1, this);
        parent_type::handle(&TestActor::foo2, this);
        parent_type::handle(&TestActor::foo3, this);
        parent_type::handle(&TestActor::foo4, this);
    }
    ~TestActor(void)
    {
        m_flag = false;
    }
    void foo(int a, const char* msg)
    {
        cout << "TestActor::foo(int, const char*) - " << a << " : " << msg << endl;
    }
    void foo1(float a, string msg)
    {
        cout << "TestActor::foo1(float, string) - " << a << " : " << msg << endl;
    }
    void foo2(float a, string&& msg)
    {
        cout << "TestActor::foo2(float, string&&) - " << a << " : " << msg << endl;
    }
    void foo4(float a, const string& msg)
    {
        cout << "TestActor::foo4(float, const string&) - " << a << " : " << msg << endl;
    }
    void foo3(int a)
    {
        cout << "TestActor::foo3(int) - " << a << endl;
    }

private:
    bool& m_flag;
};

TEST(TestActor, CreateActor)
{
    bool flag = false;
    spawn<TestActor>((bool&)flag, "test create actor");
    ASSERT_TRUE(flag);
}

TEST(TestActor, StopActor)
{
    bool flag = false;
    auto addr = spawn<TestActor>((bool&)flag, "test create actor");
    ASSERT_TRUE(flag);
    ASSERT_TRUE((bool)addr);
    stop(addr);
    ASSERT_FALSE(flag);
    ASSERT_FALSE((bool)addr);
}

TEST(TestActor, CallActor)
{
    log_appender_ref la(new console_appender("[%T] [%N] [%L] %M"));
    singletons<logger>::get_instance("actor_system").add_appender(la).enable(false);
    singletons<logger>::get_instance("mailbox").add_appender(la).enable(false);
    singletons<logger>::get_instance("actor").add_appender(la).enable(false);
    actor_system& as = singleton<actor_system>::get_instance();
    bool flag = false;
    auto addr = spawn<TestActor>((bool&)flag, "test create actor");
    ASSERT_TRUE(flag);
    ASSERT_TRUE((bool)addr);
    send(addr, 1, "send msg to foo");
    //as.call(addr);
    send(addr, 2, "send msg to foo");
    string msg("send msg to foo*");
    send(addr, 3.1f, string("send float and string"));
    send(addr, 3.2f, "send float and const char*");
    send(addr, 3.3f, msg);
    send(addr, 3.4f, (const string&)msg);
    //as.call(addr);
    send(addr, 4);
    //as.call(addr);
    this_thread::sleep_for(std::chrono::milliseconds(1000));
    stop(addr);
    ASSERT_FALSE(flag);
    ASSERT_FALSE((bool)addr);
}

