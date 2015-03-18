#include <snower/actor/actor.h>

#include <snower/logger.h>

namespace snower
{
namespace actor
{

template<typename Actor, typename... Types>
actor_address actor_system::spawn(Types&&... args)
{
    Actor* use = new Actor(std::forward<Types>(args)...);
    class actor* a = static_cast<class actor*>(use);
    uint64_t id = gen_id();
    std::string name = rand_name(id);
    actor_local_id* addr = new actor_local_id(id, name);
    return add_actor(addr, a);
}

template<typename Actor, typename... Types>
actor_address actor_system::spawned_by(const actor_address& parent, Types&&... args)
{
    Actor* use = new Actor(std::forward<Types>(args)...);
    class actor* a = static_cast<class actor*>(use);
    uint64_t id = gen_id();
    std::string name = rand_name(id);
    actor_local_id* addr = new actor_local_id(id, name);
    return add_actor(addr, a);
}

template<typename Actor, typename... Types>
actor_address actor_system::spawn_and_named(const std::string& name, Types&&... args)
{
    Actor* use = new Actor(std::forward<Types>(args)...);
    class actor* a = static_cast<class actor*>(use);
    uint64_t id = gen_id();
    actor_local_id* addr = new actor_local_id(id, name);
    return add_actor(addr, a);
}

template<typename Actor, typename... Types>
actor_address actor_system::spawn_and_named_by(const actor_address& parent, const std::string& name, Types&&... args)
{
    Actor* use = new Actor(std::forward<Types>(args)...);
    class actor* a = static_cast<class actor*>(use);
    uint64_t id = gen_id();
    actor_local_id* addr = new actor_local_id(id, name);
    return add_actor(addr, a);
}

template<typename... Types>
void actor_system::send(const actor_address& addr, Types&&... args)
{
    send_as(actor_address(), addr, std::forward<Types>(args)...);
}

template<typename... Types>
void actor_system::send_as(const actor_address& sender, const actor_address& addr, Types&&... args)
{
    using namespace std;
    bool exist;
    addr_ref lid;
    mailbox_ref mb;
    actor_ref act;
    tie(exist, lid, mb, act) = get_actor_bundle(addr);
    if(exist && lid && mb && act)
    {
        logger& l = singletons<logger>::get_instance("actor_system");
        l.INFO("actor找到了");
        l.INFO("lambda之前 : " , sender);
        function<void(Types...)> f1([&, act, this, sender](Types&&... args){
                logger& l = singletons<logger>::get_instance("actor_system");
                l.INFO("正在调用lambda...");
                l.INFO("lambda中 : " , sender);
                act->m_sender = sender;
                bool result = act->call(forward<Types>(args)...);
                act->m_sender = actor_address();
                l.INFO("调用结果为", result);
            });
        auto f2(bind(f1, forward<Types>(args)...));
        bool result = mb->push(f2);
        l.INFO("加入函数，结果为", result);
        if(mb->add_to_pool())
        {
            pool_mailbox(mb);
        }
    }
    else
    {
        logger& l = singletons<logger>::get_instance("actor_system");
        l.WARN("没有找到actor");
    }
}

} // namespace actor
} // namespace snower

