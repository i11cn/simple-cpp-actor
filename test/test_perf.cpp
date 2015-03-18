#include <chrono>
#include <gtest/gtest.h>
#include <snower/actor/actor.h>

using namespace std;
using namespace std::chrono;
using namespace snower;
using namespace snower::actor;

class TestPingActor : public snower::actor::actor
{
public:
    TestPingActor(void)
    {
        handle(&TestPingActor::ping, this);
    }
    void ping(const char* s)
    {
        //reply("pong");
        m_count++;
    }

    int32_t m_count = 0;
};

class TestPongActor : public snower::actor::actor
{
public:
    TestPongActor(void)
    {
        handle(&TestPongActor::pong, this);
    }
    void pong(const char* s)
    {
        for(int i = 0; i < 1000; i++)
        {
            reply("ping");
            m_count++;
        }
        send(get_self(), "pong");
    }

    int32_t m_count = 0;
};

TEST(TestPerformence, PingPong)
{
    singletons<logger>::get_instance("actor_system").enable(false);
    actor_system& as = singleton<actor_system>::get_instance();

    auto a1 = spawn<TestPingActor>();
    auto a2 = spawn<TestPongActor>();

    cout << a1 << endl;
    cout << a2 << endl;

    auto ping = as.get_actor(a1);
    auto pong = as.get_actor(a2);

    send_as(a1, a2, "pong");
    this_thread::sleep_for(seconds(5));
    stop(a1);
    stop(a2);
    int32_t total = ((TestPingActor*)ping.get())->m_count + ((TestPongActor*)pong.get())->m_count;
    cout << total << endl;
}

