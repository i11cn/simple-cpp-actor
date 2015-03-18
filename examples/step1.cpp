#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <snower/actor/actor.h>

using namespace std;
using namespace std::chrono;
using namespace snower::actor;

class TestActor : public snower::actor::actor
{
public:
    TestActor(void)
    {
        handle(&TestActor::foo, this);
    };
    virtual ~TestActor(void) {};

    void foo(string msg)
    {
        cout << msg << endl;
    }
};

int main(int argc, char* argv[])
{
    auto addr = spawn<TestActor>();
    send(addr, string("测试消息"));
    this_thread::sleep_for(milliseconds(100));
    return 0;
}

